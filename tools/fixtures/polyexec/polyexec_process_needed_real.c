#include <stdint.h>

typedef uint32_t poly_u32x4 __attribute__((vector_size(16)));

struct poly_compact_u32_f32 {
  uint32_t i;
  float f;
};

struct poly_compact_f32_u32 {
  float f;
  uint32_t i;
};

union poly_float_bits {
  float f;
  uint32_t u;
};

enum {
  POLY_SYS_WRITE = 64,
  POLY_SYS_EXIT = 93,
  POLY_SYS_EXIT_GROUP = 94
};

static long poly_syscall3(long number, long arg0, long arg1, long arg2) {
#if defined(__aarch64__)
  register long x0 __asm__("x0") = arg0;
  register long x1 __asm__("x1") = arg1;
  register long x2 __asm__("x2") = arg2;
  register long x8 __asm__("x8") = number;
  __asm__ volatile("svc #0"
      : "+r"(x0)
      : "r"(x1), "r"(x2), "r"(x8)
      : "memory");
  return x0;
#elif defined(__riscv)
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a7 __asm__("a7") = number;
  __asm__ volatile("ecall"
      : "+r"(a0)
      : "r"(a1), "r"(a2), "r"(a7)
      : "memory");
  return a0;
#elif defined(__x86_64__)
  long x86_number = number;
  if (number == POLY_SYS_WRITE)
    x86_number = 1;
  else if (number == POLY_SYS_EXIT)
    x86_number = 60;
  else if (number == POLY_SYS_EXIT_GROUP)
    x86_number = 231;
  register long rax __asm__("rax") = x86_number;
  register long rdi __asm__("rdi") = arg0;
  register long rsi __asm__("rsi") = arg1;
  register long rdx __asm__("rdx") = arg2;
  __asm__ volatile("syscall"
      : "+a"(rax)
      : "r"(rdi), "r"(rsi), "r"(rdx)
      : "rcx", "r11", "memory");
  return rax;
#else
#error unsupported architecture
#endif
}

#if defined(POLY_PROCESS_NEEDED_LEAF)

uint64_t poly_process_needed_leaf_bias = 0x11;

__attribute__((visibility("default")))
uint64_t poly_process_needed_leaf(uint64_t left, uint64_t right) {
  return left + right + poly_process_needed_leaf_bias;
}

#elif defined(POLY_PROCESS_NEEDED_MID)

extern uint64_t poly_process_needed_leaf(uint64_t, uint64_t);

__attribute__((visibility("default")))
uint64_t poly_process_needed_mid(uint64_t left, uint64_t right) {
  return poly_process_needed_leaf(left, right) + 0x22;
}

#elif defined(POLY_PROCESS_NEEDED_DEP)

uint64_t poly_process_needed_bias = 0x40;

__attribute__((visibility("default")))
uint64_t poly_process_needed_add(uint64_t left, uint64_t right) {
  return left + right + poly_process_needed_bias;
}

#elif defined(POLY_PROCESS_PRELOAD_OVERRIDE_DEP)

__attribute__((visibility("default")))
uint64_t poly_process_needed_add(uint64_t left, uint64_t right) {
  return left + right + 0x140;
}

#elif defined(POLY_PROCESS_PRELOAD_SECOND_OVERRIDE_DEP)

__attribute__((visibility("default")))
uint64_t poly_process_needed_add(uint64_t left, uint64_t right) {
  return left + right + 0x240;
}

#elif defined(POLY_PROCESS_NEEDED_IFUNC_DEP)

static uint64_t poly_process_needed_ifunc_add_impl(uint64_t left,
    uint64_t right) {
  return left + right + 0x40;
}

static void *poly_process_needed_ifunc_add_resolver(void) {
  return poly_process_needed_ifunc_add_impl;
}

__attribute__((visibility("default")))
uint64_t poly_process_needed_ifunc_add(uint64_t, uint64_t)
    __attribute__((ifunc("poly_process_needed_ifunc_add_resolver")));

#elif defined(POLY_PROCESS_NEEDED_FP64_DEP)

#if defined(__x86_64__)
__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_process_needed_fp64 fp64\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");
#endif

__attribute__((visibility("default")))
double poly_process_needed_fp64(double left, double right) {
  return left + right + 4.0;
}

#elif defined(POLY_PROCESS_NEEDED_FP32_DEP)

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_process_needed_fp32 fp32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
float poly_process_needed_fp32(float left, float right) {
  return left + right + 4.0f;
}

