__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  double selected = a0 > a1 ? a0 : a1;
  return selected - a2;
}
