__thread unsigned long poly_needed_tls_counter = 500;

__attribute__((visibility("default")))
unsigned long poly_needed_tls_add(unsigned long a, unsigned long b)
{
  poly_needed_tls_counter += a + b;
  return poly_needed_tls_counter;
}