#elif defined(POLY_PROCESS_NEEDED_VEC128_DEP)

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_process_needed_vec128 vec128_u32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
poly_u32x4 poly_process_needed_vec128(poly_u32x4 left, poly_u32x4 right) {
  const poly_u32x4 bias = { 100, 200, 300, 400 };
  return left + right + bias;
}

#elif defined(POLY_PROCESS_NEEDED_COMPACT_DEP)

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_process_needed_compact_u32_f32 compact_u32_f32\\n\"\n"
  "   .ascii \"poly_process_needed_compact_f32_u32 compact_f32_u32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_process_needed_compact_u32_f32(
    struct poly_compact_u32_f32 in, uint32_t scale) {
  struct poly_compact_u32_f32 out;
  out.i = in.i + scale + 100;
  out.f = in.f + (float) scale + 25.0f;
  return out;
}

__attribute__((visibility("default")))
struct poly_compact_f32_u32 poly_process_needed_compact_f32_u32(
    struct poly_compact_f32_u32 in, uint32_t scale) {
  struct poly_compact_f32_u32 out;
  out.f = in.f + (float) scale + 50.0f;
  out.i = in.i + scale + 200;
  return out;
}

#elif defined(POLY_PROCESS_NEEDED_STACK9_DEP)

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_process_needed_sum9 u64_stack9\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
uint64_t poly_process_needed_sum9(uint64_t a0, uint64_t a1, uint64_t a2,
    uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7,
    uint64_t a8) {
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + 0x90;
}

#elif defined(POLY_PROCESS_ROOT_EXPORT_DEP)

extern uint64_t poly_process_root_export(uint64_t);

__attribute__((visibility("default")))
uint64_t poly_process_root_export_dep(uint64_t value) {
  return poly_process_root_export(value) + 0x33;
}

#elif defined(POLY_PROCESS_ROOT_IFUNC_DEP)

extern uint64_t poly_process_root_ifunc(uint64_t);

__attribute__((visibility("default")))
uint64_t poly_process_root_ifunc_dep(uint64_t value) {
  return poly_process_root_ifunc(value) + 0x55;
}

#elif defined(POLY_PROCESS_WEAK_DEP)

extern uint64_t poly_process_missing_weak_value __attribute__((weak));
extern uint64_t poly_process_missing_weak_add(uint64_t) __attribute__((weak));

__attribute__((visibility("default")))
uint64_t poly_process_weak_dep(void) {
  uint64_t result = 0x88;
  if (&poly_process_missing_weak_value)
    result += poly_process_missing_weak_value;
  if (poly_process_missing_weak_add)
    result += poly_process_missing_weak_add(0x10);
  return result;
}

#elif defined(POLY_PROCESS_INIT_DEP)

uint64_t poly_process_init_dep_value;

static void poly_process_init_dep_ctor(void) __attribute__((constructor));
static void poly_process_init_dep_ctor(void) {
  poly_process_init_dep_value = 0x5a;
}

__attribute__((visibility("default")))
uint64_t poly_process_init_dep(void) {
  return poly_process_init_dep_value;
}

#elif defined(POLY_PROCESS_DT_INIT_DEP)

uint64_t poly_process_dt_init_dep_value;

void poly_process_dt_init_dep_ctor(void) {
  poly_process_dt_init_dep_value = 0x6c;
}

__attribute__((visibility("default")))
uint64_t poly_process_dt_init_dep(void) {
  return poly_process_dt_init_dep_value;
}

#elif defined(POLY_PROCESS_FINI_DEP)

uint64_t poly_process_fini_dep_value = 0x72;

static void poly_process_fini_dep_dtor(void) __attribute__((destructor));
static void poly_process_fini_dep_dtor(void) {
#if defined(__aarch64__)
  static const char marker[] = "POLY_PROCESS_AARCH64_DEP_FINI_ARRAY_OK\n";
#elif defined(__riscv)
  static const char marker[] = "POLY_PROCESS_RISCV_DEP_FINI_ARRAY_OK\n";
#endif
  (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
    sizeof(marker) - 1);
}

__attribute__((visibility("default")))
uint64_t poly_process_fini_dep(void) {
  return poly_process_fini_dep_value;
}

#elif defined(POLY_PROCESS_DT_FINI_DEP)

uint64_t poly_process_dt_fini_dep_value = 0x84;

