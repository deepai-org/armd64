static volatile unsigned long seed = 9;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned long x =
    (a << 32) ^ (b * 0x9e3779b97f4a7c15ULL) ^ c ^ seed;

  return (unsigned long) __builtin_popcountll(x) +
    (unsigned long) __builtin_clzll(x | 1) +
    (unsigned long) __builtin_ctzll(x | 0x100) +
    (unsigned long) __builtin_parityll(x);
}
