static unsigned long poly_state;

__attribute__((constructor))
static void poly_ctor(void)
{
  poly_state = 100;
}

__attribute__((destructor))
static void poly_fini(void)
{
  poly_state += 1000;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  poly_state += a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  return poly_state;
}

__attribute__((visibility("default")))
unsigned long poly_fini_result(void)
{
  return poly_state;
}
