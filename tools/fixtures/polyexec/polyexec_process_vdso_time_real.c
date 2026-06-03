#include <stdint.h>
#include <time.h>
#include <unistd.h>

static int write_all(const char *text) {
  const char *p = text;
  size_t len = 0;
  while (text[len])
    len++;
  while (len != 0) {
    ssize_t n = write(1, p, len);
    if (n <= 0)
      return -1;
    p += n;
    len -= (size_t) n;
  }
  return 0;
}

int main(void) {
  struct timespec prev = {0, 0};
  for (int iteration = 0; iteration < 64; iteration++) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
      return 10;
    if (iteration != 0 &&
        (now.tv_sec < prev.tv_sec ||
         (now.tv_sec == prev.tv_sec && now.tv_nsec < prev.tv_nsec)))
      return 11;
    prev = now;
  }
  if (write_all("POLY_VDSO_TIME_OK iterations=64\n") < 0)
    return 12;
  return 42;
}
