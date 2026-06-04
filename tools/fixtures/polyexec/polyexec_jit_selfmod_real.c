#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
  POLY_JIT_OK = 42,
  POLY_PAGE_SIZE = 4096
};

static void emit_aarch64_return(uint32_t *code, uint32_t value) {
  code[0] = 0xd2800000U | ((value & 0xffffU) << 5); /* movz x0,#value */
  code[1] = 0xd65f03c0U; /* ret */
}

static void flush_aarch64_icache(void *addr) {
  uintptr_t line = (uintptr_t) addr & ~(uintptr_t) 63;
  __asm__ volatile("dc cvau, %0" :: "r"(line) : "memory");
  __asm__ volatile("dsb ish" ::: "memory");
  __asm__ volatile("ic ivau, %0" :: "r"(line) : "memory");
  __asm__ volatile("dsb ish" ::: "memory");
  __asm__ volatile("isb" ::: "memory");
}

static int protect_page(void *page, int prot) {
  return mprotect(page, POLY_PAGE_SIZE, prot) == 0;
}

int main(void) {
  void *page = mmap(0, POLY_PAGE_SIZE, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (page == MAP_FAILED)
    return 10;

  uint32_t *code = (uint32_t *) page;
  emit_aarch64_return(code, 13);
  flush_aarch64_icache(code);
  if (!protect_page(page, PROT_READ | PROT_EXEC))
    return 11;

  uint64_t (*entry)(void) = (uint64_t (*)(void)) page;
  if (entry() != 13)
    return 12;

  if (!protect_page(page, PROT_READ | PROT_WRITE))
    return 13;
  emit_aarch64_return(code, 29);
  flush_aarch64_icache(code);
  if (!protect_page(page, PROT_READ | PROT_EXEC))
    return 14;
  if (entry() != 29)
    return 15;

  static const char ok[] = "POLY_JIT_SELF_MOD_OK: arch=aarch64 first=13 second=29 wx=1 icache=1\n";
  if (write(1, ok, sizeof(ok) - 1) != (ssize_t) sizeof(ok) - 1)
    return 16;

  return POLY_JIT_OK;
}
