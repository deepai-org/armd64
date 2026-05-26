__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  long delta = (long) (a0 - a8);
  unsigned long magnitude =
    delta < 0 ? (unsigned long) -delta : (unsigned long) delta;
  unsigned long flag = a1 < a2 ? 1UL : 0UL;
  unsigned long inverted = a3 != a4 ? ~a5 : a5;

  return magnitude + flag + (inverted & 0xffUL) + a7 + (a6 & 0UL);
}