void poly_process_dt_fini_dep_dtor(void) {
#if defined(__aarch64__)
  static const char marker[] = "POLY_PROCESS_AARCH64_DEP_DT_FINI_OK\n";
#elif defined(__riscv)
  static const char marker[] = "POLY_PROCESS_RISCV_DEP_DT_FINI_OK\n";
#endif
  (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
    sizeof(marker) - 1);
}

__attribute__((visibility("default")))
uint64_t poly_process_dt_fini_dep(void) {
  return poly_process_dt_fini_dep_value;
}

#elif defined(POLY_PROCESS_VERSIONED_DEP)

uint64_t poly_process_versioned_add_v1(uint64_t left, uint64_t right) {
  return left + right + 0x100;
}

uint64_t poly_process_versioned_add_v2(uint64_t left, uint64_t right) {
  return left + right + 0x200;
}

__asm__(".symver poly_process_versioned_add_v1, "
        "poly_process_versioned_add@POLYPROC_1.0");
__asm__(".symver poly_process_versioned_add_v2, "
        "poly_process_versioned_add@@POLYPROC_2.0");

#elif defined(POLY_PROCESS_TLS_DEP)

__thread uint64_t poly_process_tls_dep_counter
    __attribute__((tls_model("initial-exec"), visibility("default"))) = 0x70;

__attribute__((visibility("default")))
uint64_t poly_process_tls_dep_add(uint64_t left, uint64_t right) {
  poly_process_tls_dep_counter += left + right;
  return poly_process_tls_dep_counter;
}

#elif defined(POLY_PROCESS_TLS_DEFAULT_DEP)

__thread uint64_t poly_process_tls_default_dep_counter
    __attribute__((visibility("default"))) = 0x90;

__attribute__((visibility("default")))
uint64_t poly_process_tls_default_dep_add(uint64_t left, uint64_t right) {
  poly_process_tls_default_dep_counter += left + right;
  return poly_process_tls_default_dep_counter;
}

#elif defined(POLY_PROCESS_TLS_TRAD_DEP)

__thread uint64_t poly_process_tls_trad_dep_counter
    __attribute__((visibility("default"))) = 0xa0;

__attribute__((visibility("default")))
uint64_t poly_process_tls_trad_dep_add(uint64_t left, uint64_t right) {
  poly_process_tls_trad_dep_counter += left + right;
  return poly_process_tls_trad_dep_counter;
}

#elif defined(POLY_PROCESS_COPY_DEP)

__attribute__((visibility("default")))
uint64_t poly_process_copy_value = 0x123456789abcdef0ULL;

__attribute__((visibility("default")))
uint64_t poly_process_copy_ping(void) {
  return poly_process_copy_value;
}

#else

#if defined(POLY_PROCESS_NEEDED_INDIRECT_MAIN)
extern uint64_t poly_process_needed_leaf(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_ROOT_EXPORT_MAIN)
extern uint64_t poly_process_root_export_dep(uint64_t);
#elif defined(POLY_PROCESS_ROOT_IFUNC_MAIN)
extern uint64_t poly_process_root_ifunc_dep(uint64_t);
#elif defined(POLY_PROCESS_WEAK_MAIN)
extern uint64_t poly_process_missing_weak_value __attribute__((weak));
extern uint64_t poly_process_missing_weak_add(uint64_t) __attribute__((weak));
#elif defined(POLY_PROCESS_WEAK_DEP_MAIN)
extern uint64_t poly_process_weak_dep(void);
#elif defined(POLY_PROCESS_INIT_DEP_MAIN)
extern uint64_t poly_process_init_dep(void);
#elif defined(POLY_PROCESS_DT_INIT_DEP_MAIN)
extern uint64_t poly_process_dt_init_dep(void);
#elif defined(POLY_PROCESS_FINI_DEP_MAIN)
extern uint64_t poly_process_fini_dep(void);
#elif defined(POLY_PROCESS_DT_FINI_DEP_MAIN)
extern uint64_t poly_process_dt_fini_dep(void);
#elif defined(POLY_PROCESS_VERSIONED_MAIN)
extern uint64_t poly_process_versioned_add_v1(uint64_t, uint64_t);
__asm__(".symver poly_process_versioned_add_v1, "
        "poly_process_versioned_add@POLYPROC_1.0");
#elif defined(POLY_PROCESS_TLS_DEP_MAIN)
extern __thread uint64_t poly_process_tls_dep_counter
    __attribute__((tls_model("initial-exec")));
