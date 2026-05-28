typedef struct {
  unsigned int initialized;
  unsigned int signals;
  unsigned int broadcasts;
  unsigned char reserved[36];
} pthread_cond_t;

extern int pthread_cond_init(pthread_cond_t *cond, const void *attr);
extern int pthread_cond_destroy(pthread_cond_t *cond);
extern int pthread_cond_signal(pthread_cond_t *cond);
extern int pthread_cond_broadcast(pthread_cond_t *cond);

static pthread_cond_t cond;

unsigned long poly_entry(void)
{
  if (pthread_cond_init(&cond, 0) != 0)
    return 1;
  if (cond.initialized != 1)
    return 2;
  if (pthread_cond_signal(&cond) != 0)
    return 3;
  if (pthread_cond_signal(&cond) != 0)
    return 4;
  if (pthread_cond_broadcast(&cond) != 0)
    return 5;
  if (cond.signals != 2)
    return 6;
  if (cond.broadcasts != 1)
    return 7;
  if (pthread_cond_destroy(&cond) != 0)
    return 8;
  if (cond.initialized != 0)
    return 9;
  return 152;
}
