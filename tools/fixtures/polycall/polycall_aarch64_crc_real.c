typedef unsigned int u32;
typedef unsigned long u64;

static u32 hw_crc32b(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32b %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32h(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32h %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32w(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32w %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32x(u32 crc, u64 value)
{
  u32 result;
  __asm__ volatile("crc32x %w0,%w1,%x2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32cb(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32cb %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32ch(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32ch %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32cw(u32 crc, u32 value)
{
  u32 result;
  __asm__ volatile("crc32cw %w0,%w1,%w2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 hw_crc32cx(u32 crc, u64 value)
{
  u32 result;
  __asm__ volatile("crc32cx %w0,%w1,%x2" : "=r"(result) : "r"(crc), "r"(value));
  return result;
}

static u32 crc_update(u32 crc, u64 value, unsigned bytes, u32 polynomial)
{
  for (unsigned byte = 0; byte < bytes; byte++) {
    crc ^= (u32) ((value >> (byte * 8)) & 0xff);
    for (unsigned bit = 0; bit < 8; bit++)
      crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
  }
  return crc;
}

__attribute__((visibility("default")))
u64 poly_entry(u64 a0, u64 a1, u64 a2, u64 a3)
{
  u32 crc = (u32) (0x12345678U ^ a0 ^ (a2 >> 13));
  u32 word = (u32) (0x89abcdefU ^ a1 ^ (a3 << 7));
  u64 wide = 0xfedcba9876543210UL ^ (a0 << 3) ^ (a3 >> 5);
  u64 failures = 0;

  if (hw_crc32b(crc, word) != crc_update(crc, word, 1, 0xedb88320U))
    failures |= 1UL << 0;
  if (hw_crc32h(crc, word) != crc_update(crc, word, 2, 0xedb88320U))
    failures |= 1UL << 1;
  if (hw_crc32w(crc, word) != crc_update(crc, word, 4, 0xedb88320U))
    failures |= 1UL << 2;
  if (hw_crc32x(crc, wide) != crc_update(crc, wide, 8, 0xedb88320U))
    failures |= 1UL << 3;

  if (hw_crc32cb(crc, word) != crc_update(crc, word, 1, 0x82f63b78U))
    failures |= 1UL << 4;
  if (hw_crc32ch(crc, word) != crc_update(crc, word, 2, 0x82f63b78U))
    failures |= 1UL << 5;
  if (hw_crc32cw(crc, word) != crc_update(crc, word, 4, 0x82f63b78U))
    failures |= 1UL << 6;
  if (hw_crc32cx(crc, wide) != crc_update(crc, wide, 8, 0x82f63b78U))
    failures |= 1UL << 7;

  return failures == 0 ? 42 : (0xbadc0000UL | failures);
}
