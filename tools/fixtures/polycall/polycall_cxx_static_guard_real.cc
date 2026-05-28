static unsigned long guarded_value(unsigned long seed)
{
  static unsigned long value = seed + 33;
  return value;
}

extern "C" __attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;

  result += guarded_value(a0);
  result += guarded_value(a8);
  return result;
}
