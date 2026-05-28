#ifndef POLY_MANY_NEEDED_INDEX
#define POLY_MANY_NEEDED_INDEX 0
#endif

#define POLY_CAT2(a, b) a##b
#define POLY_CAT(a, b) POLY_CAT2(a, b)

__attribute__((visibility("default")))
unsigned long POLY_CAT(poly_many_needed_value_, POLY_MANY_NEEDED_INDEX)(void)
{
  return POLY_MANY_NEEDED_INDEX * 100UL;
}
