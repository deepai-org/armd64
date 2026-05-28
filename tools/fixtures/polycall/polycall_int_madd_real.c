static volatile unsigned long bias64 = 11;
static volatile unsigned int bias32 = 7;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long prod_add64 = a0 * a1 + bias64;
  unsigned long prod_sub64 = a2 - a3 * a4;
  unsigned int prod_add32 = (unsigned int) a5 * (unsigned int) a6 + bias32;
  unsigned int prod_sub32 = (unsigned int) a8 - (unsigned int) a7 * 2U;

  return prod_add64 + prod_sub64 + prod_add32 + prod_sub32;
}
