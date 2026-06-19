#include <stdint.h>

enum {
  POLY_FPU_OK = 6147,
};

static int is_quiet_nan64(uint64_t bits) {
  return (bits & UINT64_C(0x7ff0000000000000)) ==
      UINT64_C(0x7ff0000000000000) &&
    (bits & UINT64_C(0x0008000000000000)) != 0 &&
    (bits & UINT64_C(0x000fffffffffffff)) != 0;
}

static int is_subnormal64(uint64_t bits) {
  return (bits & UINT64_C(0x7ff0000000000000)) == 0 &&
    (bits & UINT64_C(0x000fffffffffffff)) != 0;
}

#if defined(__aarch64__)
static void a64_write_fpcr(uint64_t value) {
  __asm__ volatile("msr fpcr, %x0" :: "r"(value) : "memory");
}

static void a64_write_fpsr(uint64_t value) {
  __asm__ volatile("msr fpsr, %x0" :: "r"(value) : "memory");
}

static uint64_t a64_read_fpsr(void) {
  uint64_t value;
  __asm__ volatile("mrs %x0, fpsr" : "=r"(value) :: "memory");
  return value;
}

static uint64_t a64_fadd64(uint64_t left, uint64_t right) {
  uint64_t out;
  __asm__ volatile(
    "fmov d0, %x1\n"
    "fmov d1, %x2\n"
    "fadd d2, d0, d1\n"
    "fmov %x0, d2\n"
    : "=r"(out)
    : "r"(left), "r"(right)
    : "v0", "v1", "v2", "memory");
  return out;
}

static uint64_t a64_fdiv64(uint64_t left, uint64_t right) {
  uint64_t out;
  __asm__ volatile(
    "fmov d0, %x1\n"
    "fmov d1, %x2\n"
    "fdiv d2, d0, d1\n"
    "fmov %x0, d2\n"
    : "=r"(out)
    : "r"(left), "r"(right)
    : "v0", "v1", "v2", "memory");
  return out;
}

static uint64_t a64_fsqrt64(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmov d0, %x1\n"
    "fsqrt d1, d0\n"
    "fmov %x0, d1\n"
    : "=r"(out)
    : "r"(value)
    : "v0", "v1", "memory");
  return out;
}

static uint64_t a64_simd_ucvtf64(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmov d31, %x1\n"
    "ucvtf d31, d31\n"
    "fmov %x0, d31\n"
    : "=r"(out)
    : "r"(value)
    : "v31", "memory");
  return out;
}

static uint64_t a64_frintm64(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmov d31, %x1\n"
    "frintm d31, d31\n"
    "fmov %x0, d31\n"
    : "=r"(out)
    : "r"(value)
    : "v31", "memory");
  return out;
}

static uint64_t a64_fcvtzu64(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmov d29, %x1\n"
    "fcvtzu %x0, d29\n"
    : "=r"(out)
    : "r"(value)
    : "v29", "memory");
  return out;
}

static uint64_t a64_fcvtmu64(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmov d30, %x1\n"
    "fcvtmu %x0, d30\n"
    : "=r"(out)
    : "r"(value)
    : "v30", "memory");
  return out;
}

static uint64_t a64_scvtf_fixed_d_w(int32_t value) {
  uint64_t out;
  __asm__ volatile(
    "scvtf d31, %w1, #2\n"
    "fmov %x0, d31\n"
    : "=r"(out)
    : "r"(value)
    : "v31", "memory");
  return out;
}

static uint64_t a64_rehash_small_probe(void) {
  uint64_t branch_taken = 0;
  uint64_t selected = 0;
  uint64_t converted = 0;

  __asm__ volatile(
    "mov x1, #1\n"
    "mov x2, #1\n"
    "fmov d31, #11.0\n"
    "fmov s30, #1.0\n"
    "ucvtf d0, x1\n"
    "fcvt d30, s30\n"
    "fdiv d31, d31, d30\n"
    "fcmpe d0, d31\n"
    "b.ls 1f\n"
    "mov %x[branch], #0\n"
    "b 2f\n"
    "1:\n"
    "mov %x[branch], #1\n"
    "frintm d31, d31\n"
    "fmov d29, #1.0\n"
    "lsl x1, x1, #1\n"
    "fadd d29, d31, d29\n"
    "fcvtzu x2, d29\n"
    "cmp x2, x1\n"
    "csel x1, x2, x1, cs\n"
    "2:\n"
    "mov %x[selected], x1\n"
    "mov %x[converted], x2\n"
    : [branch] "=r"(branch_taken),
      [selected] "=r"(selected),
      [converted] "=r"(converted)
    :
    : "x1", "x2", "v0", "v29", "v30", "v31", "cc", "memory");

  return (branch_taken << 32) | (selected << 16) | converted;
}

