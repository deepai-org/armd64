static volatile double table[4] = { 1.5, -2.25, 3.75, 4.5 };

__attribute__((visibility("default")))
long poly_entry(double a0, double a1, double a2)
{
  long i = ((long) a0) & 3;
  long j = ((long) a1) & 3;

  return (long) (table[i] - table[j] - a2) + 13;
}
