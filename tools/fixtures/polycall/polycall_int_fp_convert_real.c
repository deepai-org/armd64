static volatile int sint32 = -3;
static volatile long sint64 = -7;
static volatile unsigned long uint64v = 11;

__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  float narrowed = (float) sint32;

  (void) a1;
  (void) a2;
  return (double) sint64 + (double) uint64v + (double) narrowed + a0;
}
