typedef volatile int pthread_spinlock_t;

extern int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern int pthread_spin_destroy(pthread_spinlock_t *lock);
extern int pthread_spin_lock(pthread_spinlock_t *lock);
extern int pthread_spin_trylock(pthread_spinlock_t *lock);
extern int pthread_spin_unlock(pthread_spinlock_t *lock);

static pthread_spinlock_t lock;
static unsigned long protected_value;

unsigned long poly_entry(void)
{
  if (pthread_spin_init(&lock, 0) != 0)
    return 1;
  if (pthread_spin_lock(&lock) != 0)
    return 2;
  if (pthread_spin_trylock(&lock) != 16)
    return 3;
  protected_value += 67;
  if (pthread_spin_unlock(&lock) != 0)
    return 4;
  if (pthread_spin_trylock(&lock) != 0)
    return 5;
  protected_value += 74;
  if (pthread_spin_unlock(&lock) != 0)
    return 6;
  if (pthread_spin_destroy(&lock) != 0)
    return 7;
  return protected_value;
}
