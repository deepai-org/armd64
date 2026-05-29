extern void *memchr(const void *, int, unsigned long);
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
extern char *strstr(const char *, const char *);

static char text[] = "prefix-polyglot-poly-end";

__attribute__((visibility("default")))
unsigned long poly_entry(void)
{
  void *(*volatile fn_memchr)(const void *, int, unsigned long) = memchr;
  char *(*volatile fn_strchr)(const char *, int) = strchr;
  char *(*volatile fn_strrchr)(const char *, int) = strrchr;
  char *(*volatile fn_strstr)(const char *, const char *) = strstr;

  if (fn_memchr(text, 'g', sizeof(text)) != text + 11)
    return 1;
  if (fn_memchr(text, 'g', 10) != 0)
    return 2;
  if (fn_strchr(text, 'p') != text)
    return 3;
  if (fn_strrchr(text, 'p') != text + 16)
    return 4;
  if (fn_strstr(text, "poly-end") != text + 16)
    return 5;
  if (fn_strstr(text, "missing") != 0)
    return 6;
  return 1468;
}
