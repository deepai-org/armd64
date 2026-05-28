#include <stdint.h>

struct poly_hfa3_fp64 {
  double a;
  double b;
  double c;
};

struct poly_hfa4_fp64 {
  double a;
  double b;
  double c;
  double d;
};

struct poly_hfa3_fp32 {
  float a;
  float b;
  float c;
};

struct poly_hfa4_fp32 {
  float a;
  float b;
  float c;
  float d;
};

static uint64_t pack_f64(double value) {
  union {
    double d;
    uint64_t u;
  } bits;
  bits.d = value;
  return bits.u >> 48;
}

static uint64_t pack_f32(float value) {
  union {
    float f;
    uint32_t u;
  } bits;
  bits.f = value;
  return bits.u >> 16;
}

uint64_t poly_hfa3_f64_arg(struct poly_hfa3_fp64 arg) {
  return (pack_f64(arg.a) << 32) |
    (pack_f64(arg.b) << 16) |
    pack_f64(arg.c);
}

uint64_t poly_hfa4_f64_arg(struct poly_hfa4_fp64 arg) {
  return (pack_f64(arg.a) << 48) |
    (pack_f64(arg.b) << 32) |
    (pack_f64(arg.c) << 16) |
    pack_f64(arg.d);
}

uint64_t poly_hfa3_f32_arg(struct poly_hfa3_fp32 arg) {
  return (pack_f32(arg.a) << 32) |
    (pack_f32(arg.b) << 16) |
    pack_f32(arg.c);
}

uint64_t poly_hfa4_f32_arg(struct poly_hfa4_fp32 arg) {
  return (pack_f32(arg.a) << 48) |
    (pack_f32(arg.b) << 32) |
    (pack_f32(arg.c) << 16) |
    pack_f32(arg.d);
}
