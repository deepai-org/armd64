struct __attribute__((packed)) packed_values {
  unsigned char tag;
  unsigned long qword;
  unsigned int word;
  signed short half;
  signed char byte;
};

static volatile struct packed_values store;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1, unsigned long a2,
    unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
    unsigned long a7, unsigned long a8)
{
  store.qword = a0 + a1;
  store.word = (unsigned int) (a2 + a3);
  store.half = (signed short) (a4 - 99);
  store.byte = (signed char) (a5 - 9);

  return store.qword + store.word + store.half + store.byte +
    a6 + a7 + a8;
}
