static volatile unsigned long seed = 0x123456789abcdef0UL;
static volatile long signed_seed = -0x123456789abcdeL;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long shift_l = (a1 + 5) & 63;
  unsigned long shift_r = (a2 + 11) & 63;
  unsigned long shift_s = (a3 + 17) & 63;
  unsigned long value = seed ^ a0;
  long signed_value = signed_seed - (long) a0;
  unsigned long left = value << shift_l;
  unsigned long right = value >> shift_r;
  unsigned long arith = (unsigned long) (signed_value >> shift_s);
  unsigned int word = (unsigned int) value;
  unsigned int wshift = (unsigned int) ((a1 + a2) & 31);
  unsigned long word_mix =
    (unsigned long) ((word << wshift) ^ (word >> ((wshift + 7) & 31)));

  return left ^ right ^ arith ^ word_mix;
}
