struct poly_x86_sret_u64 {
  unsigned long a;
  unsigned long b;
  unsigned long c;
  unsigned long d;
};

extern struct poly_x86_sret_u64 poly_import_x86_sret_u64_stack10(
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long);

unsigned long poly_entry(void)
{
  struct poly_x86_sret_u64 result =
    poly_import_x86_sret_u64_stack10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  return ((result.a & 0xffff) << 48) |
    ((result.b & 0xffff) << 32) |
    ((result.c & 0xffff) << 16) |
    (result.d & 0xffff);
}
