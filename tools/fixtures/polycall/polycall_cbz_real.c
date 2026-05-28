static volatile unsigned long branch_bias;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long total = a0 + a1 + a2;

  if (a3 != 0) {
    branch_bias += a3;
    total += branch_bias + a3 * 7;
  }
  else {
    branch_bias += a4;
    total += branch_bias + a4 * 11;
  }

  if (a6 == 0) {
    branch_bias += a7;
    total += branch_bias + a7 * 13;
  }
  else {
    branch_bias += a6;
    total += branch_bias + a6 * 17;
  }

  return total + a5 + a8;
}
