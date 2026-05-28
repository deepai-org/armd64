typedef unsigned long size_t;

struct sort_context {
  unsigned long calls;
  unsigned long bias;
};

extern void qsort_r(void *base, size_t nmemb, size_t size,
    int (*compar)(const void *, const void *, void *), void *arg);

static int compare_u64_context(const void *left_ptr, const void *right_ptr,
    void *arg)
{
  struct sort_context *context = (struct sort_context *) arg;
  unsigned long left = *(const unsigned long *) left_ptr + context->bias;
  unsigned long right = *(const unsigned long *) right_ptr + context->bias;
  context->calls++;
  if (left < right)
    return -1;
  if (left > right)
    return 1;
  return 0;
}

unsigned long poly_entry(void)
{
  unsigned long values[6] = { 14, 3, 9, 1, 7, 5 };
  struct sort_context context = { 0, 11 };
  qsort_r(values, 6, sizeof(values[0]), compare_u64_context, &context);

  static const unsigned long expected[6] = { 1, 3, 5, 7, 9, 14 };
  unsigned long checksum = 0;
  for (size_t n = 0; n < 6; n++) {
    if (values[n] != expected[n])
      return 1;
    checksum += values[n] * (n + 1);
  }
  if (context.calls == 0 || context.bias != 11)
    return 2;
  return checksum + context.calls;
}
