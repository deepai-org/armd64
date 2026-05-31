__attribute__((noinline))
static unsigned long mix(unsigned long value)
{
  return (value << 1) ^ 0x55UL;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5)
{
  unsigned long weighted = a0 + a1 * 3 + a2 * 5 + a3 * 7 + a4 * 11 +
    a5 * 13;
  return weighted + mix(a0 + a5) - mix(7);
}
