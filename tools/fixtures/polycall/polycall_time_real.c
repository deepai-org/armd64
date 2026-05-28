typedef long int64_t;

struct timespec {
  int64_t tv_sec;
  int64_t tv_nsec;
};

struct timeval {
  int64_t tv_sec;
  int64_t tv_usec;
};

int clock_gettime(int clock_id, struct timespec *tp);
int clock_getres(int clock_id, struct timespec *tp);
int64_t time(int64_t *out);
int gettimeofday(struct timeval *tv, void *tz);
int64_t clock(void);

int poly_entry(void)
{
  struct timespec now;
  struct timespec res;
  struct timeval tv;
  int64_t stored = 0;

  if (clock_gettime(0, &now) != 0)
    return 1;
  if (now.tv_sec != 1712345678 || now.tv_nsec != 123456789)
    return 2;

  if (clock_getres(1, &res) != 0)
    return 3;
  if (res.tv_sec != 0 || res.tv_nsec != 1)
    return 4;

  if (time(&stored) != 1712345678 || stored != 1712345678)
    return 5;
  if (time(0) != 1712345678)
    return 6;

  if (gettimeofday(&tv, 0) != 0)
    return 7;
  if (tv.tv_sec != 1712345678 || tv.tv_usec != 654321)
    return 8;

  if (clock() != 123456)
    return 9;

  return 176;
}
