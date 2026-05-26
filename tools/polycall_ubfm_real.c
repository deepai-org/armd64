__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long low8 = (unsigned char) a0;
  unsigned long low16 = (unsigned short) a1;
  unsigned long shifted = (a2 + 1024UL) >> 5;
  unsigned long left = ((a3 + 1024UL) << 9) & 0xffff0000UL;
  unsigned long field = ((a4 + 4096UL) >> 11) & 0x3fUL;

  return low8 + low16 + shifted + left + field + a5 + a6 + a7 + a8;
}
