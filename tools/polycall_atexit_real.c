extern int atexit(void (*)(void));
extern int __cxa_atexit(void (*)(void *), void *, void *);
extern void __cxa_finalize(void *);

static unsigned long callback_count;

static void plain_callback(void)
{
  callback_count += 1000;
}

static void cxa_callback(void *arg)
{
  callback_count += (unsigned long) arg;
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  int first = atexit(plain_callback);
  int second = __cxa_atexit(cxa_callback, (void *) 77, 0);
  __cxa_finalize(0);
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    (unsigned long) first + (unsigned long) second + callback_count;
}
