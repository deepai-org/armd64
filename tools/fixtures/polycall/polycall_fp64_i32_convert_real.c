static volatile double table[4] = { 1.5, -2.25, 3.75, 4.5 };

__attribute__((visibility("default")))
long poly_entry(double a0, double a1, double a2)
{
  int i = ((int) a0) & 3;
  int j = ((int) a1) & 3;

  return (long) ((int) (table[i] + table[j] + a2));
}
