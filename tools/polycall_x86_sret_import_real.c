struct poly_x86_sret_u64 {
  unsigned long a;
  unsigned long b;
  unsigned long c;
  unsigned long d;
};

extern struct poly_x86_sret_u64 poly_import_x86_sret_u64(unsigned long,
    unsigned long, unsigned long);

unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7)
{
  (void) a3;
  (void) a4;
  (void) a5;
  (void) a6;
  (void) a7;

  struct poly_x86_sret_u64 result =
    poly_import_x86_sret_u64(a0, a1, a2);
  return ((result.a & 0xffff) << 48) |
    ((result.b & 0xffff) << 32) |
    ((result.c & 0xffff) << 16) |
    (result.d & 0xffff);
}
