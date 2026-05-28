__attribute__((visibility("default")))
float poly_entry(float a0, float a1, float a2)
{
  return a0 < a1 ? a1 : a2;
}
