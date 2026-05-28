#define POLY_STEP(value) \
  do { \
    unsigned long v = (value); \
    if ((v & 1) != 0) \
      total += v * 3; \
    else \
      total += v + 5; \
  } while (0)

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long total = 0;
  POLY_STEP(a0);
  POLY_STEP(a1);
  POLY_STEP(a2);
  POLY_STEP(a3);
  POLY_STEP(a4);
  POLY_STEP(a5);
  POLY_STEP(a6);
  POLY_STEP(a7);
  POLY_STEP(a8);
  return total;
}
