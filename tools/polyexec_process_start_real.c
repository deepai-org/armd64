#include <stdint.h>

enum {
  POLY_AT_NULL = 0,
  POLY_AT_PHDR = 3,
  POLY_AT_PHENT = 4,
  POLY_AT_PHNUM = 5,
  POLY_AT_PAGESZ = 6,
  POLY_AT_ENTRY = 9,
  POLY_AT_RANDOM = 25,
  POLY_AT_EXECFN = 31,
  POLY_AT_PLATFORM = 15
};

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

static uint64_t *poly_auxv_after_env(char **envp) {
  while (*envp)
    envp++;
  return (uint64_t *) (envp + 1);
}

static uint64_t poly_aux_get(uint64_t *auxv, uint64_t type) {
  for (; auxv[0] != POLY_AT_NULL; auxv += 2) {
    if (auxv[0] == type)
      return auxv[1];
  }
  return 0;
}

static int poly_random_nonzero(const unsigned char *bytes) {
  unsigned char accum = 0;
  for (unsigned n = 0; n < 16; n++)
    accum |= bytes[n];
  return accum != 0;
}

uint64_t poly_process_main(uint64_t *initial_sp) {
  uint64_t argc = initial_sp[0];
  char **argv = (char **) &initial_sp[1];
  char **envp = argv + argc + 1;
  uint64_t *auxv = poly_auxv_after_env(envp);
  const char *platform =
#if defined(__aarch64__)
    "aarch64";
#elif defined(__riscv)
    "riscv64";
#else
    "";
#endif

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
  if (poly_aux_get(auxv, POLY_AT_PAGESZ) != 4096)
    return 25;
  if (poly_aux_get(auxv, POLY_AT_PHENT) != 56)
    return 26;
  if (poly_aux_get(auxv, POLY_AT_PHNUM) == 0)
    return 27;
  if (poly_aux_get(auxv, POLY_AT_PHDR) == 0)
    return 28;
  if (poly_aux_get(auxv, POLY_AT_ENTRY) == 0)
    return 29;
  if (!poly_aux_get(auxv, POLY_AT_RANDOM) ||
      !poly_random_nonzero((const unsigned char *)
        poly_aux_get(auxv, POLY_AT_RANDOM)))
    return 30;
  if (!poly_aux_get(auxv, POLY_AT_EXECFN) ||
      !poly_contains((const char *) poly_aux_get(auxv, POLY_AT_EXECFN),
        "process-argv-envp"))
    return 31;
  if (!poly_aux_get(auxv, POLY_AT_PLATFORM) ||
      !poly_streq((const char *) poly_aux_get(auxv, POLY_AT_PLATFORM),
        platform))
    return 32;

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
