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
extern char *strrchr(const char *, int);
extern char *strstr(const char *, const char *);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern size_t strnlen(const char *, size_t);

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

__attribute__((noinline))
static char *call_strrchr(const char *base, int needle)
{
  return strrchr(base, needle);
}

__attribute__((noinline))
static char *call_strstr(const char *base, const char *needle)
{
  return strstr(base, needle);
}

__attribute__((noinline))
static char *call_strcpy(char *dest, const char *src)
{
  return strcpy(dest, src);
}

__attribute__((noinline))
static char *call_strncpy(char *dest, const char *src, size_t count)
{
  return strncpy(dest, src, count);
}

__attribute__((noinline))
static size_t call_strnlen(const char *base, size_t maxlen)
{
  return strnlen(base, maxlen);
}

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  char buffer[16];
  char overlap[16];
  char copied[16];
  char ncopy[16];

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
  memset(copied, 0, sizeof(copied));
  memset(ncopy, 0x55, sizeof(ncopy));
  memmove(overlap + 2, overlap, 5);
  size_t len = strlen(buffer);
  int string_same = call_strcmp(buffer, expected);
  int prefix_same = call_strncmp(buffer, "poly-z", 4);
  void *found = call_memchr(buffer, 'y', len);
  void *not_found = call_memchr(buffer, 'z', len);
  char *letter_found = call_strchr(buffer, 'l');
  char *terminator_found = call_strchr(buffer, 0);
  char *letter_not_found = call_strchr(buffer, 'z');
  char *last_letter_found = call_strrchr(buffer, 'l');
  char *last_char_found = call_strrchr(buffer, 'c');
  char *last_not_found = call_strrchr(buffer, 'z');
  char *substring_found = call_strstr(buffer, "ly-");
  char *suffix_found = call_strstr(buffer, "libc");
  char *substring_not_found = call_strstr(buffer, "z");
  char *copy_result = call_strcpy(copied, buffer);
  char *ncopy_result = call_strncpy(ncopy, "xy", 5);
  size_t bounded_len = call_strnlen(buffer, 4);
  size_t full_bounded_len = call_strnlen(buffer, 16);
  int same = memcmp(buffer, expected, sizeof(expected));
  int moved = memcmp(overlap, overlap_expected, sizeof(overlap_expected));
  int copied_same = memcmp(copied, expected, sizeof(expected));
  int ncopy_prefix_same = memcmp(ncopy, "xy\0\0\0", 5);
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
    (last_letter_found == buffer + 5 ? 1100 : 11000) +
    (last_char_found == buffer + 8 ? 1200 : 12000) +
    (last_not_found == 0 ? 1300 : 13000) +
    (substring_found == buffer + 2 ? 1400 : 14000) +
    (suffix_found == buffer + 5 ? 1500 : 15000) +
    (substring_not_found == 0 ? 1600 : 16000) +
    (copy_result == copied ? 1700 : 17000) +
    (copied_same == 0 ? 1800 : 18000) +
    (ncopy_result == ncopy ? 1900 : 19000) +
    (ncopy_prefix_same == 0 ? 2000 : 20000) +
    (ncopy[5] == 0x55 ? 2100 : 21000) +
    (bounded_len == 4 ? 2200 : 22000) +
    (full_bounded_len == len ? 2300 : 23000) +
    (unsigned char) buffer[0];
}
