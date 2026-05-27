extern unsigned long poly_root_ifunc(unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long);

__attribute__((visibility("default")))
unsigned long poly_needed_root_ifunc_call(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return poly_root_ifunc(a0, a1, a2, a3, a4, a5, a6, a7, a8) + 300;
}
