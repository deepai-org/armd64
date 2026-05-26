static signed char signed_bytes[2] = { -5, 6 };
static signed short signed_halves[4] = { -300, 400, -500, 600 };
static signed int signed_words[4] = { -70000, 80000, -90000, 100000 };

static signed char * volatile signed_byte_ptr = signed_bytes;
static signed short * volatile signed_half_ptr = signed_halves;
static signed int * volatile signed_word_ptr = signed_words;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1, unsigned long a2,
    unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
    unsigned long a7, unsigned long a8)
{
  long byte_value = signed_byte_ptr[0];
  long half_value = signed_half_ptr[1];
  long word_value = signed_word_ptr[2];

  return (unsigned long) (byte_value + half_value + word_value +
    (long) a0 + (long) a1 + (long) a2 + (long) a3 + (long) a4 +
    (long) a5 + (long) a6 + (long) a7 + (long) a8);
}
