#include <stdbool.h>
#include <stdint.h>

#if defined(__GNUC__)
#define POLY_HOST_HELPER __attribute__((noinline, noclone, used))
#else
#define POLY_HOST_HELPER
#endif

extern uint64_t poly_runtime_register_atexit_callback(void *callback,
  void *arg, void *dso_handle);
extern uint64_t poly_runtime_finalize_atexit_callbacks(void *dso_handle);

static volatile uint64_t poly_host_x86_zero;
static int poly_host_errno_value;
static uint8_t poly_host_heap[64 * 1024];
static uint64_t poly_host_heap_cursor;

enum {
  POLY_HOST_HEAP_HEADER_SIZE = 16
};

static uint64_t poly_host_x86_tls_base(void)
{
  uint64_t value;
  __asm__ volatile("movq %%r13,%0" : "=r"(value));
  return value;
}

uint64_t POLY_HOST_HELPER poly_host_x86_add(uint64_t a, uint64_t b)
{
  return a + b + 200;
}

uint64_t POLY_HOST_HELPER poly_host_x86_mul(uint64_t a, uint64_t b)
{
  return a * b + 200;
}

uint64_t POLY_HOST_HELPER poly_host_import_add(uint64_t a, uint64_t b)
{
  return a + b + 100;
}

uint64_t POLY_HOST_HELPER poly_host_import_mul(uint64_t a, uint64_t b)
{
  return a * b + 100;
}

double POLY_HOST_HELPER poly_host_import_fp64_add(double a, double b)
{
  return a + b + 10.0;
}

