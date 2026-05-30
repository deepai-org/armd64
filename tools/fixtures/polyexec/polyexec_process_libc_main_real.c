#include <stddef.h>
#include <string.h>

static volatile char poly_libc_text[] = "polyglot-libc";

int main(int argc, char **argv)
{
  if (argc < 1 || argv == NULL || argv[0] == NULL)
    return 41;

  size_t len = strlen((const char *) poly_libc_text);
  if (len != 13)
    return 43;

  return 42;
}
