#include <stdint.h>

#if defined(__GNUC__)
#define POLY_HOST_HELPER __attribute__((noinline, noclone, used))
#else
#define POLY_HOST_HELPER
#endif

uint64_t POLY_HOST_HELPER poly_host_x86_add(uint64_t a, uint64_t b)
{
  return a + b + 200;
}

uint64_t POLY_HOST_HELPER poly_host_x86_mul(uint64_t a, uint64_t b)
{
  return a * b + 200;
}

uint64_t POLY_HOST_HELPER poly_host_x86_sum6(uint64_t a, uint64_t b,
    uint64_t c, uint64_t d, uint64_t e, uint64_t f)
{
  return a + b + c + d + e + f + 200;
}

uint64_t POLY_HOST_HELPER poly_host_x86_sum8(uint64_t a, uint64_t b,
    uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h)
{
  return a + b + c + d + e + f + g + h + 200;
}

double POLY_HOST_HELPER poly_host_x86_fp64_add(double a, double b)
{
  return a + b + 200.5;
}

float POLY_HOST_HELPER poly_host_x86_fp32_add(float a, float b)
{
  return a + b + 200.5f;
}
