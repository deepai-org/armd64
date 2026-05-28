extern __thread unsigned long poly_root_tls_counter;

__attribute__((visibility("default")))
unsigned long poly_needed_root_tls_add(unsigned long a, unsigned long b)
{
  poly_root_tls_counter += a + b;
  return poly_root_tls_counter + 300;
}
