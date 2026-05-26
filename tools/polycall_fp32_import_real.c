extern float poly_import_fp32_add(float, float);

__attribute__((visibility("default")))
float poly_entry(float a0, float a1, float a2)
{
  (void) a2;
  return poly_import_fp32_add(a0, a1);
}
