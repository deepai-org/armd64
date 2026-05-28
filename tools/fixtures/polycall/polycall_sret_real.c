struct sret_u64 {
  unsigned long a;
  unsigned long b;
  unsigned long c;
  unsigned long d;
};

__attribute__((visibility("default")))
struct sret_u64 poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7)
{
  struct sret_u64 result;
  result.a = a0 + a1 + a2 + a3;
  result.b = a4 + a5 + a6 + a7;
  result.c = a7 * 10 + a0;
  result.d = a5 * 100 + a6 * 10 + a7;
  return result;
}
