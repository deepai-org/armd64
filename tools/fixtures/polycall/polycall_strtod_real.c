extern double strtod(const char *, char **);

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  const char first[] = "12.75xyz";
  char *end = 0;
  double value = strtod(first, &end);
  if (end != first + 5)
    return 1;
  if (value != 12.75)
    return 2;

  const char second[] = "-0.5!";
  end = 0;
  value = strtod(second, &end);
  if (end != second + 4)
    return 3;
  if (value != -0.5)
    return 4;

  return 1376;
}
