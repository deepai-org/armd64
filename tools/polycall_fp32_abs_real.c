__attribute__((visibility("default")))
float poly_entry(float a0, float a1, float a2)
{
  return __builtin_fabsf(a0) + __builtin_fabsf(a1) + a2;
}
