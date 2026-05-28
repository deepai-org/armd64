#include <stdint.h>

enum {
  POLY_SYS_WRITE = 64,
  POLY_SYS_EXIT = 93
};

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
#elif defined(POLY_PROCESS_VERSIONED_MAIN)
extern uint64_t poly_process_versioned_add_v1(uint64_t, uint64_t);
__asm__(".symver poly_process_versioned_add_v1, "
        "poly_process_versioned_add@POLYPROC_1.0");
#elif defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
extern uint64_t poly_process_needed_mid(uint64_t, uint64_t);
#else
extern uint64_t poly_process_needed_add(uint64_t, uint64_t);
#endif

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
#else
#error unsupported architecture
#endif
}

#if defined(POLY_PROCESS_ROOT_EXPORT_MAIN)
__attribute__((visibility("default")))
uint64_t poly_process_root_export(uint64_t value) {
  return value + 0x44;
}
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

#if defined(POLY_PROCESS_DT_INIT_MAIN)
static uint64_t poly_process_dt_init_value;

void poly_process_dt_init_root(void) {
  poly_process_dt_init_value = 0x8d;
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
#elif defined(POLY_PROCESS_VERSIONED_MAIN)
  if (poly_process_versioned_add_v1(0x20, 0x30) != 0x150)
    return 33;
  static const char marker[] = "POLY_PROCESS_VERSIONED_NEEDED_OK\n";
#elif defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
  if (poly_process_needed_mid(0x10, 0x20) != 0x63)
    return 23;
  static const char marker[] = "POLY_PROCESS_TRANSITIVE_NEEDED_OK\n";
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
  "mov x8, #93\n"
  "svc #0\n");
#elif defined(__riscv)
__asm__(
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "call poly_process_main\n"
  "li a7, 93\n"
  "ecall\n");
#else
#error unsupported architecture
#endif

#endif
