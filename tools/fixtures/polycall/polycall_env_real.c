extern char *getenv(const char *);
extern char *secure_getenv(const char *);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;

  if (getenv("POLY_MISSING_ENV") == 0)
    result += 3;
  if (secure_getenv("POLY_MISSING_SECURE_ENV") == 0)
    result += 5;

  return result;
}
