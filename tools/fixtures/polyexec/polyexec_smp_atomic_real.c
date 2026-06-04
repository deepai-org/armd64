#include <stdint.h>

__attribute__((visibility("hidden")))
uint64_t poly_entry(uint64_t *scratch) {
  volatile uint64_t *counter = (volatile uint64_t *) (uintptr_t) scratch[0];
  uint64_t iterations = scratch[1];
  uint64_t failures = 0;

  if (counter == 0 || iterations == 0)
    return 2;

  for (uint64_t n = 0; n < iterations; n++) {
#if defined(__aarch64__)
    uint64_t loaded;
    uint32_t status;
    do {
      __asm__ volatile(
        "ldxr %0, [%2]\n"
        "add %0, %0, #1\n"
        "stxr %w1, %0, [%2]\n"
        : "=&r"(loaded), "=&r"(status)
        : "r"(counter)
        : "memory");
      failures += status != 0;
    } while (status != 0);
    if ((n & 63) == 0)
      __asm__ volatile("dmb ish" ::: "memory");
#elif defined(__riscv)
    uint64_t loaded;
    uint64_t status;
    do {
      __asm__ volatile(
        "lr.d %0, (%2)\n"
        "addi %0, %0, 1\n"
        "sc.d %1, %0, (%2)\n"
        : "=&r"(loaded), "=&r"(status)
        : "r"(counter)
        : "memory");
      failures += status != 0;
    } while (status != 0);
    if ((n & 63) == 0)
      __asm__ volatile("fence rw,rw" ::: "memory");
#else
#error unsupported architecture
#endif
  }

#if defined(__aarch64__)
  __asm__ volatile("dmb ish" ::: "memory");
#elif defined(__riscv)
  __asm__ volatile("fence rw,rw" ::: "memory");
#endif

  return failures == UINT64_MAX ? 3 : 42;
}
