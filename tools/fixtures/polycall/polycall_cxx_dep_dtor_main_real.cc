extern "C" unsigned long poly_cxx_dep_dtor_touch(unsigned long,
    unsigned long);

extern "C" __attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;

  result += poly_cxx_dep_dtor_touch(a0, a1);
  result += poly_cxx_dep_dtor_touch(a7, a8);
  return result;
}
