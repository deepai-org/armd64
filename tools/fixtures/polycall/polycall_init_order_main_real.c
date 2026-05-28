extern unsigned long poly_init_order_normal_touch(void);
extern unsigned long poly_init_order_first_touch(void);

__attribute__((visibility("default")))
unsigned long poly_init_order_state;

__attribute__((visibility("default")))
unsigned long poly_entry(void) {
  return poly_init_order_state +
    (poly_init_order_normal_touch() & 0) +
    (poly_init_order_first_touch() & 0);
}
