static volatile signed char c_bias = -2;
static volatile signed short h_bias = -3;
static volatile signed int w_bias = -20;
static volatile unsigned char uc_bias = 9;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  signed char c = c_bias;
  signed short h = h_bias;
  signed int w = w_bias;
  unsigned char uc = uc_bias;

  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    (unsigned long) c + (unsigned long) h + (unsigned long) w + uc;
}
