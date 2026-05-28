typedef struct {
  unsigned int state;
  unsigned int owner;
  unsigned int depth;
  unsigned int type;
  unsigned char reserved[32];
} pthread_mutex_t;

typedef struct {
  unsigned int type;
  unsigned char reserved[8];
} pthread_mutexattr_t;

enum {
  PTHREAD_MUTEX_NORMAL = 0,
  PTHREAD_MUTEX_RECURSIVE = 1
};

extern int pthread_mutexattr_init(pthread_mutexattr_t *attr);
extern int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
extern int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,
    int *type);
extern int pthread_mutex_init(pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

static pthread_mutex_t mutex;
static pthread_mutexattr_t attr;

unsigned long poly_entry(void)
{
  int type = 99;
  unsigned long value = 0;

  if (pthread_mutexattr_init(&attr) != 0)
    return 1;
  if (pthread_mutexattr_gettype(&attr, &type) != 0)
    return 2;
  if (type != PTHREAD_MUTEX_NORMAL)
    return 3;
  if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
    return 4;
  if (pthread_mutexattr_gettype(&attr, &type) != 0)
    return 5;
  if (type != PTHREAD_MUTEX_RECURSIVE)
    return 6;
  if (pthread_mutexattr_settype(&attr, 99) != 22)
    return 7;
  if (pthread_mutex_init(&mutex, &attr) != 0)
    return 8;
  if (pthread_mutex_lock(&mutex) != 0)
    return 9;
  if (pthread_mutex_lock(&mutex) != 0)
    return 10;
  if (pthread_mutex_trylock(&mutex) != 0)
    return 11;
  value += 132;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 12;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 13;
  if (pthread_mutex_unlock(&mutex) != 0)
    return 14;
  if (pthread_mutex_destroy(&mutex) != 0)
    return 15;
  if (pthread_mutexattr_destroy(&attr) != 0)
    return 16;
  return value;
}
