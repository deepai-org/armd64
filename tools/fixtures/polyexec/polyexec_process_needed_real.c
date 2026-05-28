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

#else

#if defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
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

uint64_t poly_process_main(void) {
#if defined(POLY_PROCESS_NEEDED_TRANSITIVE_MAIN)
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
