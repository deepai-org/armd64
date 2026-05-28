typedef unsigned int pthread_key_t;

extern int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
extern int pthread_key_delete(pthread_key_t key);
extern void *pthread_getspecific(pthread_key_t key);
extern int pthread_setspecific(pthread_key_t key, const void *value);

static unsigned long first_value = 37;
static unsigned long second_value = 54;

static void unused_destructor(void *value)
{
  (void) value;
}

unsigned long poly_entry(void)
{
  pthread_key_t key = 0xffffffffu;
  if (pthread_key_create(&key, unused_destructor) != 0)
    return 1;
  if (pthread_getspecific(key) != 0)
    return 2;
  if (pthread_setspecific(key, &first_value) != 0)
    return 3;
  if (*(unsigned long *) pthread_getspecific(key) != 37)
    return 4;
  if (pthread_setspecific(key, &second_value) != 0)
    return 5;
  if (*(unsigned long *) pthread_getspecific(key) != 54)
    return 6;
  if (pthread_key_delete(key) != 0)
    return 7;
  if (pthread_getspecific(key) != 0)
    return 8;
  return first_value + second_value;
}
