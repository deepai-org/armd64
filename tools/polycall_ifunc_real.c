static unsigned long poly_ifunc_impl(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return 700 + a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}

static void *poly_ifunc_resolver(void)
{
  return poly_ifunc_impl;
}

unsigned long poly_ifunc_target(unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long)
    __attribute__((ifunc("poly_ifunc_resolver")));

static unsigned long (*volatile poly_ifunc_ptr)(unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long) = poly_ifunc_target;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return poly_ifunc_ptr(a0, a1, a2, a3, a4, a5, a6, a7, a8);
}
