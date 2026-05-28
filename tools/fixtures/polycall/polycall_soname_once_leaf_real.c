#ifndef POLY_SONAME_ONCE_VALUE
#define POLY_SONAME_ONCE_VALUE 100
#endif

__attribute__((visibility("default")))
unsigned long poly_soname_once_leaf(void) {
  return POLY_SONAME_ONCE_VALUE;
}
