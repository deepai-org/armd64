__attribute__((visibility("default")))
unsigned long poly_needed_interpose(void) {
  return 222;
}

__attribute__((visibility("default")))
unsigned long poly_needed_weak_dep(void) {
  return 17;
}
