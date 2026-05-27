typedef unsigned long size_t;

extern size_t strlen(const char *);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  char buf[32];

  buf[0] = 'p';
  buf[1] = 'o';
  buf[2] = 'l';
  buf[3] = 'y';
  buf[4] = 0;

  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + strlen(buf);
}