static uint64_t run_aarch64_fpu_torture(void) {
  const uint64_t fpsr_ioc = UINT64_C(1) << 0;
  const uint64_t fpsr_ufc = UINT64_C(1) << 3;
  const uint64_t fpsr_ixc = UINT64_C(1) << 4;
  const uint64_t qnan_payload = UINT64_C(0x7ff8000000012345);
  const uint64_t snan_payload = UINT64_C(0x7ff0000000012345);

  a64_write_fpcr(0);
  a64_write_fpsr(0);
  uint64_t quiet = a64_fadd64(qnan_payload, UINT64_C(0x3ff0000000000000));
  if (!is_quiet_nan64(quiet) ||
      (quiet & UINT64_C(0x0007ffffffffffff)) != UINT64_C(0x12345) ||
      a64_read_fpsr() != 0)
    return 10;

  a64_write_fpsr(0);
  uint64_t signaled = a64_fadd64(snan_payload, UINT64_C(0x3ff0000000000000));
  if (!is_quiet_nan64(signaled) ||
      (signaled & UINT64_C(0x0007ffffffffffff)) != UINT64_C(0x12345) ||
      (a64_read_fpsr() & fpsr_ioc) == 0)
    return 11;

  a64_write_fpsr(0);
  uint64_t sqrt_nan = a64_fsqrt64(UINT64_C(0xbff0000000000000));
  if (!is_quiet_nan64(sqrt_nan) || (a64_read_fpsr() & fpsr_ioc) == 0)
    return 12;

  a64_write_fpsr(0);
  (void) a64_fdiv64(UINT64_C(0x3ff0000000000000),
    UINT64_C(0x4008000000000000));
  if ((a64_read_fpsr() & fpsr_ixc) == 0)
    return 13;

  a64_write_fpsr(0);
  uint64_t tiny = a64_fdiv64(UINT64_C(0x0010000000000000),
    UINT64_C(0x4008000000000000));
  if (!is_subnormal64(tiny) ||
      (a64_read_fpsr() & (fpsr_ufc | fpsr_ixc)) != (fpsr_ufc | fpsr_ixc))
    return 14;

  a64_write_fpsr(0);
  a64_write_fpcr(UINT64_C(1) << 22);
  uint64_t rup = a64_fadd64(UINT64_C(0x3ff0000000000000),
    UINT64_C(0x3ca0000000000000));
  if (rup != UINT64_C(0x3ff0000000000001) ||
      (a64_read_fpsr() & fpsr_ixc) == 0)
    return 15;

  a64_write_fpsr(0);
  a64_write_fpcr(UINT64_C(2) << 22);
  uint64_t rdn = a64_fadd64(UINT64_C(0xbff0000000000000),
    UINT64_C(0xbca0000000000000));
  if (rdn != UINT64_C(0xbff0000000000001) ||
      (a64_read_fpsr() & fpsr_ixc) == 0)
    return 16;

  a64_write_fpcr(0);
  a64_write_fpsr(0);
  if (a64_simd_ucvtf64(13) != UINT64_C(0x402a000000000000))
    return 17;

  a64_write_fpsr(0);
  if (a64_frintm64(UINT64_C(0x4027800000000000)) !=
      UINT64_C(0x4026000000000000))
    return 18;

  a64_write_fpsr(0);
  if (a64_fcvtzu64(UINT64_C(0x4028000000000000)) != 12)
    return 19;

  a64_write_fpsr(0);
  if (a64_fcvtmu64(UINT64_C(0x402a000000000000)) != 13)
    return 27;

  a64_write_fpsr(0);
  if (a64_rehash_small_probe() != UINT64_C(0x00000001000c000c))
    return 28;

  a64_write_fpsr(0);
  if (a64_scvtf_fixed_d_w(-9) != UINT64_C(0xc002000000000000))
    return 29;

  a64_write_fpcr(0);
  a64_write_fpsr(0);
  return POLY_FPU_OK;
}
#endif

#if defined(__riscv)
static void rv_write_fcsr(uint64_t value) {
  __asm__ volatile("csrw fcsr, %0" :: "r"(value) : "memory");
}

static void rv_write_fflags(uint64_t value) {
  __asm__ volatile("csrw fflags, %0" :: "r"(value) : "memory");
}

