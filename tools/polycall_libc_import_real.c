typedef unsigned long size_t;

extern size_t strlen(const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memmove(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);
extern void *memchr(const void *, int, size_t);
extern char *strchr(const char *, int);

static const char source[] = "poly-libc";
static const char expected[] = "poly-libc";
static const char overlap_source[] = "abcdef";
static const char overlap_expected[] = "ababcde";

__attribute__((noinline))
static int call_strcmp(const char *left, const char *right)
{
  return strcmp(left, right);
}

__attribute__((noinline))
static int call_strncmp(const char *left, const char *right, size_t count)
{
  return strncmp(left, right, count);
}

__attribute__((noinline))
static void *call_memchr(const void *base, int needle, size_t count)
{
  return memchr(base, needle, count);
}

__attribute__((noinline))
static char *call_strchr(const char *base, int needle)
{
  return strchr(base, needle);
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  char buffer[16];
  char overlap[16];

  (void) a0;
  (void) a1;
  (void) a2;
  (void) a3;
  (void) a4;
  (void) a5;
  (void) a6;
  (void) a7;
  (void) a8;

  memset(buffer, 0x41, sizeof(buffer));
  memcpy(buffer, source, sizeof(source));
  memset(overlap, 0, sizeof(overlap));
  memcpy(overlap, overlap_source, sizeof(overlap_source));
  memmove(overlap + 2, overlap, 5);
  size_t len = strlen(buffer);
  int string_same = call_strcmp(buffer, expected);
  int prefix_same = call_strncmp(buffer, "poly-z", 4);
  void *found = call_memchr(buffer, 'y', len);
  void *not_found = call_memchr(buffer, 'z', len);
  char *letter_found = call_strchr(buffer, 'l');
  char *terminator_found = call_strchr(buffer, 0);
  char *letter_not_found = call_strchr(buffer, 'z');
  int same = memcmp(buffer, expected, sizeof(expected));
  int moved = memcmp(overlap, overlap_expected, sizeof(overlap_expected));
  buffer[4] = 'X';
  int different = memcmp(buffer, expected, sizeof(expected));

  return len + (same == 0 ? 100 : 1000) +
    (different > 0 ? 200 : 2000) + (moved == 0 ? 300 : 3000) +
    (string_same == 0 ? 400 : 4000) +
    (prefix_same == 0 ? 500 : 5000) +
    (found == buffer + 3 ? 600 : 6000) +
    (not_found == 0 ? 700 : 7000) +
    (letter_found == buffer + 2 ? 800 : 8000) +
    (terminator_found == buffer + len ? 900 : 9000) +
    (letter_not_found == 0 ? 1000 : 10000) +
    (unsigned char) buffer[0];
}
