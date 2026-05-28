static unsigned long counter = 41;
static unsigned int word_counter = 9;
static unsigned int add_word_counter = 31;
static unsigned int swap_word_counter = 23;
static unsigned long and_counter = 0xff;
static unsigned long xor_counter = 0x55;
static unsigned long or_counter = 0x40;
static unsigned int and_word_counter = 0x7f;
static unsigned int xor_word_counter = 0x21;
static unsigned int or_word_counter = 0x12;
static unsigned short add_half_counter = 5;
static unsigned char add_byte_counter = 6;
static unsigned short swap_half_counter = 17;
static unsigned char swap_byte_counter = 11;
static unsigned short and_half_counter = 0x7f;
static unsigned char and_byte_counter = 0x3f;
static unsigned short xor_half_counter = 0x21;
static unsigned char xor_byte_counter = 0x12;
static unsigned short or_half_counter = 0x100;
static unsigned char or_byte_counter = 0x20;
static unsigned long nand_counter = 0xff;
static unsigned int nand_word_counter = 0x7f;
static unsigned short nand_half_counter = 0x33;
static unsigned char nand_byte_counter = 0x17;
static unsigned long min_counter = 100;
static unsigned long max_counter = 100;
static unsigned long minu_counter = 100;
static unsigned long maxu_counter = 100;
static unsigned int min_word_counter = 100;
static unsigned int max_word_counter = 100;
static unsigned int minu_word_counter = 100;
static unsigned int maxu_word_counter = 100;
static unsigned long cas_value = 99;
static unsigned int cas_word_value = 19;
static unsigned short cas_half_value = 123;
static unsigned char cas_byte_value = 27;
static __int128 wide_value __attribute__((aligned(16))) =
  (((__int128) 2) << 64) | 1;

struct pair64 {
  unsigned long lo;
  unsigned long hi;
};

static struct pair64 pair_value __attribute__((aligned(16))) = { 3, 4 };

