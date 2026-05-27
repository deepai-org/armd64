extern unsigned long poly_symbolic_target(void);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  return poly_symbolic_target();
}