float POLY_HOST_HELPER poly_host_import_fp32_add(float a, float b)
{
  return a + b + 10.0f;
}

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_tlsdesc(uint64_t descriptor)
{
  const uint64_t *words = (const uint64_t *) (uintptr_t) descriptor;
  return words[1];
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_tls_get_addr(uint64_t descriptor)
{
  const uint64_t *words = (const uint64_t *) (uintptr_t) descriptor;
  return poly_host_x86_tls_base() + words[1];
}

uint64_t POLY_HOST_HELPER poly_host_x86_sum6(uint64_t a, uint64_t b,
    uint64_t c, uint64_t d, uint64_t e, uint64_t f)
{
  return a + b + c + d + e + f + 200;
}

static uint64_t POLY_HOST_HELPER poly_host_x86_add_bias(uint64_t value)
{
  return value + 200;
}

uint64_t POLY_HOST_HELPER poly_host_x86_sum8(uint64_t a, uint64_t b,
    uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h)
{
  const uint64_t result =
    poly_host_x86_add_bias(a + b + c + d + e + f + g + h);
  return result + poly_host_x86_zero;
}

double POLY_HOST_HELPER poly_host_x86_fp64_add(double a, double b)
{
  return a + b + 200.5;
}

double POLY_HOST_HELPER poly_host_x86_fp64_sum8(double a, double b, double c,
    double d, double e, double f, double g, double h)
{
  return a + b + c + d + e + f + g + h + 200.5;
}

double POLY_HOST_HELPER poly_host_x86_mixed_u64_fp64(uint64_t a, double b,
    uint64_t c, double d, uint64_t e, double f)
{
  return (double) (a + c + e) + b + d + f + 200.5;
}

float POLY_HOST_HELPER poly_host_x86_fp32_add(float a, float b)
{
  return a + b + 200.5f;
}

unsigned __int128 POLY_HOST_HELPER poly_host_x86_udivti3(
    unsigned __int128 dividend, unsigned __int128 divisor)
{
  if (divisor == 0)
    return 0;
  return dividend / divisor;
}

unsigned __int128 POLY_HOST_HELPER poly_host_x86_umodti3(
    unsigned __int128 dividend, unsigned __int128 divisor)
{
  if (divisor == 0)
    return 0;
  return dividend % divisor;
}

__int128 POLY_HOST_HELPER poly_host_x86_divti3(__int128 dividend,
    __int128 divisor)
{
  if (divisor == 0)
    return 0;
  return dividend / divisor;
}

__int128 POLY_HOST_HELPER poly_host_x86_modti3(__int128 dividend,
    __int128 divisor)
{
  if (divisor == 0)
    return 0;
  return dividend % divisor;
}

__int128 POLY_HOST_HELPER poly_host_x86_fixdfti(double source)
{
  return (__int128) source;
}

unsigned __int128 POLY_HOST_HELPER poly_host_x86_fixunsdfti(double source)
{
  return (unsigned __int128) source;
}

double POLY_HOST_HELPER poly_host_x86_floattidf(__int128 source)
{
  return (double) source;
}

double POLY_HOST_HELPER poly_host_x86_floatuntidf(unsigned __int128 source)
{
  return (double) source;
}

__int128 POLY_HOST_HELPER poly_host_x86_fixsfti(float source)
{
  return (__int128) source;
}

unsigned __int128 POLY_HOST_HELPER poly_host_x86_fixunssfti(float source)
{
  return (unsigned __int128) source;
}

float POLY_HOST_HELPER poly_host_x86_floattisf(__int128 source)
{
  return (float) source;
}

float POLY_HOST_HELPER poly_host_x86_floatuntisf(unsigned __int128 source)
{
  return (float) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_atomic_compare_exchange_16(
    uint64_t *ptr, uint64_t *expected, uint64_t desired_lo,
    uint64_t desired_hi, uint64_t weak, uint64_t success_order,
    uint64_t failure_order)
{
  (void) weak;
  (void) success_order;
  (void) failure_order;

  uint64_t actual_lo = ptr[0];
  uint64_t actual_hi = ptr[1];
  uint64_t expected_lo = expected[0];
  uint64_t expected_hi = expected[1];
  if (actual_lo == expected_lo && actual_hi == expected_hi) {
    ptr[0] = desired_lo;
    ptr[1] = desired_hi;
    return 1;
  }

  expected[0] = actual_lo;
  expected[1] = actual_hi;
  return 0;
}

unsigned __int128 POLY_HOST_HELPER poly_host_x86_atomic_load_16(
    uint64_t *ptr, uint64_t order)
{
  (void) order;
  return ((unsigned __int128) ptr[1] << 64) | ptr[0];
}

uint64_t POLY_HOST_HELPER poly_host_x86_atomic_store_16(uint64_t *ptr,
    uint64_t value_lo, uint64_t value_hi, uint64_t order)
{
  (void) order;
  ptr[0] = value_lo;
  ptr[1] = value_hi;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_atomic_store_16(
    uint64_t *ptr, uint64_t unused_aapcs64_x1, uint64_t value_lo,
    uint64_t value_hi, uint64_t order)
{
  (void) unused_aapcs64_x1;
  return poly_host_x86_atomic_store_16(ptr, value_lo, value_hi, order);
}

#define DEFINE_AARCH64_ATOMIC_LOAD_OP(name, op) \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_##name##1( \
    uint64_t source, uint8_t *ptr) \
{ \
  return __atomic_fetch_##op(ptr, (uint8_t) source, __ATOMIC_SEQ_CST); \
} \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_##name##2( \
    uint64_t source, uint16_t *ptr) \
{ \
  return __atomic_fetch_##op(ptr, (uint16_t) source, __ATOMIC_SEQ_CST); \
} \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_##name##4( \
    uint64_t source, uint32_t *ptr) \
{ \
  return __atomic_fetch_##op(ptr, (uint32_t) source, __ATOMIC_SEQ_CST); \
} \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_##name##8( \
    uint64_t source, uint64_t *ptr) \
{ \
  return __atomic_fetch_##op(ptr, source, __ATOMIC_SEQ_CST); \
}

DEFINE_AARCH64_ATOMIC_LOAD_OP(ldadd, add)
DEFINE_AARCH64_ATOMIC_LOAD_OP(ldeor, xor)
DEFINE_AARCH64_ATOMIC_LOAD_OP(ldset, or)

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_ldclr1(uint64_t source,
    uint8_t *ptr)
{
  return __atomic_fetch_and(ptr, (uint8_t) ~source, __ATOMIC_SEQ_CST);
}

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_ldclr2(uint64_t source,
    uint16_t *ptr)
{
  return __atomic_fetch_and(ptr, (uint16_t) ~source, __ATOMIC_SEQ_CST);
}

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_ldclr4(uint64_t source,
    uint32_t *ptr)
{
  return __atomic_fetch_and(ptr, (uint32_t) ~source, __ATOMIC_SEQ_CST);
}

uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_ldclr8(uint64_t source,
    uint64_t *ptr)
{
  return __atomic_fetch_and(ptr, ~source, __ATOMIC_SEQ_CST);
}

