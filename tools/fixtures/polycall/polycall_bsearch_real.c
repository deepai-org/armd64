typedef unsigned long size_t;

extern void *bsearch(const void *key, const void *base, size_t nmemb,
    size_t size, int (*compar)(const void *, const void *));

static int compare_u64(const void *left_ptr, const void *right_ptr)
{
  unsigned long left = *(const unsigned long *) left_ptr;
  unsigned long right = *(const unsigned long *) right_ptr;
  if (left < right)
    return -1;
  if (left > right)
    return 1;
  return 0;
}

static unsigned long find_value(unsigned long *values, size_t count,
    unsigned long needle)
{
  unsigned long *found = (unsigned long *) bsearch(&needle, values, count,
    sizeof(values[0]), compare_u64);
  if (found == 0)
    return 0;
  return *found + (unsigned long) (found - values) * 31;
}

unsigned long poly_entry(void)
{
  unsigned long values[7] = { 2, 4, 6, 9, 12, 18, 21 };
  unsigned long found = find_value(values, 7, 12);
  unsigned long missing = find_value(values, 7, 5);
  if (found != 136 || missing != 0)
    return 1;
  return found;
}
