extern "C" __attribute__((visibility("default")))
unsigned long poly_cxx_guard_dep(unsigned long seed, unsigned long addend)
{
  static unsigned long value = seed + 70;
  return value + seed + addend;
}
