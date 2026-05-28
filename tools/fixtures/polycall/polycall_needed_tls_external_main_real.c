extern __thread unsigned long poly_needed_tls_counter;
extern unsigned long poly_needed_tls_add(unsigned long, unsigned long);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long before = poly_needed_tls_counter;
  unsigned long after = poly_needed_tls_add(a0, a1);
  return before + after + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}
