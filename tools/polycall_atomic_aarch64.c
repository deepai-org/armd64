static unsigned long counter = 41;
static unsigned int word_counter = 9;
static unsigned int add_word_counter = 31;
static unsigned int swap_word_counter = 23;
static unsigned long and_counter = 0xff;
static unsigned long xor_counter = 0x55;
static unsigned int and_word_counter = 0x7f;
static unsigned int xor_word_counter = 0x21;
static unsigned long cas_value = 99;
static unsigned int cas_word_value = 19;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a, unsigned long b, unsigned long c)
{
  unsigned long old = __atomic_fetch_add(&counter, a + 1, __ATOMIC_ACQ_REL);
  unsigned long prev = __atomic_exchange_n(&counter, old ^ b,
      __ATOMIC_SEQ_CST);
  unsigned int wold = __atomic_fetch_or(&word_counter, (unsigned int) c,
      __ATOMIC_RELAXED);
  unsigned int add_wold = __atomic_fetch_add(&add_word_counter,
      (unsigned int) c + 4, __ATOMIC_ACQ_REL);
  unsigned int swap_wold = __atomic_exchange_n(&swap_word_counter,
      (unsigned int) c + 30, __ATOMIC_SEQ_CST);
  unsigned long and_old = __atomic_fetch_and(&and_counter, a + 0x0f,
      __ATOMIC_ACQ_REL);
  unsigned long xor_old = __atomic_fetch_xor(&xor_counter, b + 0x10,
      __ATOMIC_ACQ_REL);
  unsigned int and_wold = __atomic_fetch_and(&and_word_counter,
      (unsigned int) c + 0x10, __ATOMIC_ACQ_REL);
  unsigned int xor_wold = __atomic_fetch_xor(&xor_word_counter,
      (unsigned int) c + 0x20, __ATOMIC_ACQ_REL);
  unsigned long expected = 99;
  int ok = __atomic_compare_exchange_n(&cas_value, &expected, c + 100, 0,
      __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
  unsigned int word_expected = 19;
  int word_ok = __atomic_compare_exchange_n(&cas_word_value, &word_expected,
      (unsigned int) c + 20, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);

  return old + prev + counter + word_counter + wold + cas_value +
    (unsigned long) ok + expected + (unsigned long) word_ok +
    word_expected + cas_word_value + add_wold + add_word_counter +
    swap_wold + swap_word_counter + and_old + and_counter + xor_old +
    xor_counter + and_wold + and_word_counter + xor_wold + xor_word_counter;
}
