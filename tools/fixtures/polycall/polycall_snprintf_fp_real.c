typedef unsigned long size_t;

extern int snprintf(char *, size_t, const char *, ...);
extern int strcmp(const char *, const char *);

static unsigned long sum_bytes(const char *value)
{
  unsigned long result = 0;
  for (unsigned long n = 0; value[n] != 0; n++)
    result += (unsigned char) value[n];
  return result;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  char buffer[64];
  int written = snprintf(buffer, sizeof(buffer), "%s-%lu-%.2f-%lu",
    "poly", a0 + a1 + a2, 2.25, a8);
  if (written != 13 || strcmp(buffer, "poly-6-2.25-9") != 0)
    return 1;
  if (a0 != 1)
    return 100;
  if (a1 != 2)
    return 101;
  if (a2 != 3)
    return 102;
  if (a3 != 4)
    return 103;
  if (a4 != 5)
    return 104;
  if (a5 != 6)
    return 105;
  if (a6 != 7)
    return 106;
  if (a7 != 8)
    return 107;
  if (a8 != 9)
    return 108;
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    (unsigned long) written + sum_bytes(buffer);
}
