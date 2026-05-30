extern unsigned long getauxval(unsigned long);

static int poly_streq(const char *left, const char *right)
{
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == *right;
}

static int poly_contains(const char *haystack, const char *needle)
{
  if (!*needle)
    return 1;
  for (; *haystack; haystack++) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n && *h == *n) {
      h++;
      n++;
    }
    if (!*n)
      return 1;
  }
  return 0;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  const char *platform = (const char *) getauxval(15);
  if (!platform)
    return 9000;
#if defined(__aarch64__)
  if (!poly_streq(platform, "aarch64"))
    return 9001;
#elif defined(__riscv)
  if (!poly_streq(platform, "riscv64"))
    return 9002;
#else
#error unsupported architecture
#endif
  if (getauxval(23) != 0)
    return 9003;
  const char *execfn = (const char *) getauxval(31);
  if (!execfn)
    return 9004;
  if (!poly_contains(execfn, "pcall-getauxval-real.so"))
    return 9005;

  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    getauxval(6) + getauxval(17) + getauxval(16) + getauxval(26) +
    getauxval(0xdead) + 31;
}
