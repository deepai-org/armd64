static unsigned long poly_preinit_state;

static void poly_preinit(void) {
  poly_preinit_state = 310;
}

static void (*const poly_preinit_slot)(void)
  __attribute__((section(".preinit_array"), used)) = poly_preinit;

unsigned long poly_entry(void) {
  return poly_preinit_state + 35;
}
