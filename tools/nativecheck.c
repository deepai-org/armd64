#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
  POLY_CPUID_BASE = 0x40000000
};

struct cpuid_regs {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

static inline struct cpuid_regs read_cpuid(uint32_t leaf, uint32_t subleaf) {
  struct cpuid_regs regs;
  asm volatile("cpuid"
    : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx), "=d"(regs.edx)
    : "a"(leaf), "c"(subleaf)
    : "memory");
  return regs;
}

static int poly_cpuid_present(void) {
  struct cpuid_regs regs = read_cpuid(POLY_CPUID_BASE, 0);
  char vendor[13];
  memcpy(vendor, &regs.ebx, 4);
  memcpy(vendor + 4, &regs.edx, 4);
  memcpy(vendor + 8, &regs.ecx, 4);
  vendor[12] = '\0';
  return regs.eax >= POLY_CPUID_BASE + 1 &&
    strcmp(vendor, "PolyglotCPU!") == 0;
}

int main(void) {
  const char *expect_poly_cpuid = getenv("EXPECT_POLY_CPUID");

  puts("NATIVE_ELF_OK");
  if (expect_poly_cpuid != NULL && strcmp(expect_poly_cpuid, "0") == 0) {
    if (poly_cpuid_present()) {
      fputs("NATIVE_CHECK_FAIL: poly CPUID leaf visible while disabled\n", stderr);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_ABSENT");
  }
  else if (expect_poly_cpuid != NULL && strcmp(expect_poly_cpuid, "1") == 0) {
    if (!poly_cpuid_present()) {
      fputs("NATIVE_CHECK_FAIL: poly CPUID leaf missing while enabled\n", stderr);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_PRESENT");
  }
  puts("NATIVE_CHECK_OK");
  return 0;
}
