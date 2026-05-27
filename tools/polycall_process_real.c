extern unsigned long getpid(void);
extern unsigned long getppid(void);
extern unsigned long getuid(void);
extern unsigned long geteuid(void);
extern unsigned long getgid(void);
extern unsigned long getegid(void);
extern unsigned long gettid(void);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    getpid() + getppid() + getuid() + geteuid() + getgid() + getegid() +
    gettid();
}
