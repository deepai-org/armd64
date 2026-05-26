__attribute__((visibility("default")))
float poly_entry(float a0, float a1, float a2)
{
  return __builtin_fmaxf(a0, a1) + __builtin_fminf(a1, a2);
}
