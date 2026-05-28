static volatile double dbias = 1.25;
static volatile float fbias = 2.5f;

__attribute__((visibility("default")))
double poly_entry(double a0, double a1, double a2)
{
  float narrowed = (float) (a0 + dbias);
  double widened = (double) ((float) a1 + fbias);

  return (double) narrowed + widened + a2;
}
