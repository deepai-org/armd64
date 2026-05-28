static volatile unsigned long udivisor64 = 3;
static volatile long sdivisor64 = -2;
static volatile unsigned int udivisor32 = 5;
static volatile int sdivisor32 = -3;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long unsigned64 = (a8 + 100UL) / udivisor64;
  long signed64 = ((long) a0 - 21L) / sdivisor64;
  unsigned int unsigned32 = ((unsigned int) a6 + 54U) / udivisor32;
  int signed32 = ((int) a7 - 29) / sdivisor32;

  return unsigned64 + (unsigned long) signed64 + unsigned32 +
    (unsigned int) signed32;
}
