static volatile float fp_bias = 1.0f;

float poly_entry(float a0, float a1, float a2)
{
  volatile float local = a0 + fp_bias;
  return (local - a1) * a2;
}
