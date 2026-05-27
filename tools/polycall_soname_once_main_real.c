extern unsigned long poly_soname_once_a(void);
extern unsigned long poly_soname_once_b(void);

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  return poly_soname_once_a() + poly_soname_once_b() + 5;
}
