#include <stdint.h>

static int poly_streq(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

static int poly_contains(const char *text, const char *needle) {
  if (!*needle)
    return 1;
  for (; *text; text++) {
    const char *t = text;
    const char *n = needle;
    while (*t && *n && *t == *n) {
      t++;
      n++;
    }
    if (!*n)
      return 1;
  }
  return 0;
}

static int poly_env_has(char **envp, const char *entry) {
  for (; *envp; envp++) {
    if (poly_streq(*envp, entry))
      return 1;
  }
  return 0;
}

uint64_t poly_process_main(uint64_t *initial_sp) {
  uint64_t argc = initial_sp[0];
  char **argv = (char **) &initial_sp[1];
  char **envp = argv + argc + 1;

  if (argc != 3)
    return 10 + argc;
  if (!argv[0] || !poly_contains(argv[0], "process-argv-envp"))
    return 20;
  if (!argv[1] || !poly_streq(argv[1], "alpha"))
    return 21;
  if (!argv[2] || !poly_streq(argv[2], "beta"))
    return 22;
  if (argv[3] != 0)
    return 23;
  if (!poly_env_has(envp, "POLY_PROCESS_ENV=present"))
    return 24;

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
