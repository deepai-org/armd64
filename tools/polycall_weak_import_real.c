extern unsigned long poly_optional_value __attribute__((weak));
extern unsigned long poly_optional_add(unsigned long value) __attribute__((weak));

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1)
{
  unsigned long result = a0 + 7;

  if (&poly_optional_value)
    result += poly_optional_value;
  if (poly_optional_add)
    result += poly_optional_add(a1);

  return result;
}
