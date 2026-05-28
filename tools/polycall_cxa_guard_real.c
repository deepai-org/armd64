extern int __cxa_guard_acquire(unsigned long long *);
extern void __cxa_guard_release(unsigned long long *);
extern void __cxa_guard_abort(unsigned long long *);

static unsigned long long guard_value;
static unsigned long long aborted_guard_value;
static unsigned long initialized_value;

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;

  if (__cxa_guard_acquire(&guard_value)) {
    initialized_value = 33;
    result += 5;
    __cxa_guard_release(&guard_value);
  }
  if (__cxa_guard_acquire(&guard_value)) {
    initialized_value = 1000;
    result += 500;
    __cxa_guard_release(&guard_value);
  }

  if (__cxa_guard_acquire(&aborted_guard_value)) {
    result += 7;
    __cxa_guard_abort(&aborted_guard_value);
  }
  if (__cxa_guard_acquire(&aborted_guard_value)) {
    result += 11;
    __cxa_guard_release(&aborted_guard_value);
  }
  if (__cxa_guard_acquire(&aborted_guard_value))
    result += 100;

  return result + initialized_value;
}
