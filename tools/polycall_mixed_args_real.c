__attribute__((visibility("default")))
double poly_entry(unsigned long a0, double f0, unsigned long a1,
    double f1, unsigned long a2, double f2)
{
  return (double) (a0 * 10 + a1 * 100 + a2 * 1000) + f0 + f1 * 2.0 + f2 * 3.0;
}
