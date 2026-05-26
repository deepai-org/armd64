typedef unsigned long size_t;

extern size_t strlen(const char *);
extern int strcmp(const char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memmove(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);
extern void *memchr(const void *, int, size_t);
extern void *memrchr(const void *, int, size_t);
extern void *memmem(const void *, size_t, const void *, size_t);
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
extern char *strstr(const char *, const char *);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *, const char *, size_t);
extern size_t strnlen(const char *, size_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *, const char *, size_t);
extern size_t strspn(const char *, const char *);
extern size_t strcspn(const char *, const char *);
extern char *strpbrk(const char *, const char *);
extern char *stpcpy(char *, const char *);
extern char *stpncpy(char *, const char *, size_t);
extern void *mempcpy(void *, const void *, size_t);
extern void *rawmemchr(const void *, int);
extern char *strchrnul(const char *, int);
extern int bcmp(const void *, const void *, size_t);
extern void bcopy(const void *, void *, size_t);
extern void bzero(void *, size_t);
extern char *index(const char *, int);
extern char *rindex(const char *, int);

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
static void *call_memrchr(const void *base, int needle, size_t count)
{
  return memrchr(base, needle, count);
}

__attribute__((noinline))
static void *call_memmem(const void *haystack, size_t haystack_len,
    const void *needle, size_t needle_len)
{
  return memmem(haystack, haystack_len, needle, needle_len);
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

__attribute__((noinline))
static char *call_strcat(char *dest, const char *src)
{
  return strcat(dest, src);
}

__attribute__((noinline))
static char *call_strncat(char *dest, const char *src, size_t count)
{
  return strncat(dest, src, count);
}

__attribute__((noinline))
static size_t call_strspn(const char *base, const char *accept)
{
  return strspn(base, accept);
}

__attribute__((noinline))
static size_t call_strcspn(const char *base, const char *reject)
{
  return strcspn(base, reject);
}

__attribute__((noinline))
static char *call_strpbrk(const char *base, const char *accept)
{
  return strpbrk(base, accept);
}

__attribute__((noinline))
static char *call_stpcpy(char *dest, const char *src)
{
  return stpcpy(dest, src);
}

__attribute__((noinline))
static char *call_stpncpy(char *dest, const char *src, size_t count)
{
  return stpncpy(dest, src, count);
}

__attribute__((noinline))
static void *call_mempcpy(void *dest, const void *src, size_t count)
{
  return mempcpy(dest, src, count);
}

__attribute__((noinline))
static void *call_rawmemchr(const void *base, int needle)
{
  return rawmemchr(base, needle);
}

__attribute__((noinline))
static char *call_strchrnul(const char *base, int needle)
{
  return strchrnul(base, needle);
}

__attribute__((noinline))
static int call_bcmp(const void *left, const void *right, size_t count)
{
  return bcmp(left, right, count);
}

__attribute__((noinline))
static void call_bcopy(const void *src, void *dest, size_t count)
{
  bcopy(src, dest, count);
}

__attribute__((noinline))
static void call_bzero(void *dest, size_t count)
{
  bzero(dest, count);
}

__attribute__((noinline))
static char *call_index(const char *base, int needle)
{
  return index(base, needle);
}

__attribute__((noinline))
static char *call_rindex(const char *base, int needle)
{
  return rindex(base, needle);
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
  char appended[16];
  char nappended[16];
  char stpcopied[16];
  char stpncopied[16];
  char mempcopied[16];
  char bcopied[16];
  char bzeroed[16];

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
  memset(appended, 0, sizeof(appended));
  memset(nappended, 0, sizeof(nappended));
  memset(stpcopied, 0, sizeof(stpcopied));
  memset(stpncopied, 0x55, sizeof(stpncopied));
  memset(mempcopied, 0, sizeof(mempcopied));
  memset(bcopied, 0, sizeof(bcopied));
  memset(bzeroed, 0x33, sizeof(bzeroed));
  memcpy(appended, "poly", 5);
  memcpy(nappended, "hi", 3);
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
  char *append_result = call_strcat(appended, "-cat");
  char *nappend_result = call_strncat(nappended, "there", 2);
  size_t span = call_strspn(buffer, "poly-");
  size_t cspan = call_strcspn(buffer, "-");
  char *break_found = call_strpbrk(buffer, "-x");
  char *break_not_found = call_strpbrk(buffer, "z");
  char *stpcopy_result = call_stpcpy(stpcopied, buffer);
  char *stpncopy_result = call_stpncpy(stpncopied, "uv", 5);
  void *mempcopy_result = call_mempcpy(mempcopied, buffer, sizeof(expected));
  void *raw_found = call_rawmemchr(buffer, '-');
  char *nul_found = call_strchrnul(buffer, 'c');
  char *nul_not_found = call_strchrnul(buffer, 'z');
  int bcmp_same = call_bcmp(buffer, expected, sizeof(expected));
  int bcmp_different = call_bcmp(buffer, "poly-z", 6);
  call_bcopy(buffer, bcopied, sizeof(expected));
  call_bzero(bzeroed + 2, 4);
  char *index_found = call_index(buffer, 'l');
  char *rindex_found = call_rindex(buffer, 'l');
  char *rindex_not_found = call_rindex(buffer, 'z');
  void *last_mem_found = call_memrchr(buffer, 'l', len);
  void *last_mem_not_found = call_memrchr(buffer, 'z', len);
  void *memory_substring_found = call_memmem(buffer, len, "y-l", 3);
  void *empty_memory_found = call_memmem(buffer, len, "", 0);
  int same = memcmp(buffer, expected, sizeof(expected));
  int moved = memcmp(overlap, overlap_expected, sizeof(overlap_expected));
  int copied_same = memcmp(copied, expected, sizeof(expected));
  int ncopy_prefix_same = memcmp(ncopy, "xy\0\0\0", 5);
  int appended_same = memcmp(appended, "poly-cat", 9);
  int nappended_same = memcmp(nappended, "hith\0", 5);
  int stpcopied_same = memcmp(stpcopied, expected, sizeof(expected));
  int stpncopied_same = memcmp(stpncopied, "uv\0\0\0", 5);
  int mempcopied_same = memcmp(mempcopied, expected, sizeof(expected));
  int bcopied_same = memcmp(bcopied, expected, sizeof(expected));
  int bzeroed_same = memcmp(bzeroed + 2, "\0\0\0\0", 4);
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
    (append_result == appended ? 2400 : 24000) +
    (appended_same == 0 ? 2500 : 25000) +
    (nappend_result == nappended ? 2600 : 26000) +
    (nappended_same == 0 ? 2700 : 27000) +
    (span == 6 ? 2800 : 28000) +
    (cspan == 4 ? 2900 : 29000) +
    (break_found == buffer + 4 ? 3000 : 30000) +
    (break_not_found == 0 ? 3100 : 31000) +
    (stpcopy_result == stpcopied + len ? 3200 : 32000) +
    (stpcopied_same == 0 ? 3300 : 33000) +
    (stpncopy_result == stpncopied + 2 ? 3400 : 34000) +
    (stpncopied_same == 0 ? 3500 : 35000) +
    (stpncopied[5] == 0x55 ? 3600 : 36000) +
    (mempcopy_result == mempcopied + sizeof(expected) ? 3700 : 37000) +
    (mempcopied_same == 0 ? 3800 : 38000) +
    (raw_found == buffer + 4 ? 3900 : 39000) +
    (nul_found == buffer + 8 ? 4000 : 40000) +
    (nul_not_found == buffer + len ? 4100 : 41000) +
    (bcmp_same == 0 ? 4200 : 42000) +
    (bcmp_different != 0 ? 4300 : 43000) +
    (bcopied_same == 0 ? 4400 : 44000) +
    (bzeroed[1] == 0x33 ? 4500 : 45000) +
    (bzeroed_same == 0 ? 4600 : 46000) +
    (bzeroed[6] == 0x33 ? 4700 : 47000) +
    (index_found == buffer + 2 ? 4800 : 48000) +
    (rindex_found == buffer + 5 ? 4900 : 49000) +
    (rindex_not_found == 0 ? 5000 : 50000) +
    (last_mem_found == buffer + 5 ? 5100 : 51000) +
    (last_mem_not_found == 0 ? 5200 : 52000) +
    (memory_substring_found == buffer + 3 ? 5300 : 53000) +
    (empty_memory_found == buffer ? 5400 : 54000) +
    (unsigned char) buffer[0];
}
