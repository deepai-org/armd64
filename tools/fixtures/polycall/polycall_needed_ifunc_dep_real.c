static unsigned long poly_needed_ifunc_impl(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return 800 + a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}

static void *poly_needed_ifunc_resolver(void)
{
  return poly_needed_ifunc_impl;
}

unsigned long poly_needed_ifunc(unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long)
    __attribute__((ifunc("poly_needed_ifunc_resolver")));
