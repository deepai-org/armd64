__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  long delta = (long) (a0 - a8);
  long shifted = delta >> 2;
  long field = (long) (((a1 - a7) & 0xfc0UL) << 52) >> 58;
  long byte = (signed char) (a2 - a8);
  long half = (short) (a3 - a8);

  return (unsigned long) (shifted + field + byte + half +
    (long) a4 + (long) a5 + (long) a6);
}