extern uint64_t poly_process_tls_dep_add(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_TLS_DEFAULT_DEP_MAIN)
extern __thread uint64_t poly_process_tls_default_dep_counter;
extern uint64_t poly_process_tls_default_dep_add(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_TLS_TRAD_DEP_MAIN)
extern __thread uint64_t poly_process_tls_trad_dep_counter;
extern uint64_t poly_process_tls_trad_dep_add(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_COPY_MAIN)
extern uint64_t poly_process_copy_value;
#elif defined(POLY_PROCESS_SONAME_ONCE_MAIN)
extern uint64_t poly_soname_once_a(void);
extern uint64_t poly_soname_once_b(void);
#elif defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
extern uint64_t poly_process_needed_mid(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_NEEDED_IFUNC_MAIN)
extern uint64_t poly_process_needed_ifunc_add(uint64_t, uint64_t);
#elif defined(POLY_PROCESS_NEEDED_FP64_MAIN)
extern double poly_process_needed_fp64(double, double);
#elif defined(POLY_PROCESS_NEEDED_FP32_MAIN)
extern float poly_process_needed_fp32(float, float);
#elif defined(POLY_PROCESS_NEEDED_VEC128_MAIN)
extern poly_u32x4 poly_process_needed_vec128(poly_u32x4, poly_u32x4);
#elif defined(POLY_PROCESS_NEEDED_COMPACT_MAIN)
extern struct poly_compact_u32_f32 poly_process_needed_compact_u32_f32(
    struct poly_compact_u32_f32, uint32_t);
extern struct poly_compact_f32_u32 poly_process_needed_compact_f32_u32(
    struct poly_compact_f32_u32, uint32_t);
#elif defined(POLY_PROCESS_NEEDED_STACK9_MAIN)
extern uint64_t poly_process_needed_sum9(uint64_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
#else
extern uint64_t poly_process_needed_add(uint64_t, uint64_t);
#endif

#if defined(POLY_PROCESS_ROOT_EXPORT_MAIN)
__attribute__((visibility("default")))
uint64_t poly_process_root_export(uint64_t value) {
  return value + 0x44;
}
#endif

#if defined(POLY_PROCESS_TLS_MAIN)
__thread uint64_t poly_process_tls_counter
    __attribute__((tls_model("initial-exec"))) = 0x31;
#endif

#if defined(POLY_PROCESS_TLS_DEFAULT_MAIN)
__thread uint64_t poly_process_tls_default_counter = 0x41;
#endif

#if defined(POLY_PROCESS_TLS_TRAD_MAIN)
__thread uint64_t poly_process_tls_trad_counter = 0x51;
#endif

#if defined(POLY_PROCESS_ROOT_IFUNC_MAIN)
static uint64_t poly_process_root_ifunc_impl(uint64_t value) {
  return value + 0x66;
}

static void *poly_process_root_ifunc_resolver(void) {
  return poly_process_root_ifunc_impl;
}

__attribute__((visibility("default")))
uint64_t poly_process_root_ifunc(uint64_t)
    __attribute__((ifunc("poly_process_root_ifunc_resolver")));
#endif

#if defined(POLY_PROCESS_INIT_MAIN)
static uint64_t poly_process_init_value;

static void poly_process_init_ctor(void) __attribute__((constructor));
static void poly_process_init_ctor(void) {
  poly_process_init_value = 0x7b;
}
#endif

#if defined(POLY_PROCESS_PREINIT_MAIN)
extern uint64_t poly_process_needed_add(uint64_t, uint64_t);

static uint64_t poly_process_preinit_value;

static void poly_process_preinit_ctor(void) {
  poly_process_preinit_value = poly_process_needed_add(0x20, 0x30) + 0x10;
}

static void (*const poly_process_preinit_entry)(void)
    __attribute__((section(".preinit_array"), used)) =
  poly_process_preinit_ctor;
#endif

#if defined(POLY_PROCESS_DT_INIT_MAIN)
static uint64_t poly_process_dt_init_value;

void poly_process_dt_init_root(void) {
  poly_process_dt_init_value = 0x8d;
}
#endif

#if defined(POLY_PROCESS_FINI_MAIN) || \
    defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
static void poly_process_fini_dtor(void) __attribute__((destructor));
static void poly_process_fini_dtor(void) {
#if defined(__aarch64__)
#if defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  static const char marker[] = "POLY_PROCESS_AARCH64_FINI_EXIT_GROUP_OK\n";
#else
  static const char marker[] = "POLY_PROCESS_AARCH64_FINI_ARRAY_OK\n";
#endif
#elif defined(__riscv)
#if defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  static const char marker[] = "POLY_PROCESS_RISCV_FINI_EXIT_GROUP_OK\n";
#else
  static const char marker[] = "POLY_PROCESS_RISCV_FINI_ARRAY_OK\n";
#endif
#endif
  (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
    sizeof(marker) - 1);
}
#endif

#if defined(POLY_PROCESS_FINI_ORDER_MAIN)
static uint64_t poly_process_fini_order_state;

static void poly_process_fini_order_first(void) {
  if (poly_process_fini_order_state == 2) {
#if defined(__aarch64__)
    static const char marker[] = "POLY_PROCESS_AARCH64_FINI_ORDER_OK\n";
#elif defined(__riscv)
    static const char marker[] = "POLY_PROCESS_RISCV_FINI_ORDER_OK\n";
#endif
    (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
      sizeof(marker) - 1);
  } else {
    static const char marker[] = "POLY_PROCESS_FINI_ORDER_FAIL\n";
    (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
      sizeof(marker) - 1);
  }
}

static void poly_process_fini_order_second(void) {
  poly_process_fini_order_state =
    poly_process_fini_order_state == 0 ? 2 : 99;
}

static void (*const poly_process_fini_order_entries[])(void)
    __attribute__((section(".fini_array"), used)) = {
  poly_process_fini_order_first,
  poly_process_fini_order_second,
};
#endif

#if defined(POLY_PROCESS_DT_FINI_MAIN)
void poly_process_dt_fini_root(void) {
#if defined(__aarch64__)
  static const char marker[] = "POLY_PROCESS_AARCH64_DT_FINI_OK\n";
#elif defined(__riscv)
  static const char marker[] = "POLY_PROCESS_RISCV_DT_FINI_OK\n";
#endif
  (void) poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
    sizeof(marker) - 1);
}
#endif

