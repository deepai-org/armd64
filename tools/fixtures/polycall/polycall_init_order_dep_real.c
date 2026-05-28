extern unsigned long poly_init_order_state;

#ifndef POLY_INIT_ORDER_DIGIT
#define POLY_INIT_ORDER_DIGIT 0
#endif

#ifndef POLY_INIT_ORDER_FUNC
#define POLY_INIT_ORDER_FUNC poly_init_order_touch
#endif

__attribute__((constructor))
static void poly_init_order_ctor(void) {
  poly_init_order_state =
    poly_init_order_state * 10 + POLY_INIT_ORDER_DIGIT;
}

unsigned long POLY_INIT_ORDER_FUNC(void) {
  return poly_init_order_state;
}
