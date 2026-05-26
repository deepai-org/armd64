__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  volatile double neg = -a0;
  double mag = a1 < 0.0 ? -a1 : a1;
  return neg + mag + a2;
}
