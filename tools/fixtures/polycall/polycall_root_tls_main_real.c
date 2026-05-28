extern unsigned long poly_needed_root_tls_add(unsigned long, unsigned long);

__thread __attribute__((visibility("default")))
unsigned long poly_root_tls_counter = 1000;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  const unsigned long after = poly_needed_root_tls_add(a0, a1);
  return after + poly_root_tls_counter + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}
