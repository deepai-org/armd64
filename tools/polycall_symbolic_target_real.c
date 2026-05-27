__attribute__((visibility("default"), noinline))
unsigned long poly_symbolic_value(void)
{
  return 10;
}

__attribute__((visibility("default"), noinline))
unsigned long poly_symbolic_target(void)
{
  return poly_symbolic_value() + 5;
}
