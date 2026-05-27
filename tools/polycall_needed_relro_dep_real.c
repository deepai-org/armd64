__attribute__((visibility("default"), noinline))
unsigned long poly_needed_relro_target(unsigned long value) {
  return value + 700;
}

__attribute__((visibility("default")))
unsigned long (* const poly_needed_relro_slot)(unsigned long) =
  poly_needed_relro_target;

__attribute__((visibility("default")))
unsigned long poly_needed_relro_value(void) {
  unsigned long (*fn)(unsigned long) = poly_needed_relro_slot;
  return fn(45);
}
