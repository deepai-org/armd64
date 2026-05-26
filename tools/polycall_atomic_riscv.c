static unsigned long counter = 41;
static unsigned int word_counter = 9;
static unsigned int add_word_counter = 31;
static unsigned int swap_word_counter = 23;
static unsigned long and_counter = 0xff;
static unsigned long xor_counter = 0x55;
static unsigned long or_counter = 0x40;
static unsigned long sub_counter = 0xff;
static unsigned long nand_counter = 0xff;
static unsigned long min_counter = 100;
static unsigned long max_counter = 100;
static unsigned long minu_counter = 100;
static unsigned long maxu_counter = 100;
static unsigned int and_word_counter = 0x7f;
static unsigned int xor_word_counter = 0x21;
static unsigned int or_word_counter = 0x12;
static unsigned int sub_word_counter = 0x7f;
static unsigned int nand_word_counter = 0x7f;
static unsigned int min_word_counter = 100;
static unsigned int max_word_counter = 100;
static unsigned int minu_word_counter = 100;
static unsigned int maxu_word_counter = 100;
static unsigned short add_half_counter = 5;
static unsigned char add_byte_counter = 6;
static unsigned short xor_half_counter = 0x21;
static unsigned char or_byte_counter = 0x20;
static unsigned short sub_half_counter = 0x33;
static unsigned char sub_byte_counter = 0x17;
static unsigned short nand_half_counter = 0x33;
static unsigned char nand_byte_counter = 0x17;
static unsigned long cas_value = 99;
static unsigned short cas_half_value = 123;
static unsigned char cas_byte_value = 27;

static unsigned long fetch_min_u64(unsigned long *ptr, long value)
{
#ifdef __riscv
  unsigned long old;
  __asm__ volatile ("amomin.d.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
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
#ifdef __riscv
  unsigned long old;
  __asm__ volatile ("amomax.d.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
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
#ifdef __riscv
  unsigned long old;
  __asm__ volatile ("amominu.d.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
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
#ifdef __riscv
  unsigned long old;
  __asm__ volatile ("amomaxu.d.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
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
#ifdef __riscv
  long old;
  __asm__ volatile ("amomin.w.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
      : "memory");
  return (unsigned int) old;
#else
  unsigned int old = *ptr;
  if ((int) old > value)
    *ptr = (unsigned int) value;
  return old;
#endif
}

static unsigned int fetch_max_u32(unsigned int *ptr, int value)
{
#ifdef __riscv
  long old;
  __asm__ volatile ("amomax.w.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
      : "memory");
  return (unsigned int) old;
#else
  unsigned int old = *ptr;
  if ((int) old < value)
    *ptr = (unsigned int) value;
  return old;
#endif
}

static unsigned int fetch_minu_u32(unsigned int *ptr, unsigned int value)
{
#ifdef __riscv
  long old;
  __asm__ volatile ("amominu.w.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
      : "memory");
  return (unsigned int) old;
#else
  unsigned int old = *ptr;
  if (old > value)
    *ptr = value;
  return old;
#endif
}

static unsigned int fetch_maxu_u32(unsigned int *ptr, unsigned int value)
{
#ifdef __riscv
  long old;
  __asm__ volatile ("amomaxu.w.aqrl %0, %2, %1"
      : "=r"(old), "+A"(*ptr)
      : "r"(value)
      : "memory");
  return (unsigned int) old;
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
  unsigned long prev = __atomic_exchange_n(&counter, old ^ b, __ATOMIC_SEQ_CST);
  unsigned int wold = __atomic_fetch_or(&word_counter, (unsigned int) c,
      __ATOMIC_RELAXED);
  unsigned int add_wold = __atomic_fetch_add(&add_word_counter,
      (unsigned int) c + 4, __ATOMIC_ACQ_REL);
  unsigned int swap_wold = __atomic_exchange_n(&swap_word_counter,
      (unsigned int) c + 30, __ATOMIC_SEQ_CST);
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
  unsigned long sub_old = __atomic_fetch_sub(&sub_counter, a + 0x10,
      __ATOMIC_ACQ_REL);
  unsigned int sub_wold = __atomic_fetch_sub(&sub_word_counter,
      (unsigned int) b + 0x20, __ATOMIC_ACQ_REL);
  unsigned long nand_old = __atomic_fetch_nand(&nand_counter, a + 0x10,
      __ATOMIC_ACQ_REL);
  unsigned int nand_wold = __atomic_fetch_nand(&nand_word_counter,
      (unsigned int) b + 0x20, __ATOMIC_ACQ_REL);
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
  unsigned short add_hold = __atomic_fetch_add(&add_half_counter,
      (unsigned short) c + 5, __ATOMIC_ACQUIRE);
  unsigned char add_bold = __atomic_fetch_add(&add_byte_counter,
      (unsigned char) a + 4, __ATOMIC_RELAXED);
  unsigned short xor_hold = __atomic_fetch_xor(&xor_half_counter,
      (unsigned short) b + 0x40, __ATOMIC_RELEASE);
  unsigned char or_bold = __atomic_fetch_or(&or_byte_counter,
      (unsigned char) b + 0x04, __ATOMIC_RELAXED);
  unsigned short sub_hold = __atomic_fetch_sub(&sub_half_counter,
      (unsigned short) c + 0x30, __ATOMIC_ACQ_REL);
  unsigned char sub_bold = __atomic_fetch_sub(&sub_byte_counter,
      (unsigned char) a + 0x40, __ATOMIC_ACQ_REL);
  unsigned short nand_hold = __atomic_fetch_nand(&nand_half_counter,
      (unsigned short) c + 0x30, __ATOMIC_ACQ_REL);
  unsigned char nand_bold = __atomic_fetch_nand(&nand_byte_counter,
      (unsigned char) a + 0x40, __ATOMIC_ACQ_REL);
  unsigned long expected = 99;
  int ok = __atomic_compare_exchange_n(&cas_value, &expected, c + 100, 0,
      __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
  unsigned short half_expected = 123;
  int half_ok = __atomic_compare_exchange_n(&cas_half_value, &half_expected,
      (unsigned short) c + 200, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  unsigned char byte_expected = 27;
  int byte_ok = __atomic_compare_exchange_n(&cas_byte_value, &byte_expected,
      (unsigned char) a + 40, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);

  return old + prev + counter + word_counter + wold + cas_value +
    (unsigned long) ok + expected + add_wold + add_word_counter +
    swap_wold + swap_word_counter + and_old + and_counter + xor_old +
    xor_counter + or_old + or_counter + and_wold + and_word_counter +
    xor_wold + xor_word_counter + or_wold + or_word_counter + add_hold +
    add_half_counter + add_bold + add_byte_counter + xor_hold +
    xor_half_counter + or_bold + or_byte_counter + sub_old + sub_counter +
    sub_wold + sub_word_counter + sub_hold + sub_half_counter + sub_bold +
    sub_byte_counter + nand_old + nand_counter + nand_wold +
    nand_word_counter + nand_hold + nand_half_counter + nand_bold +
    nand_byte_counter + min_old + min_counter + max_old + max_counter +
    minu_old + minu_counter + maxu_old + maxu_counter + min_wold +
    min_word_counter + max_wold + max_word_counter + minu_wold +
    minu_word_counter + maxu_wold + maxu_word_counter +
    (unsigned long) half_ok + half_expected + cas_half_value +
    (unsigned long) byte_ok + byte_expected + cas_byte_value;
}
