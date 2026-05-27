extern unsigned long poly_copy_value;

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  return poly_copy_value + 1;
}
