struct sret_u64 {
  unsigned long a;
  unsigned long b;
  unsigned long c;
  unsigned long d;
};

__attribute__((visibility("default")))
struct sret_u64 poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2)
{
  struct sret_u64 result;
  result.a = a0 + 10;
  result.b = a1 + 20;
  result.c = a2 + 30;
  result.d = a0 * 100 + a1 * 10 + a2;
  return result;
}
