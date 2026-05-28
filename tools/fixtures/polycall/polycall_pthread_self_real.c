typedef unsigned long pthread_t;

extern pthread_t pthread_self(void);
extern int pthread_equal(pthread_t left, pthread_t right);

unsigned long poly_entry(void)
{
  pthread_t self = pthread_self();
  if (self == 0)
    return 1;
  if (pthread_equal(self, self) != 1)
    return 2;
  if (pthread_equal(self, self + 1) != 0)
    return 3;
  return 109;
}
