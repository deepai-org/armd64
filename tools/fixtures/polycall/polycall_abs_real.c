extern int abs(int);
extern long labs(long);
extern long long llabs(long long);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  int (*volatile fn_abs)(int) = abs;
  long (*volatile fn_labs)(long) = labs;
  long long (*volatile fn_llabs)(long long) = llabs;

  if (fn_abs(-37) != 37 || fn_abs(12) != 12)
    return 1;
  if (fn_labs(-123456789L) != 123456789L || fn_labs(55L) != 55L)
    return 2;
  if (fn_llabs(-1234567890123LL) != 1234567890123LL ||
      fn_llabs(77LL) != 77LL)
    return 3;
  return 1524;
}
