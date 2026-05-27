#ifndef POLY_RPATH_INHERIT_VALUE
#define POLY_RPATH_INHERIT_VALUE 700
#endif

__attribute__((visibility("default")))
unsigned long poly_rpath_inherit_leaf(void) {
  return POLY_RPATH_INHERIT_VALUE;
}
