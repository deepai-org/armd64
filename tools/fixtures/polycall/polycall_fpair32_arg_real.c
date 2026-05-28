struct pair_fp32 {
  float lo;
  float hi;
};

__attribute__((visibility("default")))
float poly_entry(struct pair_fp32 pair, float scale)
{
  return (pair.lo + pair.hi) * scale;
}
