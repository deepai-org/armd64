extern float poly_import_x86_fp32_add(float, float);

float poly_entry(float a0, float a1, float a2)
{
  return poly_import_x86_fp32_add(a0, a1) + a2;
}
