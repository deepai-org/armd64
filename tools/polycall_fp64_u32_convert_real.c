static volatile double table[4] = { 1.5, -2.25, 3.75, 4.5 };

__attribute__((visibility("default")))
unsigned long poly_entry(double a0, double a1, double a2)
{
  unsigned int i = ((unsigned int) a0) & 3;
  unsigned int j = ((unsigned int) a1) & 3;

  return (unsigned long) ((unsigned int) (table[i] + table[j] + a2));
}
