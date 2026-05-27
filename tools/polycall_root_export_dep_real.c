extern unsigned long poly_root_callback(unsigned long, unsigned long);

__attribute__((visibility("default")))
unsigned long poly_needed_root_call(unsigned long a, unsigned long b)
{
  return poly_root_callback(a, b) + 300;
}
