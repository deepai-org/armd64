#define POLY_SC_PAGESIZE 30

extern long sysconf(int);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    (unsigned long) sysconf(POLY_SC_PAGESIZE);
}
