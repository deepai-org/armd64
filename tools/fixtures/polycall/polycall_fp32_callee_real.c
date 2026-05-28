extern float poly_import_fp32_add(float, float);

__attribute__((visibility("default")))
float poly_entry(float a0, float a1, float a2)
{
  float sum01 = a0 + a1;
  float sum12 = a1 + a2;
  float sum02 = a0 + a2;
  float imported = poly_import_fp32_add(sum01, sum12);

  return imported + sum01 + sum12 + sum02;
}
