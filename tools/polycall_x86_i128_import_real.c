extern unsigned __int128 poly_import_x86_i128(unsigned long long,
    unsigned long long);

unsigned long long poly_entry(void)
{
  unsigned __int128 result = poly_import_x86_i128(0x1111, 0x2222);
  unsigned long long lo = (unsigned long long) result;
  unsigned long long hi = (unsigned long long) (result >> 64);
  return ((hi & 0xffff) << 16) | (lo & 0xffff);
}
