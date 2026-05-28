static volatile unsigned long seed = 5;

static unsigned __int128 unsigned_value =
  (((unsigned __int128) 0x1020304050607080ULL) << 64) |
  0x90a0b0c0d0e0f001ULL;

static __int128 signed_value =
  -((((__int128) 0x102030405060708ULL) << 64) |
    0x90a0b0c0d0e0f001ULL);

static float positive_value = 12345.25f;
static float negative_value = -987.75f;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned __int128 from_float =
    (unsigned __int128) (positive_value + (float) a);
  __int128 signed_from_float =
    (__int128) (negative_value - (float) b);
  float unsigned_to_float = (float) (unsigned_value + seed + c);
  float signed_to_float = (float) (signed_value - (__int128) (a + b));
  unsigned long mix =
    (unsigned long) unsigned_to_float ^ (unsigned long) (-signed_to_float);

  return (unsigned long) from_float +
    (unsigned long) (from_float >> 64) +
    (unsigned long) signed_from_float +
    (unsigned long) (signed_from_float >> 64) + mix + c;
}
