static unsigned long counter = 41;
static unsigned int word_counter = 9;
static unsigned int add_word_counter = 31;
static unsigned int swap_word_counter = 23;
static unsigned long and_counter = 0xff;
static unsigned long xor_counter = 0x55;
static unsigned long or_counter = 0x40;
static unsigned long sub_counter = 0xff;
static unsigned long nand_counter = 0xff;
static unsigned int and_word_counter = 0x7f;
static unsigned int xor_word_counter = 0x21;
static unsigned int or_word_counter = 0x12;
static unsigned int sub_word_counter = 0x7f;
static unsigned int nand_word_counter = 0x7f;
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
    nand_byte_counter + (unsigned long) half_ok + half_expected +
    cas_half_value + (unsigned long) byte_ok + byte_expected + cas_byte_value;
}
