typedef unsigned long size_t;

extern void qsort(void *base, size_t nmemb, size_t size,
    int (*compar)(const void *, const void *));

static int compare_u64(const void *left_ptr, const void *right_ptr)
{
  unsigned long left = *(const unsigned long *) left_ptr;
  unsigned long right = *(const unsigned long *) right_ptr;
  return (left > right) - (left < right);
}

__attribute__((noinline))
static void call_qsort(unsigned long *values, size_t count)
{
  qsort(values, count, sizeof(values[0]), compare_u64);
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  (void) a0;
  (void) a1;
  (void) a2;
  (void) a3;
  (void) a4;
  (void) a5;
  (void) a6;
  (void) a7;
  (void) a8;

  unsigned long values[6] = { 9, 1, 5, 3, 8, 2 };
  call_qsort(values, 6);

  unsigned long weighted = 0;
  for (size_t n = 0; n < 6; n++) {
    if (n != 0 && values[n - 1] > values[n])
      return 9000 + n;
    weighted += values[n] * (n + 1);
  }

  return weighted;
}