uint64_t poly_process_main(void) {
#if defined(POLY_PROCESS_NEEDED_INDIRECT_MAIN)
  if (poly_process_needed_leaf(0x12, 0x23) != 0x46)
    return 24;
  static const char marker[] = "POLY_PROCESS_INDIRECT_NEEDED_OK\n";
#elif defined(POLY_PROCESS_ROOT_EXPORT_MAIN)
  if (poly_process_root_export_dep(0x22) != 0x99)
    return 25;
  static const char marker[] = "POLY_PROCESS_ROOT_EXPORT_NEEDED_OK\n";
#elif defined(POLY_PROCESS_ROOT_IFUNC_MAIN)
  if (poly_process_root_ifunc_dep(0x11) != 0xcc)
    return 26;
  static const char marker[] = "POLY_PROCESS_ROOT_IFUNC_NEEDED_OK\n";
#elif defined(POLY_PROCESS_WEAK_MAIN)
  if (&poly_process_missing_weak_value || poly_process_missing_weak_add)
    return 27;
  static const char marker[] = "POLY_PROCESS_WEAK_UNRESOLVED_OK\n";
#elif defined(POLY_PROCESS_WEAK_DEP_MAIN)
  if (poly_process_weak_dep() != 0x88)
    return 28;
  static const char marker[] = "POLY_PROCESS_WEAK_DEP_UNRESOLVED_OK\n";
#elif defined(POLY_PROCESS_INIT_MAIN)
  if (poly_process_init_value != 0x7b)
    return 29;
  static const char marker[] = "POLY_PROCESS_INIT_ARRAY_OK\n";
#elif defined(POLY_PROCESS_PREINIT_MAIN)
  if (poly_process_preinit_value != 0xa0)
    return 46;
  static const char marker[] = "POLY_PROCESS_PREINIT_ARRAY_OK\n";
#elif defined(POLY_PROCESS_INIT_DEP_MAIN)
  if (poly_process_init_dep() != 0x5a)
    return 30;
  static const char marker[] = "POLY_PROCESS_DEP_INIT_ARRAY_OK\n";
#elif defined(POLY_PROCESS_DT_INIT_MAIN)
  if (poly_process_dt_init_value != 0x8d)
    return 31;
  static const char marker[] = "POLY_PROCESS_DT_INIT_OK\n";
#elif defined(POLY_PROCESS_DT_INIT_DEP_MAIN)
  if (poly_process_dt_init_dep() != 0x6c)
    return 32;
  static const char marker[] = "POLY_PROCESS_DEP_DT_INIT_OK\n";
#elif defined(POLY_PROCESS_FINI_DEP_MAIN)
  if (poly_process_fini_dep() != 0x72)
    return 48;
  static const char marker[] = "POLY_PROCESS_DEP_FINI_MAIN_OK\n";
#elif defined(POLY_PROCESS_DT_FINI_DEP_MAIN)
  if (poly_process_dt_fini_dep() != 0x84)
    return 49;
  static const char marker[] = "POLY_PROCESS_DEP_DT_FINI_MAIN_OK\n";
#elif defined(POLY_PROCESS_FINI_MAIN)
  static const char marker[] = "POLY_PROCESS_FINI_MAIN_OK\n";
#elif defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  static const char marker[] = "POLY_PROCESS_FINI_EXIT_GROUP_MAIN_OK\n";
#elif defined(POLY_PROCESS_FINI_ORDER_MAIN)
  static const char marker[] = "POLY_PROCESS_FINI_ORDER_MAIN_OK\n";
#elif defined(POLY_PROCESS_DT_FINI_MAIN)
  static const char marker[] = "POLY_PROCESS_DT_FINI_MAIN_OK\n";
#elif defined(POLY_PROCESS_VERSIONED_MAIN)
  if (poly_process_versioned_add_v1(0x20, 0x30) != 0x150)
    return 33;
  static const char marker[] = "POLY_PROCESS_VERSIONED_NEEDED_OK\n";
#elif defined(POLY_PROCESS_TLS_MAIN)
  poly_process_tls_counter += 0x11;
  if (poly_process_tls_counter != 0x42)
    return 34;
  static const char marker[] = "POLY_PROCESS_TLS_OK\n";
#elif defined(POLY_PROCESS_TLS_DEP_MAIN)
  const uint64_t before = poly_process_tls_dep_counter;
  const uint64_t after = poly_process_tls_dep_add(0x12, 0x13);
  if (before != 0x70 || after != 0x95 ||
      poly_process_tls_dep_counter != 0x95)
    return 35;
  static const char marker[] = "POLY_PROCESS_DEP_TLS_OK\n";
#elif defined(POLY_PROCESS_TLS_DEFAULT_MAIN)
  poly_process_tls_default_counter += 0x11;
  if (poly_process_tls_default_counter != 0x52)
    return 36;
  static const char marker[] = "POLY_PROCESS_TLS_DEFAULT_OK\n";
#elif defined(POLY_PROCESS_TLS_DEFAULT_DEP_MAIN)
  const uint64_t before = poly_process_tls_default_dep_counter;
  const uint64_t after = poly_process_tls_default_dep_add(0x21, 0x22);
  if (before != 0x90 || after != 0xd3 ||
      poly_process_tls_default_dep_counter != 0xd3)
    return 37;
  static const char marker[] = "POLY_PROCESS_DEP_TLS_DEFAULT_OK\n";
#elif defined(POLY_PROCESS_TLS_TRAD_MAIN)
  poly_process_tls_trad_counter += 0x11;
  if (poly_process_tls_trad_counter != 0x62)
    return 39;
  static const char marker[] = "POLY_PROCESS_TLS_TRAD_OK\n";
#elif defined(POLY_PROCESS_TLS_TRAD_DEP_MAIN)
  const uint64_t before = poly_process_tls_trad_dep_counter;
  const uint64_t after = poly_process_tls_trad_dep_add(0x31, 0x32);
  if (before != 0xa0 || after != 0x103 ||
      poly_process_tls_trad_dep_counter != 0x103)
    return 40;
  static const char marker[] = "POLY_PROCESS_DEP_TLS_TRAD_OK\n";
#elif defined(POLY_PROCESS_COPY_MAIN)
  if (poly_process_copy_value != 0x123456789abcdef0ULL)
    return 41;
  poly_process_copy_value += 0x10;
  if (poly_process_copy_value != 0x123456789abcdf00ULL)
    return 43;
  static const char marker[] = "POLY_PROCESS_COPY_RELOC_OK\n";
#elif defined(POLY_PROCESS_SONAME_ONCE_MAIN)
  if (poly_soname_once_a() + poly_soname_once_b() + 5 != 235)
    return 47;
  static const char marker[] = "POLY_PROCESS_SONAME_ONCE_OK\n";
#elif defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
  if (poly_process_needed_mid(0x10, 0x20) != 0x63)
    return 23;
  static const char marker[] = "POLY_PROCESS_TRANSITIVE_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_IFUNC_MAIN)
  if (poly_process_needed_ifunc_add(0x20, 0x30) != 0x90)
    return 38;
  static const char marker[] = "POLY_PROCESS_IFUNC_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_FP64_MAIN)
  if (poly_process_needed_fp64(1.5, 2.25) != 7.75)
    return 50;
  static const char marker[] = "POLY_PROCESS_CROSS_FP64_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_FP32_MAIN)
  const union poly_float_bits result = {
    .f = poly_process_needed_fp32(1.5f, 2.25f)
  };
  if (result.u != 0x40f80000U)
    return 54;
  static const char marker[] = "POLY_PROCESS_CROSS_FP32_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_VEC128_MAIN)
  union {
    poly_u32x4 v;
    uint32_t u[4];
  } a = { .u = { 1, 2, 3, 4 } };
  union {
    poly_u32x4 v;
    uint32_t u[4];
  } b = { .u = { 10, 20, 30, 40 } };
  union {
    poly_u32x4 v;
    uint32_t u[4];
  } result;
  result.v = poly_process_needed_vec128(a.v, b.v);
  if (result.u[0] != 111 || result.u[1] != 222 ||
      result.u[2] != 333 || result.u[3] != 444)
    return 51;
  static const char marker[] = "POLY_PROCESS_CROSS_VEC128_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_COMPACT_MAIN)
  const struct poly_compact_u32_f32 a = { 7, 2.5f };
  const struct poly_compact_f32_u32 b = { 1.25f, 6 };
  const struct poly_compact_u32_f32 ra =
    poly_process_needed_compact_u32_f32(a, 5);
  const struct poly_compact_f32_u32 rb =
    poly_process_needed_compact_f32_u32(b, 8);
  const union poly_float_bits fa = { .f = ra.f };
  const union poly_float_bits fb = { .f = rb.f };
  if (ra.i != 112 || fa.u != 0x42020000U ||
      rb.i != 214 || fb.u != 0x426d0000U)
    return 52;
  static const char marker[] = "POLY_PROCESS_CROSS_COMPACT_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_STACK9_MAIN)
  if (poly_process_needed_sum9(1, 2, 3, 4, 5, 6, 7, 8, 9) != 0xbd)
    return 53;
  static const char marker[] = "POLY_PROCESS_CROSS_STACK9_NEEDED_OK\n";
#elif defined(POLY_PROCESS_PRELOAD_MAIN)
  if (poly_process_needed_add(0x20, 0x30) != 0x190)
    return 44;
  static const char marker[] = "POLY_PROCESS_PRELOAD_OK\n";
#elif defined(POLY_PROCESS_PRELOAD_SECOND_MAIN)
  if (poly_process_needed_add(0x20, 0x30) != 0x290)
    return 45;
  static const char marker[] = "POLY_PROCESS_PRELOAD_SECOND_OK\n";
#else
  if (poly_process_needed_add(0x20, 0x30) != 0x90)
    return 21;
  static const char marker[] = "POLY_PROCESS_NEEDED_OK\n";
#endif
  if (poly_syscall3(POLY_SYS_WRITE, 1, (long) marker,
        sizeof(marker) - 1) != (long) sizeof(marker) - 1)
    return 22;

  return 42;
}

#if defined(__aarch64__)
__asm__(
  ".global _start\n"
  ".type _start, %function\n"
  "_start:\n"
  "bl poly_process_main\n"
#if defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  "mov x8, #94\n"
#else
  "mov x8, #93\n"
#endif
  "svc #0\n");
#elif defined(__riscv)
__asm__(
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "call poly_process_main\n"
#if defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  "li a7, 94\n"
#else
  "li a7, 93\n"
#endif
  "ecall\n");
#elif defined(__x86_64__)
__asm__(
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "call poly_process_main\n"
  "mov %eax, %edi\n"
#if defined(POLY_PROCESS_FINI_EXIT_GROUP_MAIN)
  "mov $231, %rax\n"
#else
  "mov $60, %rax\n"
#endif
  "syscall\n");
#else
#error unsupported architecture
#endif

#endif
