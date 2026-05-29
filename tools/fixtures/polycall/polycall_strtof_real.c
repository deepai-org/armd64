extern float strtof(const char *, char **);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const char first[] = "12.5xyz";
  char *end = 0;
  float value = strtof(first, &end);
  if (end != first + 4)
    return 1;
  if (value != 12.5f)
    return 2;

  const char second[] = "-0.25!";
  end = 0;
  value = strtof(second, &end);
  if (end != second + 5)
    return 3;
  if (value != -0.25f)
    return 4;

  return 1384;
}
