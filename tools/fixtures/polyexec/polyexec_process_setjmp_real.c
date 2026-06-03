#include <setjmp.h>

static jmp_buf poly_jump_target;
static volatile int poly_guard;

static int jump_from_leaf(int value) {
  if (value == 3)
    longjmp(poly_jump_target, 29);
  return value;
}

int main(void) {
  volatile int stack_value = 13;
  int jump_value = setjmp(poly_jump_target);

  if (jump_value == 0) {
    poly_guard = 5;
    (void) jump_from_leaf(3);
    return 1;
  }

  if (poly_guard != 5 || stack_value != 13)
    return 2;
  return jump_value + stack_value;
}
