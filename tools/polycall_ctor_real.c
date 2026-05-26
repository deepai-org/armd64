static unsigned long poly_ctor_bias;

__attribute__((constructor))
static void poly_ctor(void)
{
  poly_ctor_bias = 200;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return poly_ctor_bias + a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}
