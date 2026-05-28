static volatile unsigned long seed64 = 0x9e3779b97f4a7c15UL;
static volatile unsigned int seed32 = 0x85ebca6bU;

static unsigned long rotl64(unsigned long value, unsigned int shift)
{
  return (value << shift) | (value >> (64 - shift));
}

static unsigned long rotr64(unsigned long value, unsigned int shift)
{
  return (value >> shift) | (value << (64 - shift));
}

static unsigned int rotl32(unsigned int value, unsigned int shift)
{
  return (value << shift) | (value >> (32 - shift));
}

static unsigned int rotr32(unsigned int value, unsigned int shift)
{
  return (value >> shift) | (value << (32 - shift));
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long x = seed64 ^ a0 ^ (a1 << 9);
  unsigned long y = seed64 + a2 + (a3 << 17);
  unsigned int w = seed32 ^ (unsigned int) a2 ^ ((unsigned int) a3 << 5);
  unsigned long r0 = rotl64(x, 13);
  unsigned long r1 = rotr64(x + a2, 27);
  unsigned long e0 = (x >> 19) | (y << 45);
  unsigned int r2 = rotl32(w, 7);
  unsigned int r3 = rotr32(w + (unsigned int) a1, 11);
  unsigned int e1 = (w >> 5) | ((unsigned int) a0 << 27);

  return r0 ^ r1 ^ e0 ^ ((unsigned long) r2 << 32) ^ r3 ^ e1;
}
