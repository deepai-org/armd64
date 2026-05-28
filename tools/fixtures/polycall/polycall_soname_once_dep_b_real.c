extern unsigned long poly_soname_once_leaf(void);

__attribute__((visibility("default")))
unsigned long poly_soname_once_b(void) {
  return poly_soname_once_leaf() + 20;
}
