typedef unsigned long (*poly_funcptr)(unsigned long, unsigned long);

__attribute__((noinline))
static unsigned long poly_helper(unsigned long a0, unsigned long a1)
{
  return a0 * 3 + a1 + 77;
}

__attribute__((used))
static poly_funcptr poly_indirect = poly_helper;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return poly_indirect(a0, a1) + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}
