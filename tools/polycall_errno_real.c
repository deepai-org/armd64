extern int *__errno_location(void);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  int *errp = __errno_location();

  *errp = (int) (a0 + a1 + 17);
  return (unsigned long) *__errno_location() + a8;
}
