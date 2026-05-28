typedef int pthread_once_t;

#define PTHREAD_ONCE_INIT 0

extern int pthread_once(pthread_once_t *once_control,
    void (*init_routine)(void));

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static unsigned long init_count;
static unsigned long init_value;

static void initialize_once(void)
{
  init_count++;
  init_value = 77;
}

unsigned long poly_entry(void)
{
  int first = pthread_once(&once_control, initialize_once);
  int second = pthread_once(&once_control, initialize_once);
  if (first != 0 || second != 0)
    return 1;
  if (init_count != 1 || init_value != 77)
    return 2;
  return init_value;
}
