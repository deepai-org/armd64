static volatile unsigned long seed = 0x0123456789abcdefUL;
static volatile unsigned int word_seed = 0x89abcdefU;
static volatile long signed_seed = -0x123456789abcL;

#ifdef __aarch64__
#define poly_bswap64(v) __builtin_bswap64(v)
#define poly_bswap32(v) __builtin_bswap32(v)
#define poly_bswap16(v) __builtin_bswap16(v)
#define poly_clzl(v) __builtin_clzl(v)
#define poly_clz(v) __builtin_clz(v)
#define poly_clrsbl(v) __builtin_clrsbl(v)
#define poly_clrsb(v) __builtin_clrsb(v)
#define poly_ctzl(v) __builtin_ctzl(v)
#define poly_ctz(v) __builtin_ctz(v)
#else
static unsigned long poly_bswap64(unsigned long value)
{
  unsigned long result = 0;
  for (unsigned i = 0; i < 8; i++)
    result |= ((value >> (i * 8)) & 0xffUL) << ((7 - i) * 8);
  return result;
}

static unsigned int poly_bswap32(unsigned int value)
{
  unsigned int result = 0;
  for (unsigned i = 0; i < 4; i++)
    result |= ((value >> (i * 8)) & 0xffU) << ((3 - i) * 8);
  return result;
}

static unsigned short poly_bswap16(unsigned short value)
{
  return (unsigned short) ((value << 8) | (value >> 8));
}

static int poly_clzl(unsigned long value)
{
  int count = 0;
  for (int bit = 63; bit >= 0; bit--) {
    if (value & (1UL << bit))
      break;
    count++;
  }
  return count;
}

static int poly_clz(unsigned int value)
{
  int count = 0;
  for (int bit = 31; bit >= 0; bit--) {
    if (value & (1U << bit))
      break;
    count++;
  }
  return count;
}

static int poly_clrsbl(long value)
{
  unsigned long bits = (unsigned long) value;
  int sign = (bits >> 63) & 1;
  int count = 0;
  for (int bit = 62; bit >= 0; bit--) {
    if (((bits >> bit) & 1) != (unsigned long) sign)
      break;
    count++;
  }
  return count;
}

static int poly_clrsb(int value)
{
  unsigned int bits = (unsigned int) value;
  int sign = (bits >> 31) & 1;
  int count = 0;
  for (int bit = 30; bit >= 0; bit--) {
    if (((bits >> bit) & 1) != (unsigned int) sign)
      break;
    count++;
  }
  return count;
}

static int poly_ctzl(unsigned long value)
{
  int count = 0;
  while (((value >> count) & 1UL) == 0)
    count++;
  return count;
}

static int poly_ctz(unsigned int value)
{
  int count = 0;
  while (((value >> count) & 1U) == 0)
    count++;
  return count;
}
#endif

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3)
{
  unsigned long x = (seed ^ (a0 << 3) ^ a1) | 1UL;
  unsigned int w = (word_seed ^ (unsigned int) a2) | 1U;
  unsigned short h = (unsigned short) (w ^ (unsigned int) a3);
  long sx = signed_seed - (long) (a1 << 7);
  int sw = (int) (w | 0x80000000U);
  unsigned long swapped = poly_bswap64(x);
  unsigned int swapped_w = poly_bswap32(w);
  unsigned short swapped_h = poly_bswap16(h);
  unsigned long lz = (unsigned long) poly_clzl(x);
  unsigned long lzw = (unsigned long) poly_clz(w);
  unsigned long cls = (unsigned long) poly_clrsbl(sx);
  unsigned long clsw = (unsigned long) poly_clrsb(sw);
  unsigned long tz = (unsigned long) poly_ctzl(x + (a3 << 1));
  unsigned long tzw = (unsigned long) poly_ctz(w + ((unsigned int) a3 << 1));

  return swapped ^ ((unsigned long) swapped_w << 17) ^
    ((unsigned long) swapped_h << 33) ^ (lz << 48) ^ (lzw << 40) ^
    (cls << 24) ^ (clsw << 16) ^ (tz << 8) ^ tzw;
}
