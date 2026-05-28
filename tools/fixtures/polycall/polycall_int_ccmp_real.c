static volatile unsigned long bias = 17;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b,
    unsigned long c, unsigned long d)
{
  unsigned long r = bias;

  if (a < b && c == d)
    r += a + c;
  if (a >= d || b != c)
    r ^= (b << 3) + d;
  if ((a + b) > (c + d) && (a ^ c) <= (b ^ d))
    r += 91;

  return r;
}