static uint64_t rv_read_fflags(void) {
  uint64_t value;
  __asm__ volatile("csrr %0, fflags" : "=r"(value) :: "memory");
  return value;
}

static uint64_t rv_fadd64_dyn(uint64_t left, uint64_t right) {
  uint64_t out;
  __asm__ volatile(
    "fmv.d.x ft0, %1\n"
    "fmv.d.x ft1, %2\n"
    "fadd.d ft2, ft0, ft1, dyn\n"
    "fmv.x.d %0, ft2\n"
    : "=r"(out)
    : "r"(left), "r"(right)
    : "ft0", "ft1", "ft2", "memory");
  return out;
}

static uint64_t rv_fdiv64_dyn(uint64_t left, uint64_t right) {
  uint64_t out;
  __asm__ volatile(
    "fmv.d.x ft0, %1\n"
    "fmv.d.x ft1, %2\n"
    "fdiv.d ft2, ft0, ft1, dyn\n"
    "fmv.x.d %0, ft2\n"
    : "=r"(out)
    : "r"(left), "r"(right)
    : "ft0", "ft1", "ft2", "memory");
  return out;
}

static uint64_t rv_fsqrt64_dyn(uint64_t value) {
  uint64_t out;
  __asm__ volatile(
    "fmv.d.x ft0, %1\n"
    "fsqrt.d ft1, ft0, dyn\n"
    "fmv.x.d %0, ft1\n"
    : "=r"(out)
    : "r"(value)
    : "ft0", "ft1", "memory");
  return out;
}

static uint64_t run_riscv_fpu_torture(void) {
  const uint64_t fflag_nx = UINT64_C(1) << 0;
  const uint64_t fflag_uf = UINT64_C(1) << 1;
  const uint64_t fflag_nv = UINT64_C(1) << 4;

  rv_write_fcsr(0);
  uint64_t quiet = rv_fadd64_dyn(UINT64_C(0x7ff8000000012345),
    UINT64_C(0x3ff0000000000000));
  if (quiet != UINT64_C(0x7ff8000000000000) || rv_read_fflags() != 0)
    return 20;

  rv_write_fflags(0);
  uint64_t signaled = rv_fadd64_dyn(UINT64_C(0x7ff0000000012345),
    UINT64_C(0x3ff0000000000000));
  if (signaled != UINT64_C(0x7ff8000000000000) ||
      (rv_read_fflags() & fflag_nv) == 0)
    return 21;

  rv_write_fflags(0);
  uint64_t sqrt_nan = rv_fsqrt64_dyn(UINT64_C(0xbff0000000000000));
  if (sqrt_nan != UINT64_C(0x7ff8000000000000) ||
      (rv_read_fflags() & fflag_nv) == 0)
    return 22;

  rv_write_fflags(0);
  (void) rv_fdiv64_dyn(UINT64_C(0x3ff0000000000000),
    UINT64_C(0x4008000000000000));
  if ((rv_read_fflags() & fflag_nx) == 0)
    return 23;

  rv_write_fflags(0);
  uint64_t tiny = rv_fdiv64_dyn(UINT64_C(0x0010000000000000),
    UINT64_C(0x4008000000000000));
  if (!is_subnormal64(tiny) ||
      (rv_read_fflags() & (fflag_uf | fflag_nx)) != (fflag_uf | fflag_nx))
    return 24;

  rv_write_fcsr(UINT64_C(3) << 5);
  uint64_t rup = rv_fadd64_dyn(UINT64_C(0x3ff0000000000000),
    UINT64_C(0x3ca0000000000000));
  if (rup != UINT64_C(0x3ff0000000000001) ||
      (rv_read_fflags() & fflag_nx) == 0)
    return 25;

  rv_write_fcsr(UINT64_C(2) << 5);
  uint64_t rdn = rv_fadd64_dyn(UINT64_C(0xbff0000000000000),
    UINT64_C(0xbca0000000000000));
  if (rdn != UINT64_C(0xbff0000000000001) ||
      (rv_read_fflags() & fflag_nx) == 0)
    return 26;

  rv_write_fcsr(0);
  return POLY_FPU_OK;
}
#endif

__attribute__((visibility("hidden")))
uint64_t poly_entry(uint64_t *scratch) {
  (void) scratch;
#if defined(__aarch64__)
  return run_aarch64_fpu_torture();
#elif defined(__riscv)
  return run_riscv_fpu_torture();
#else
#error unsupported architecture
#endif
}
