extern int ffs(int);
extern int ffsl(long);
extern int ffsll(long long);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  int (*volatile fn_ffs)(int) = ffs;
  int (*volatile fn_ffsl)(long) = ffsl;
  int (*volatile fn_ffsll)(long long) = ffsll;

  if (fn_ffs(0) != 0)
    return 1;
  if (fn_ffs(0x10) != 5)
    return 2;
  if (fn_ffs(-8) != 4)
    return 3;
  if (fn_ffsl(1L << 40) != 41)
    return 4;
  if (fn_ffsl(0) != 0)
    return 5;
  if (fn_ffsll(1LL << 52) != 53)
    return 6;
  if (fn_ffsll(0) != 0)
    return 7;

  return 1564;
}
