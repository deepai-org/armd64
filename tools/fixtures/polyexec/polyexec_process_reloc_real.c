#include <stdint.h>

enum {
  POLY_SYS_WRITE = 64,
  POLY_SYS_EXIT = 93
};

uint64_t poly_reloc_seed = 0x1234000000000000ULL;
uint64_t poly_reloc_delta = 0x5678ULL;
uint64_t *poly_reloc_seed_ptr = &poly_reloc_seed;
uint64_t *poly_reloc_delta_ptr = &poly_reloc_delta;
const char poly_reloc_arg[] = "reloc";
const char *poly_reloc_arg_ptr = poly_reloc_arg;

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

static int poly_streq(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

uint64_t poly_reloc_helper(uint64_t value) {
  return value + *poly_reloc_seed_ptr + *poly_reloc_delta_ptr;
}

__attribute__((visibility("default")))
uint64_t poly_reloc_interposable(uint64_t value) {
  return value + 0x42;
}

uint64_t (*poly_reloc_helper_ptr)(uint64_t) = poly_reloc_helper;

uint64_t poly_process_main(uint64_t *initial_sp) {
  uint64_t argc = initial_sp[0];
  char **argv = (char **) &initial_sp[1];

  if (argc != 2)
    return 10 + argc;
  if (!argv[1] || !poly_streq(argv[1], poly_reloc_arg_ptr))
    return 20;

  uint64_t result = poly_reloc_helper_ptr(0x9abcULL);
  if (result != 0x123400000000f134ULL)
    return 21;
  if (poly_reloc_interposable(0x55) != 0x97)
    return 23;

  static const char marker[] = "POLY_PROCESS_RELOC_OK\n";
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
  "mov x0, sp\n"
  "bl poly_process_main\n"
  "mov x8, #93\n"
  "svc #0\n");
#elif defined(__riscv)
__asm__(
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "mv a0, sp\n"
  "call poly_process_main\n"
  "li a7, 93\n"
  "ecall\n");
#else
#error unsupported architecture
#endif
