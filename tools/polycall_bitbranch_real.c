static volatile unsigned long bit_sink;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1, unsigned long a2,
    unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
    unsigned long a7, unsigned long a8)
{
  unsigned long total = a0 + a8;

  if (a1 & (1UL << 5)) {
    bit_sink += a2;
    total += bit_sink + a2 + 30;
  }
  else {
    bit_sink += a3;
    total += bit_sink + a3 + 40;
  }

  if ((a4 & (1UL << 33)) == 0) {
    bit_sink += a5;
    total += bit_sink + a5 + 50;
  }
  else {
    bit_sink += a6;
    total += bit_sink + a6 + 60;
  }

  return total + a7;
}
