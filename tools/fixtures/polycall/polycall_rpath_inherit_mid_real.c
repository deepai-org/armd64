extern unsigned long poly_rpath_inherit_leaf(void);

__attribute__((visibility("default")))
unsigned long poly_rpath_inherit_mid(void) {
  return poly_rpath_inherit_leaf() + 40;
}
