static volatile double table[4] = { 1.5, -2.25, 3.75, 4.5 };

__attribute__((visibility("default")))
unsigned long poly_entry(double a0, double a1, double a2)
{
  unsigned long i = ((unsigned long) a0) & 3;
  unsigned long j = ((unsigned long) a1) & 3;

  return (unsigned long) (table[i] + table[j] + a2);
}
