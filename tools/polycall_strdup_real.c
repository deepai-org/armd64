extern char *strdup(const char *);
extern char *strndup(const char *, unsigned long);
extern void free(void *);

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
  char *first = strdup("poly");
  char *second = strndup("architecture", 4);
  if (first == 0 || second == 0)
    return 1;

  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    sum_bytes(first) + sum_bytes(second);
  free(first);
  free(second);
  return result;
}
