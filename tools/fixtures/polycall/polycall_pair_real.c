struct pair_u64 {
  unsigned long lo;
  unsigned long hi;
};

__attribute__((visibility("default")))
struct pair_u64 poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  struct pair_u64 result;
  result.lo = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  result.hi = a8 * 10 + a7;
  return result;
}