static unsigned long fetch_min_u64(unsigned long *ptr, long value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned long old;
  __asm__ volatile ("ldsminal %x[value], %x[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned long old = *ptr;
  if ((long) old > value)
    *ptr = (unsigned long) value;
  return old;
#endif
}

static unsigned long fetch_max_u64(unsigned long *ptr, long value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned long old;
  __asm__ volatile ("ldsmaxal %x[value], %x[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned long old = *ptr;
  if ((long) old < value)
    *ptr = (unsigned long) value;
  return old;
#endif
}

static unsigned long fetch_minu_u64(unsigned long *ptr, unsigned long value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned long old;
  __asm__ volatile ("lduminal %x[value], %x[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned long old = *ptr;
  if (old > value)
    *ptr = value;
  return old;
#endif
}

static unsigned long fetch_maxu_u64(unsigned long *ptr, unsigned long value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned long old;
  __asm__ volatile ("ldumaxal %x[value], %x[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned long old = *ptr;
  if (old < value)
    *ptr = value;
  return old;
#endif
}

static unsigned int fetch_min_u32(unsigned int *ptr, int value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned int old;
  __asm__ volatile ("ldsminal %w[value], %w[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned int old = *ptr;
  if ((int) old > value)
    *ptr = (unsigned int) value;
  return old;
#endif
}

static unsigned int fetch_max_u32(unsigned int *ptr, int value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned int old;
  __asm__ volatile ("ldsmaxal %w[value], %w[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned int old = *ptr;
  if ((int) old < value)
    *ptr = (unsigned int) value;
  return old;
#endif
}

static unsigned int fetch_minu_u32(unsigned int *ptr, unsigned int value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned int old;
  __asm__ volatile ("lduminal %w[value], %w[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned int old = *ptr;
  if (old > value)
    *ptr = value;
  return old;
#endif
}

static unsigned int fetch_maxu_u32(unsigned int *ptr, unsigned int value)
{
#if defined(__aarch64__) && defined(__ARM_FEATURE_ATOMICS)
  unsigned int old;
  __asm__ volatile ("ldumaxal %w[value], %w[old], [%[ptr]]"
      : [old] "=&r"(old), "+m"(*ptr)
      : [value] "r"(value), [ptr] "r"(ptr)
      : "memory");
  return old;
#else
  unsigned int old = *ptr;
  if (old < value)
    *ptr = value;
  return old;
#endif
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned long old = __atomic_fetch_add(&counter, a + 1, __ATOMIC_ACQ_REL);
  unsigned long prev = __atomic_exchange_n(&counter, old ^ b,
      __ATOMIC_RELEASE);
  unsigned int wold = __atomic_fetch_or(&word_counter, (unsigned int) c,
      __ATOMIC_RELAXED);
  unsigned int add_wold = __atomic_fetch_add(&add_word_counter,
      (unsigned int) c + 4, __ATOMIC_ACQUIRE);
  unsigned int swap_wold = __atomic_exchange_n(&swap_word_counter,
      (unsigned int) c + 30, __ATOMIC_RELAXED);
  unsigned long and_old = __atomic_fetch_and(&and_counter, a + 0x0f,
      __ATOMIC_ACQUIRE);
  unsigned long xor_old = __atomic_fetch_xor(&xor_counter, b + 0x10,
      __ATOMIC_RELEASE);
  unsigned long or_old = __atomic_fetch_or(&or_counter, a + 0x80,
      __ATOMIC_RELAXED);
  unsigned int and_wold = __atomic_fetch_and(&and_word_counter,
      (unsigned int) c + 0x10, __ATOMIC_ACQUIRE);
  unsigned int xor_wold = __atomic_fetch_xor(&xor_word_counter,
      (unsigned int) c + 0x20, __ATOMIC_RELEASE);
  unsigned int or_wold = __atomic_fetch_or(&or_word_counter,
      (unsigned int) c + 0x40, __ATOMIC_RELAXED);
  unsigned short add_hold = __atomic_fetch_add(&add_half_counter,
      (unsigned short) c + 5, __ATOMIC_ACQUIRE);
  unsigned char add_bold = __atomic_fetch_add(&add_byte_counter,
      (unsigned char) a + 4, __ATOMIC_RELAXED);
  unsigned short swap_hold = __atomic_exchange_n(&swap_half_counter,
      (unsigned short) b + 40, __ATOMIC_RELEASE);
  unsigned char swap_bold = __atomic_exchange_n(&swap_byte_counter,
      (unsigned char) c + 7, __ATOMIC_ACQUIRE);
  unsigned short and_hold = __atomic_fetch_and(&and_half_counter,
      (unsigned short) c + 0x30, __ATOMIC_ACQ_REL);
  unsigned char and_bold = __atomic_fetch_and(&and_byte_counter,
      (unsigned char) a + 0x08, __ATOMIC_RELAXED);
  unsigned short xor_hold = __atomic_fetch_xor(&xor_half_counter,
      (unsigned short) b + 0x40, __ATOMIC_RELEASE);
  unsigned char xor_bold = __atomic_fetch_xor(&xor_byte_counter,
      (unsigned char) c + 0x50, __ATOMIC_ACQUIRE);
  unsigned short or_hold = __atomic_fetch_or(&or_half_counter,
      (unsigned short) a + 0x20, __ATOMIC_ACQ_REL);
  unsigned char or_bold = __atomic_fetch_or(&or_byte_counter,
      (unsigned char) b + 0x04, __ATOMIC_RELAXED);
  unsigned long nand_old = __atomic_fetch_nand(&nand_counter, a + 0x10,
      __ATOMIC_ACQ_REL);
  unsigned int nand_wold = __atomic_fetch_nand(&nand_word_counter,
      (unsigned int) b + 0x20, __ATOMIC_ACQ_REL);
  unsigned short nand_hold = __atomic_fetch_nand(&nand_half_counter,
      (unsigned short) c + 0x30, __ATOMIC_ACQ_REL);
  unsigned char nand_bold = __atomic_fetch_nand(&nand_byte_counter,
      (unsigned char) a + 0x40, __ATOMIC_ACQ_REL);
  unsigned long min_old = fetch_min_u64(&min_counter, -((long) a + 5));
  unsigned long max_old = fetch_max_u64(&max_counter, (long) a + 200);
  unsigned long minu_old = fetch_minu_u64(&minu_counter, b + 50);
  unsigned long maxu_old = fetch_maxu_u64(&maxu_counter, c + 300);
  unsigned int min_wold = fetch_min_u32(&min_word_counter, -((int) b + 5));
  unsigned int max_wold = fetch_max_u32(&max_word_counter, (int) b + 200);
  unsigned int minu_wold = fetch_minu_u32(&minu_word_counter,
      (unsigned int) c + 50);
  unsigned int maxu_wold = fetch_maxu_u32(&maxu_word_counter,
      (unsigned int) c + 300);
  unsigned long expected = 99;
  int ok = __atomic_compare_exchange_n(&cas_value, &expected, c + 100, 0,
      __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  unsigned int word_expected = 19;
  int word_ok = __atomic_compare_exchange_n(&cas_word_value, &word_expected,
      (unsigned int) c + 20, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
  unsigned short half_expected = 123;
  int half_ok = __atomic_compare_exchange_n(&cas_half_value, &half_expected,
      (unsigned short) c + 200, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  unsigned char byte_expected = 27;
  int byte_ok = __atomic_compare_exchange_n(&cas_byte_value, &byte_expected,
      (unsigned char) a + 40, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
  __int128 wide_expected = (((__int128) 2) << 64) | 1;
  __int128 wide_desired = (((__int128) (b + 20)) << 64) | (a + 10);
  int wide_ok = __atomic_compare_exchange_n(&wide_value, &wide_expected,
      wide_desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
  struct pair64 pair_desired = { a + 30, b + 40 };
  __atomic_store(&pair_value, &pair_desired, __ATOMIC_RELEASE);
  struct pair64 pair_loaded;
  __atomic_load(&pair_value, &pair_loaded, __ATOMIC_ACQUIRE);

  return old + prev + counter + word_counter + wold + cas_value +
    (unsigned long) ok + expected + (unsigned long) word_ok +
    word_expected + cas_word_value + add_wold + add_word_counter +
    swap_wold + swap_word_counter + and_old + and_counter + xor_old +
    xor_counter + or_old + or_counter + and_wold + and_word_counter +
    xor_wold + xor_word_counter + or_wold + or_word_counter + add_hold +
    add_half_counter + add_bold + add_byte_counter + swap_hold +
    swap_half_counter + swap_bold + swap_byte_counter + and_hold +
    and_half_counter + and_bold + and_byte_counter + xor_hold +
    xor_half_counter + xor_bold + xor_byte_counter + or_hold +
    or_half_counter + or_bold + or_byte_counter + (unsigned long) half_ok +
    half_expected + cas_half_value + (unsigned long) byte_ok +
    byte_expected + cas_byte_value + nand_old + nand_counter + nand_wold +
    nand_word_counter + nand_hold + nand_half_counter + nand_bold +
    nand_byte_counter + min_old + min_counter + max_old + max_counter +
    minu_old + minu_counter + maxu_old + maxu_counter + min_wold +
    min_word_counter + max_wold + max_word_counter + minu_wold +
    minu_word_counter + maxu_wold + maxu_word_counter +
    (unsigned long) wide_ok + (unsigned long) wide_expected +
    (unsigned long) (wide_expected >> 64) + (unsigned long) wide_value +
    (unsigned long) (wide_value >> 64) + pair_loaded.lo + pair_loaded.hi;
}
