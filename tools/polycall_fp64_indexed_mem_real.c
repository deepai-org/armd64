static volatile unsigned long load_index = 2;
static volatile unsigned long store_index = 0;
static volatile double table[4] = { 1.5, -2.25, 3.75, 4.5 };

__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  unsigned long i = load_index & 3;
  unsigned long j = store_index & 3;
  (void) a2;

  table[j] = a0 + a1;
  return table[i] + table[j];
}
