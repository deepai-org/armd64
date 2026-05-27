extern unsigned long poly_rpath_inherit_mid(void);

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  return poly_rpath_inherit_mid() + 5;
}
