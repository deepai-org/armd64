extern void *malloc(unsigned long);
extern void *calloc(unsigned long, unsigned long);
extern void *realloc(void *, unsigned long);
extern void free(void *);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned char *first = (unsigned char *) malloc(24);
  if (first == 0)
    return 1;
  first[0] = 11;
  first[23] = 22;

  unsigned char *second = (unsigned char *) calloc(4, 8);
  if (second == 0)
    return 2;
  if (second[0] != 0 || second[31] != 0)
    return 3;
  second[3] = 5;

  unsigned char *grown = (unsigned char *) realloc(first, 40);
  if (grown == 0)
    return 4;
  if (grown[0] != 11 || grown[23] != 22)
    return 5;
  grown[39] = 7;

  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    grown[0] + grown[23] + second[3] + grown[39];
  free(second);
  free(grown);
  return result;
}
