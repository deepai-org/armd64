#include <stdint.h>

static volatile uint64_t poly_crash_sink;

__attribute__((noinline)) static uint64_t poly_crash_leaf(uint64_t marker) {
  volatile uint64_t *fault = (volatile uint64_t *) (uintptr_t) 0;
  return *fault + marker;
}

__attribute__((noinline)) static uint64_t poly_crash_middle(uint64_t marker) {
  poly_crash_sink = marker ^ 0x706f6c79636f7265ULL;
  return poly_crash_leaf(marker + 1);
}

uint64_t _start(uint64_t *initial_sp) {
  return poly_crash_middle((uint64_t) (uintptr_t) initial_sp);
}
