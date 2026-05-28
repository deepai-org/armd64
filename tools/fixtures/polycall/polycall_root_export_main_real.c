extern unsigned long poly_needed_root_call(unsigned long, unsigned long);

__attribute__((visibility("default")))
unsigned long poly_root_callback(unsigned long a, unsigned long b)
{
  return a + b + 1000;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return poly_needed_root_call(a0, a1) + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}
