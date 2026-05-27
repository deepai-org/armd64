extern int posix_memalign(void **, unsigned long, unsigned long);
extern void *aligned_alloc(unsigned long, unsigned long);
extern void *memalign(unsigned long, unsigned long);
extern void free(void *);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  void *first_ptr = 0;
  if (posix_memalign(&first_ptr, 64, 48) != 0)
    return 1;
  if (((unsigned long) first_ptr & 63) != 0)
    return 2;
  unsigned char *first = (unsigned char *) first_ptr;
  first[0] = 13;
  first[47] = 17;

  unsigned char *second = (unsigned char *) aligned_alloc(128, 128);
  if (second == 0)
    return 3;
  if (((unsigned long) second & 127) != 0)
    return 4;
  second[0] = 19;
  second[127] = 23;

  unsigned char *third = (unsigned char *) memalign(256, 33);
  if (third == 0)
    return 5;
  if (((unsigned long) third & 255) != 0)
    return 6;
  third[0] = 29;
  third[32] = 31;

  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    first[0] + first[47] + second[0] + second[127] + third[0] + third[32];
  free(first);
  free(second);
  free(third);
  return result;
}
