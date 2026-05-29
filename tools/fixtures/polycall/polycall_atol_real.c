extern long atol(const char *);
extern long long atoll(const char *);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  long (*volatile fn_atol)(const char *) = atol;
  long long (*volatile fn_atoll)(const char *) = atoll;

  if (fn_atol(" -123456789 rest") != -123456789L)
    return 1;
  if (fn_atol("55") != 55L)
    return 2;
  if (fn_atoll(" -1234567890123 rest") != -1234567890123LL)
    return 3;
  if (fn_atoll("77") != 77LL)
    return 4;
  return 1532;
}
