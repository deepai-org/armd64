typedef struct {
  unsigned int state;
  unsigned char reserved[40];
} pthread_mutex_t;

extern int pthread_mutex_init(pthread_mutex_t *mutex, const void *attr);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

static pthread_mutex_t mutex;
static unsigned long protected_value;

unsigned long poly_entry(void)
{
  if (pthread_mutex_init(&mutex, 0) != 0)
    return 1;
  if (pthread_mutex_lock(&mutex) != 0)
    return 2;
  if (pthread_mutex_trylock(&mutex) != 16)
    return 3;
  protected_value += 40;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 4;
  if (pthread_mutex_trylock(&mutex) != 0)
    return 5;
  protected_value += 58;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 6;
  if (pthread_mutex_destroy(&mutex) != 0)
    return 7;
  return protected_value;
}
