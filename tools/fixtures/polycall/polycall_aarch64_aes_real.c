typedef unsigned char u8x16 __attribute__((vector_size(16)));
typedef unsigned long u64;

union vec_bytes {
  u8x16 v;
  unsigned char b[16];
};

static const union vec_bytes seed_state = {
  .b = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
         0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff }
};

static const union vec_bytes seed_key = {
  .b = { 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
         0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 }
};

static const union vec_bytes expected_aese = {
  .b = { 0x76, 0xcf, 0x79, 0x16, 0x84, 0xdb, 0xdf, 0x75,
         0x73, 0x9e, 0x15, 0xd2, 0x8a, 0xc0, 0xa8, 0x08 }
};

static const union vec_bytes expected_aesd = {
  .b = { 0xfb, 0xef, 0x1b, 0x6b, 0x92, 0xcb, 0x61, 0xf4,
         0x73, 0x84, 0x4e, 0x7d, 0x5f, 0x6e, 0x06, 0x25 }
};

static const union vec_bytes expected_aesmc = {
  .b = { 0xc9, 0x6e, 0x71, 0x00, 0xcf, 0x26, 0x65, 0x79,
         0x98, 0xb9, 0xaa, 0xa1, 0xf4, 0xfa, 0x19, 0xfd }
};

static const union vec_bytes expected_aesimc = {
  .b = { 0x17, 0x53, 0x3f, 0x1f, 0x37, 0x8e, 0x47, 0x32,
         0xcc, 0x7c, 0x62, 0x16, 0xc0, 0x0b, 0xbd, 0x64 }
};

static u8x16 hw_aese(u8x16 state, u8x16 key)
{
  __asm__ volatile("aese %0.16b,%1.16b" : "+w"(state) : "w"(key));
  return state;
}

static u8x16 hw_aesd(u8x16 state, u8x16 key)
{
  __asm__ volatile("aesd %0.16b,%1.16b" : "+w"(state) : "w"(key));
  return state;
}

static u8x16 hw_aesmc(u8x16 state)
{
  u8x16 result;
  __asm__ volatile("aesmc %0.16b,%1.16b" : "=w"(result) : "w"(state));
  return result;
}

static u8x16 hw_aesimc(u8x16 state)
{
  u8x16 result;
  __asm__ volatile("aesimc %0.16b,%1.16b" : "=w"(result) : "w"(state));
  return result;
}

static int same_vec(union vec_bytes actual, union vec_bytes expected)
{
  for (unsigned i = 0; i < 16; i++) {
    if (actual.b[i] != expected.b[i])
      return 0;
  }
  return 1;
}

__attribute__((visibility("default")))
u64 poly_entry(u64 a0, u64 a1, u64 a2, u64 a3)
{
  union vec_bytes actual;
  u64 failures = 0;

  (void) a0;
  (void) a1;
  (void) a2;
  (void) a3;

  actual.v = hw_aese(seed_state.v, seed_key.v);
  if (!same_vec(actual, expected_aese))
    failures |= 1UL << 0;

  actual.v = hw_aesd(seed_state.v, seed_key.v);
  if (!same_vec(actual, expected_aesd))
    failures |= 1UL << 1;

  actual.v = hw_aesmc(expected_aese.v);
  if (!same_vec(actual, expected_aesmc))
    failures |= 1UL << 2;

  actual.v = hw_aesimc(expected_aesd.v);
  if (!same_vec(actual, expected_aesimc))
    failures |= 1UL << 3;

  return failures == 0 ? 42 : (0xbae50000UL | failures);
}
