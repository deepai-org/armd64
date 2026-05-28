extern unsigned long long poly_import_x86_sum14(unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long, unsigned long long, unsigned long long,
    unsigned long long);

unsigned long long poly_entry(unsigned long long a0, unsigned long long a1,
    unsigned long long a2, unsigned long long a3, unsigned long long a4,
    unsigned long long a5, unsigned long long a6, unsigned long long a7)
{
  return poly_import_x86_sum14(a0, a1, a2, a3, a4, a5, a6, a7,
    9, 10, 11, 12, 13, 14);
}
