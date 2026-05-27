extern unsigned long poly_root_weak_value __attribute__((weak));
extern unsigned long poly_root_weak_add(unsigned long) __attribute__((weak));

__attribute__((visibility("default")))
unsigned long poly_needed_root_weak(unsigned long a, unsigned long b)
{
  unsigned long result = 10;
  if (&poly_root_weak_value)
    result += poly_root_weak_value;
  if (poly_root_weak_add)
    result += poly_root_weak_add(a + b);
  return result;
}
