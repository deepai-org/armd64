extern unsigned long poly_needed_relro_value(void);

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  return poly_needed_relro_value();
}
