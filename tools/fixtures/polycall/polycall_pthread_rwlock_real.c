typedef struct {
  unsigned int state;
  unsigned char reserved[56];
} pthread_rwlock_t;

extern int pthread_rwlock_init(pthread_rwlock_t *lock, const void *attr);
extern int pthread_rwlock_destroy(pthread_rwlock_t *lock);
extern int pthread_rwlock_rdlock(pthread_rwlock_t *lock);
extern int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock);
extern int pthread_rwlock_wrlock(pthread_rwlock_t *lock);
extern int pthread_rwlock_trywrlock(pthread_rwlock_t *lock);
extern int pthread_rwlock_unlock(pthread_rwlock_t *lock);

static pthread_rwlock_t lock;
static unsigned long protected_value;

unsigned long poly_entry(void)
{
  if (pthread_rwlock_init(&lock, 0) != 0)
    return 1;
  if (pthread_rwlock_rdlock(&lock) != 0)
    return 2;
  if (pthread_rwlock_tryrdlock(&lock) != 0)
    return 3;
  if (pthread_rwlock_trywrlock(&lock) != 16)
    return 4;
  protected_value += 40;
  if (pthread_rwlock_unlock(&lock) != 0)
    return 5;
  if (pthread_rwlock_unlock(&lock) != 0)
    return 6;
  if (pthread_rwlock_wrlock(&lock) != 0)
    return 7;
  if (pthread_rwlock_tryrdlock(&lock) != 16)
    return 8;
  if (pthread_rwlock_trywrlock(&lock) != 16)
    return 9;
  protected_value += 83;
  if (pthread_rwlock_unlock(&lock) != 0)
    return 10;
  if (pthread_rwlock_trywrlock(&lock) != 0)
    return 11;
  if (pthread_rwlock_unlock(&lock) != 0)
    return 12;
  if (pthread_rwlock_destroy(&lock) != 0)
    return 13;
  return protected_value;
}