#define DEFINE_AARCH64_ATOMIC_SWP(width, type) \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_swp##width( \
    uint64_t source, type *ptr) \
{ \
  return __atomic_exchange_n(ptr, (type) source, __ATOMIC_SEQ_CST); \
}

DEFINE_AARCH64_ATOMIC_SWP(1, uint8_t)
DEFINE_AARCH64_ATOMIC_SWP(2, uint16_t)
DEFINE_AARCH64_ATOMIC_SWP(4, uint32_t)
DEFINE_AARCH64_ATOMIC_SWP(8, uint64_t)

#define DEFINE_AARCH64_ATOMIC_CAS(width, type) \
uint64_t POLY_HOST_HELPER poly_host_x86_aarch64_cas##width( \
    uint64_t expected_value, uint64_t desired, type *ptr) \
{ \
  type expected = (type) expected_value; \
  __atomic_compare_exchange_n(ptr, &expected, (type) desired, false, \
    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
  return expected; \
}

DEFINE_AARCH64_ATOMIC_CAS(1, uint8_t)
DEFINE_AARCH64_ATOMIC_CAS(2, uint16_t)
DEFINE_AARCH64_ATOMIC_CAS(4, uint32_t)
DEFINE_AARCH64_ATOMIC_CAS(8, uint64_t)

uint64_t POLY_HOST_HELPER poly_host_x86_clzdi2(uint64_t value)
{
  uint64_t count = 0;
  for (int bit = 63; bit >= 0; bit--) {
    if ((value >> bit) & 1)
      break;
    count++;
  }
  return count;
}

uint64_t POLY_HOST_HELPER poly_host_x86_ctzdi2(uint64_t value)
{
  uint64_t count = 0;
  for (int bit = 0; bit < 64; bit++) {
    if ((value >> bit) & 1)
      break;
    count++;
  }
  return count;
}

uint64_t POLY_HOST_HELPER poly_host_x86_paritydi2(uint64_t value)
{
  uint64_t parity = 0;
  while (value) {
    parity ^= value & 1;
    value >>= 1;
  }
  return parity;
}

uint64_t POLY_HOST_HELPER poly_host_x86_popcountdi2(uint64_t value)
{
  uint64_t count = 0;
  while (value) {
    count += value & 1;
    value >>= 1;
  }
  return count;
}

static __float128 poly_host_x86_make_tf(uint64_t lo, uint64_t hi)
{
  union {
    __float128 value;
    uint64_t words[2];
  } packed;
  packed.words[0] = lo;
  packed.words[1] = hi;
  return packed.value;
}

__float128 POLY_HOST_HELPER poly_host_x86_addtf3(__float128 left,
    __float128 right)
{
  return left + right;
}

__float128 POLY_HOST_HELPER poly_host_x86_riscv_addtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_addtf3(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_subtf3(__float128 left,
    __float128 right)
{
  return left - right;
}

__float128 POLY_HOST_HELPER poly_host_x86_riscv_subtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_subtf3(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_multf3(__float128 left,
    __float128 right)
{
  return left * right;
}

__float128 POLY_HOST_HELPER poly_host_x86_riscv_multf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_multf3(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_divtf3(__float128 left,
    __float128 right)
{
  return left / right;
}

__float128 POLY_HOST_HELPER poly_host_x86_riscv_divtf3(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_divtf3(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_floatunditf(uint64_t source)
{
  return (__float128) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_fixunstfdi(__float128 source)
{
  return (uint64_t) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_fixunstfdi(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_fixunstfdi(poly_host_x86_make_tf(source_lo, source_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_floatditf(int64_t source)
{
  return (__float128) source;
}

__float128 POLY_HOST_HELPER poly_host_x86_floatsitf(int64_t source)
{
  return (__float128) (int32_t) source;
}

__float128 POLY_HOST_HELPER poly_host_x86_floatunsitf(uint64_t source)
{
  return (__float128) (uint32_t) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_fixtfdi(__float128 source)
{
  return (uint64_t) (int64_t) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_fixtfdi(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_fixtfdi(poly_host_x86_make_tf(source_lo, source_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_fixtfsi(__float128 source)
{
  return (uint64_t) (int64_t) (int32_t) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_fixtfsi(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_fixtfsi(poly_host_x86_make_tf(source_lo, source_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_fixunstfsi(__float128 source)
{
  return (uint64_t) (uint32_t) source;
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_fixunstfsi(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_fixunstfsi(poly_host_x86_make_tf(source_lo, source_hi));
}

static uint64_t poly_host_x86_tf_compare(__float128 left, __float128 right,
    uint64_t import_kind)
{
  const int unordered = (left != left) || (right != right);
  if (import_kind == 102)
    return unordered ? 1 : 0;
  if (unordered) {
    if (import_kind == 92 || import_kind == 101 ||
        import_kind == 93 || import_kind == 94)
      return 1;
    return (uint64_t) -1;
  }
  if (import_kind == 92 || import_kind == 101)
    return left == right ? 0 : 1;
  if (left < right)
    return (uint64_t) -1;
  if (left > right)
    return 1;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_eqtf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 92);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_eqtf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_eqtf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_lttf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 93);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_lttf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_lttf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_letf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 94);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_letf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_letf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_gttf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 95);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_gttf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_gttf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_getf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 96);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_getf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_getf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

__float128 POLY_HOST_HELPER poly_host_x86_extendsftf2(float source)
{
  return (__float128) source;
}

__float128 POLY_HOST_HELPER poly_host_x86_extenddftf2(double source)
{
  return (__float128) source;
}

float POLY_HOST_HELPER poly_host_x86_trunctfsf2(__float128 source)
{
  return (float) source;
}

float POLY_HOST_HELPER poly_host_x86_riscv_trunctfsf2(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_trunctfsf2(poly_host_x86_make_tf(source_lo, source_hi));
}

double POLY_HOST_HELPER poly_host_x86_trunctfdf2(__float128 source)
{
  return (double) source;
}

double POLY_HOST_HELPER poly_host_x86_riscv_trunctfdf2(uint64_t source_lo,
    uint64_t source_hi)
{
  return poly_host_x86_trunctfdf2(poly_host_x86_make_tf(source_lo, source_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_netf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 101);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_netf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_netf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

uint64_t POLY_HOST_HELPER poly_host_x86_unordtf2(__float128 left,
    __float128 right)
{
  return poly_host_x86_tf_compare(left, right, 102);
}

uint64_t POLY_HOST_HELPER poly_host_x86_riscv_unordtf2(uint64_t left_lo,
    uint64_t left_hi, uint64_t right_lo, uint64_t right_hi)
{
  return poly_host_x86_unordtf2(poly_host_x86_make_tf(left_lo, left_hi),
    poly_host_x86_make_tf(right_lo, right_hi));
}

static uint64_t poly_host_bound_4096(uint64_t count)
{
  return count < 4096 ? count : 4096;
}

static unsigned char poly_host_ascii_lower(unsigned char value)
{
  if (value >= 'A' && value <= 'Z')
    return (unsigned char) (value + ('a' - 'A'));
  return value;
}

static uint64_t poly_host_align_up(uint64_t value, uint64_t alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

static void *poly_host_alloc_aligned(uint64_t size, uint64_t alignment)
{
  if (alignment < 8)
    alignment = 8;
  if ((alignment & (alignment - 1)) != 0)
    return 0;

  const uint64_t base = (uint64_t) (uintptr_t) poly_host_heap;
  const uint64_t min_raw = poly_host_heap_cursor +
    POLY_HOST_HEAP_HEADER_SIZE;
  if (min_raw < poly_host_heap_cursor || min_raw > sizeof(poly_host_heap))
    return 0;
  if (base > UINT64_MAX - min_raw)
    return 0;
  const uint64_t absolute = base + min_raw;
  if (absolute > UINT64_MAX - (alignment - 1))
    return 0;

  const uint64_t aligned = poly_host_align_up(absolute, alignment);
  if (aligned < base)
    return 0;
  const uint64_t raw = aligned - base;
  if (raw < POLY_HOST_HEAP_HEADER_SIZE)
    return 0;
  const uint64_t header = raw - POLY_HOST_HEAP_HEADER_SIZE;
  if (size > sizeof(poly_host_heap) || raw > sizeof(poly_host_heap) - size)
    return 0;

  poly_host_heap_cursor = raw + size;
  *((uint64_t *) (void *) (poly_host_heap + header)) = size;
  *((uint64_t *) (void *) (poly_host_heap + header + 8)) = raw;
  return poly_host_heap + raw;
}

static uint64_t poly_host_alloc_size(const void *ptr)
{
  const uint8_t *bytes = (const uint8_t *) ptr;
  if (bytes < poly_host_heap + POLY_HOST_HEAP_HEADER_SIZE ||
      bytes >= poly_host_heap + sizeof(poly_host_heap))
    return 0;
  return *((const uint64_t *) (const void *) (bytes -
    POLY_HOST_HEAP_HEADER_SIZE));
}

uint64_t POLY_HOST_HELPER poly_host_x86_errno_location(void)
{
  return (uint64_t) (uintptr_t) &poly_host_errno_value;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getauxval(uint64_t type)
{
  (void) type;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getpagesize(void)
{
  return 4096;
}

uint64_t POLY_HOST_HELPER poly_host_x86_sysconf(uint64_t name)
{
  return name == 30 ? 4096 : (uint64_t) -1;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getenv(const char *name)
{
  (void) name;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_malloc(uint64_t size)
{
  return (uint64_t) (uintptr_t) poly_host_alloc_aligned(size, 8);
}

uint64_t POLY_HOST_HELPER poly_host_x86_calloc(uint64_t count, uint64_t size)
{
  if (count != 0 && size > UINT64_MAX / count)
    return 0;
  const uint64_t total = count * size;
  uint8_t *ptr = (uint8_t *) poly_host_alloc_aligned(total, 8);
  for (uint64_t n = 0; ptr != 0 && n < total; n++)
    ptr[n] = 0;
  return (uint64_t) (uintptr_t) ptr;
}

uint64_t POLY_HOST_HELPER poly_host_x86_realloc(uint8_t *old_ptr,
    uint64_t size)
{
  if (old_ptr == 0)
    return poly_host_x86_malloc(size);
  if (size == 0)
    return 0;

  uint8_t *new_ptr = (uint8_t *) poly_host_alloc_aligned(size, 8);
  const uint64_t old_size = poly_host_alloc_size(old_ptr);
  const uint64_t copy_size = old_size < size ? old_size : size;
  for (uint64_t n = 0; new_ptr != 0 && n < copy_size; n++)
    new_ptr[n] = old_ptr[n];
  return (uint64_t) (uintptr_t) new_ptr;
}

uint64_t POLY_HOST_HELPER poly_host_x86_free(void *ptr)
{
  (void) ptr;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_posix_memalign(uint64_t *out,
    uint64_t alignment, uint64_t size)
{
  if (alignment < 8 || (alignment & (alignment - 1)) != 0)
    return 22;
  void *ptr = poly_host_alloc_aligned(size, alignment);
  if (ptr == 0)
    return 12;
  *out = (uint64_t) (uintptr_t) ptr;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_aligned_alloc(uint64_t alignment,
    uint64_t size)
{
  if (alignment == 0 || (alignment & (alignment - 1)) != 0 ||
      size % alignment != 0)
    return 0;
  return (uint64_t) (uintptr_t) poly_host_alloc_aligned(size, alignment);
}

uint64_t POLY_HOST_HELPER poly_host_x86_memalign(uint64_t alignment,
    uint64_t size)
{
  if (alignment == 0 || (alignment & (alignment - 1)) != 0)
    return 0;
  return (uint64_t) (uintptr_t) poly_host_alloc_aligned(size, alignment);
}

uint64_t POLY_HOST_HELPER poly_host_x86_atexit(void *callback)
{
  return poly_runtime_register_atexit_callback(callback, 0, 0);
}

uint64_t POLY_HOST_HELPER poly_host_x86_cxa_atexit(void *callback, void *arg,
    void *dso_handle)
{
  return poly_runtime_register_atexit_callback(callback, arg, dso_handle);
}

uint64_t POLY_HOST_HELPER poly_host_x86_cxa_finalize(void *dso_handle)
{
  return poly_runtime_finalize_atexit_callbacks(dso_handle);
}

uint64_t POLY_HOST_HELPER poly_host_x86_cxa_guard_acquire(uint64_t *guard)
{
  uint8_t *bytes = (uint8_t *) guard;
  if ((bytes[0] & 1) != 0)
    return 0;
  bytes[1] = 1;
  return 1;
}

uint64_t POLY_HOST_HELPER poly_host_x86_cxa_guard_release(uint64_t *guard)
{
  uint8_t *bytes = (uint8_t *) guard;
  bytes[0] = 1;
  bytes[1] = 0;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_cxa_guard_abort(uint64_t *guard)
{
  uint8_t *bytes = (uint8_t *) guard;
  bytes[1] = 0;
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getpid(void)
{
  return 4242;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getppid(void)
{
  return 4241;
}

uint64_t POLY_HOST_HELPER poly_host_x86_getuid(void)
{
  return 1000;
}

uint64_t POLY_HOST_HELPER poly_host_x86_gettid(void)
{
  return 4243;
}

uint64_t POLY_HOST_HELPER poly_host_x86_puts(const uint8_t *text)
{
  uint64_t len = 0;
  while (len < 4096 && text[len] != 0)
    len++;
  if (len == 4096)
    return (uint64_t) -1;
  return (uint64_t) len + 1;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strlen(const char *text)
{
  uint64_t result = 0;
  while (result < 4096 && text[result] != 0)
    result++;
  return result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strnlen(const char *text,
    uint64_t max_len)
{
  const uint64_t count = poly_host_bound_4096(max_len);
  uint64_t result = 0;
  while (result < count && text[result] != 0)
    result++;
  return result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strdup(const uint8_t *src)
{
  uint64_t len = 0;
  while (len < 4096 && src[len] != 0)
    len++;
  if (len == 4096)
    return 0;
  uint8_t *dest = (uint8_t *) poly_host_alloc_aligned(len + 1, 8);
  for (uint64_t n = 0; dest != 0 && n <= len; n++)
    dest[n] = src[n];
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strndup(const uint8_t *src,
    uint64_t max_len)
{
  const uint64_t limit = poly_host_bound_4096(max_len);
  uint64_t len = 0;
  while (len < limit && src[len] != 0)
    len++;
  uint8_t *dest = (uint8_t *) poly_host_alloc_aligned(len + 1, 8);
  for (uint64_t n = 0; dest != 0 && n < len; n++)
    dest[n] = src[n];
  if (dest != 0)
    dest[len] = 0;
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcmp(const unsigned char *left_text,
    const unsigned char *right_text)
{
  int64_t cmp = 0;
  for (uint64_t n = 0; n < 4096; n++) {
    const unsigned char left = left_text[n];
    const unsigned char right = right_text[n];
    if (left != right || left == 0 || right == 0) {
      cmp = (int64_t) left - (int64_t) right;
      break;
    }
  }
  return (uint64_t) cmp;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strncmp(const unsigned char *left_text,
    const unsigned char *right_text, uint64_t max_len)
{
  const uint64_t count = poly_host_bound_4096(max_len);
  int64_t cmp = 0;
  for (uint64_t n = 0; n < count; n++) {
    const unsigned char left = left_text[n];
    const unsigned char right = right_text[n];
    if (left != right || left == 0 || right == 0) {
      cmp = (int64_t) left - (int64_t) right;
      break;
    }
  }
  return (uint64_t) cmp;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcasecmp(
    const unsigned char *left_text, const unsigned char *right_text)
{
  int64_t cmp = 0;
  for (uint64_t n = 0; n < 4096; n++) {
    const unsigned char left = poly_host_ascii_lower(left_text[n]);
    const unsigned char right = poly_host_ascii_lower(right_text[n]);
    if (left != right || left == 0 || right == 0) {
      cmp = (int64_t) left - (int64_t) right;
      break;
    }
  }
  return (uint64_t) cmp;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strncasecmp(
    const unsigned char *left_text, const unsigned char *right_text,
    uint64_t max_len)
{
  const uint64_t count = poly_host_bound_4096(max_len);
  int64_t cmp = 0;
  for (uint64_t n = 0; n < count; n++) {
    const unsigned char left = poly_host_ascii_lower(left_text[n]);
    const unsigned char right = poly_host_ascii_lower(right_text[n]);
    if (left != right || left == 0 || right == 0) {
      cmp = (int64_t) left - (int64_t) right;
      break;
    }
  }
  return (uint64_t) cmp;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memcpy(uint8_t *dest,
    const uint8_t *src, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = 0; n < count; n++)
    dest[n] = src[n];
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memmove(uint8_t *dest,
    const uint8_t *src, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  if (dest <= src) {
    for (uint64_t n = 0; n < count; n++)
      dest[n] = src[n];
  }
  else {
    for (uint64_t n = count; n > 0; n--)
      dest[n - 1] = src[n - 1];
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memset(uint8_t *dest, uint64_t value,
    uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = 0; n < count; n++)
    dest[n] = (uint8_t) value;
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memcmp(const unsigned char *left_text,
    const unsigned char *right_text, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  int64_t cmp = 0;
  for (uint64_t n = 0; n < count; n++) {
    const unsigned char left = left_text[n];
    const unsigned char right = right_text[n];
    if (left != right) {
      cmp = (int64_t) left - (int64_t) right;
      break;
    }
  }
  return (uint64_t) cmp;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memchr(const uint8_t *text,
    uint64_t needle, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = 0; n < count; n++) {
    if (text[n] == (uint8_t) needle)
      return (uint64_t) (uintptr_t) (text + n);
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strchr(const uint8_t *text,
    uint64_t needle)
{
  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t value = text[n];
    if (value == (uint8_t) needle)
      return (uint64_t) (uintptr_t) (text + n);
    if (value == 0)
      break;
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strrchr(const uint8_t *text,
    uint64_t needle)
{
  uint64_t result = 0;
  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t value = text[n];
    if (value == (uint8_t) needle)
      result = (uint64_t) (uintptr_t) (text + n);
    if (value == 0)
      break;
  }
  return result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strstr(const uint8_t *haystack,
    const uint8_t *needle_text)
{
  const uint8_t first = needle_text[0];
  if (first == 0)
    return (uint64_t) (uintptr_t) haystack;

  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t left = haystack[n];
    if (left == 0)
      break;
    if (left != first)
      continue;

    bool matched = true;
    uint64_t m = 1;
    for (; m < 4096 - n; m++) {
      const uint8_t needle = needle_text[m];
      if (needle == 0)
        break;
      const uint8_t value = haystack[n + m];
      if (value != needle || value == 0) {
        matched = false;
        break;
      }
    }
    if (m == 4096 - n)
      matched = false;
    if (matched)
      return (uint64_t) (uintptr_t) (haystack + n);
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcasestr(const uint8_t *haystack,
    const uint8_t *needle_text)
{
  const uint8_t first = poly_host_ascii_lower(needle_text[0]);
  if (first == 0)
    return (uint64_t) (uintptr_t) haystack;

  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t left = poly_host_ascii_lower(haystack[n]);
    if (left == 0)
      break;
    if (left != first)
      continue;

    bool matched = true;
    uint64_t m = 1;
    for (; m < 4096 - n; m++) {
      const uint8_t needle = poly_host_ascii_lower(needle_text[m]);
      if (needle == 0)
        break;
      const uint8_t value = poly_host_ascii_lower(haystack[n + m]);
      if (value != needle || value == 0) {
        matched = false;
        break;
      }
    }
    if (m == 4096 - n)
      matched = false;
    if (matched)
      return (uint64_t) (uintptr_t) (haystack + n);
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcpy(uint8_t *dest,
    const uint8_t *src)
{
  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t value = src[n];
    dest[n] = value;
    if (value == 0)
      break;
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strncpy(uint8_t *dest,
    const uint8_t *src, uint64_t max_len)
{
  const uint64_t count = poly_host_bound_4096(max_len);
  bool padding = false;
  for (uint64_t n = 0; n < count; n++) {
    uint8_t value = 0;
    if (!padding) {
      value = src[n];
      if (value == 0)
        padding = true;
    }
    dest[n] = value;
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcat(uint8_t *dest,
    const uint8_t *src)
{
  uint64_t dest_len = 0;
  while (dest_len < 4096 && dest[dest_len] != 0)
    dest_len++;
  for (uint64_t n = 0; dest_len + n < 4096; n++) {
    const uint8_t value = src[n];
    dest[dest_len + n] = value;
    if (value == 0)
      break;
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strncat(uint8_t *dest,
    const uint8_t *src, uint64_t max_len)
{
  uint64_t dest_len = 0;
  const uint64_t count = poly_host_bound_4096(max_len);
  while (dest_len < 4096 && dest[dest_len] != 0)
    dest_len++;
  uint64_t copied = 0;
  while (copied < count && dest_len + copied < 4095) {
    const uint8_t value = src[copied];
    if (value == 0)
      break;
    dest[dest_len + copied] = value;
    copied++;
  }
  if (dest_len + copied < 4096)
    dest[dest_len + copied] = 0;
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strspn(const uint8_t *text,
    const uint8_t *accept_text)
{
  uint64_t result = 0;
  for (; result < 4096; result++) {
    const uint8_t value = text[result];
    bool matched = false;
    if (value == 0)
      break;
    for (uint64_t n = 0; n < 4096; n++) {
      const uint8_t accept = accept_text[n];
      if (accept == 0)
        break;
      if (accept == value) {
        matched = true;
        break;
      }
    }
    if (!matched)
      break;
  }
  return result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strcspn(const uint8_t *text,
    const uint8_t *reject_text)
{
  uint64_t result = 0;
  for (; result < 4096; result++) {
    const uint8_t value = text[result];
    bool rejected = false;
    if (value == 0)
      break;
    for (uint64_t n = 0; n < 4096; n++) {
      const uint8_t reject = reject_text[n];
      if (reject == 0)
        break;
      if (reject == value) {
        rejected = true;
        break;
      }
    }
    if (rejected)
      break;
  }
  return result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strpbrk(const uint8_t *text,
    const uint8_t *accept_text)
{
  for (uint64_t offset = 0; offset < 4096; offset++) {
    const uint8_t value = text[offset];
    if (value == 0)
      break;
    for (uint64_t n = 0; n < 4096; n++) {
      const uint8_t accept = accept_text[n];
      if (accept == 0)
        break;
      if (accept == value)
        return (uint64_t) (uintptr_t) (text + offset);
    }
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_stpcpy(uint8_t *dest,
    const uint8_t *src)
{
  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t value = src[n];
    dest[n] = value;
    if (value == 0)
      return (uint64_t) (uintptr_t) (dest + n);
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_stpncpy(uint8_t *dest,
    const uint8_t *src, uint64_t max_len)
{
  const uint64_t count = poly_host_bound_4096(max_len);
  bool padding = false;
  uint8_t *result = dest + count;
  for (uint64_t n = 0; n < count; n++) {
    uint8_t value = 0;
    if (!padding) {
      value = src[n];
      if (value == 0) {
        padding = true;
        result = dest + n;
      }
    }
    dest[n] = value;
  }
  return (uint64_t) (uintptr_t) result;
}

uint64_t POLY_HOST_HELPER poly_host_x86_mempcpy(uint8_t *dest,
    const uint8_t *src, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = 0; n < count; n++)
    dest[n] = src[n];
  return (uint64_t) (uintptr_t) (dest + count);
}

uint64_t POLY_HOST_HELPER poly_host_x86_rawmemchr(const uint8_t *text,
    uint64_t needle)
{
  for (uint64_t n = 0; n < 4096; n++) {
    if (text[n] == (uint8_t) needle)
      return (uint64_t) (uintptr_t) (text + n);
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_strchrnul(const uint8_t *text,
    uint64_t needle)
{
  for (uint64_t n = 0; n < 4096; n++) {
    const uint8_t value = text[n];
    if (value == (uint8_t) needle || value == 0)
      return (uint64_t) (uintptr_t) (text + n);
  }
  return (uint64_t) (uintptr_t) text;
}

uint64_t POLY_HOST_HELPER poly_host_x86_bcopy(const uint8_t *src,
    uint8_t *dest, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  if (dest <= src) {
    for (uint64_t n = 0; n < count; n++)
      dest[n] = src[n];
  }
  else {
    for (uint64_t n = count; n > 0; n--)
      dest[n - 1] = src[n - 1];
  }
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_bzero(uint8_t *dest, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = 0; n < count; n++)
    dest[n] = 0;
  return (uint64_t) (uintptr_t) dest;
}

uint64_t POLY_HOST_HELPER poly_host_x86_stack_chk_fail(void)
{
  return (uint64_t) -5;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memrchr(const uint8_t *text,
    uint64_t needle, uint64_t size)
{
  const uint64_t count = poly_host_bound_4096(size);
  for (uint64_t n = count; n > 0; n--) {
    if (text[n - 1] == (uint8_t) needle)
      return (uint64_t) (uintptr_t) (text + n - 1);
  }
  return 0;
}

uint64_t POLY_HOST_HELPER poly_host_x86_memmem(const uint8_t *haystack,
    uint64_t haystack_size, const uint8_t *needle, uint64_t needle_size)
{
  const uint64_t haystack_len = poly_host_bound_4096(haystack_size);
  const uint64_t needle_len = poly_host_bound_4096(needle_size);
  if (needle_len == 0)
    return (uint64_t) (uintptr_t) haystack;
  if (needle_len > haystack_len)
    return 0;

  for (uint64_t n = 0; n <= haystack_len - needle_len; n++) {
    bool matched = true;
    for (uint64_t m = 0; m < needle_len; m++) {
      if (haystack[n + m] != needle[m]) {
        matched = false;
        break;
      }
    }
    if (matched)
      return (uint64_t) (uintptr_t) (haystack + n);
  }
  return 0;
}
