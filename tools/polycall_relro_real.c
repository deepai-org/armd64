__attribute__((visibility("default"), noinline))
unsigned long poly_relro_target(unsigned long value) {
  return value + 700;
}

__attribute__((visibility("default")))
unsigned long (* const poly_relro_slot)(unsigned long) = poly_relro_target;

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  unsigned long (*fn)(unsigned long) = poly_relro_slot;
  return fn(45);
}
