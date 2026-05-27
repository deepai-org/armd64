#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polycpuid.h"

int main(void) {
  const char *expect_poly_cpuid = getenv("EXPECT_POLY_CPUID");
  const char *expect_poly_compat_traps = getenv("EXPECT_POLY_COMPAT_TRAPS");

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
    int compat_traps = expect_poly_compat_traps == NULL ||
      strcmp(expect_poly_compat_traps, "0") != 0;
    struct poly_cpuid_regs features = poly_read_cpuid(POLY_CPUID_BASE + 1, 0);
    uint32_t expected_features =
      poly_cpuid_expected_feature_mask_for_compat(compat_traps);
    if (features.eax != POLY_CPUID_ABI_VERSION ||
        features.ebx != poly_cpuid_expected_mode_mask() ||
        features.ecx != expected_features ||
        features.edx != 0) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID feature leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x expected_ecx=0x%x\n",
        features.eax, features.ebx, features.ecx, features.edx,
        expected_features);
      return 1;
    }
    struct poly_cpuid_regs expected_state = poly_cpuid_expected_state_leaf();
    struct poly_cpuid_regs state = poly_read_cpuid(POLY_CPUID_BASE + 3, 0);
    if (state.eax != expected_state.eax ||
        state.ebx != expected_state.ebx ||
        state.ecx != expected_state.ecx ||
        state.edx != expected_state.edx) {
      fprintf(stderr, "NATIVE_CHECK_FAIL: poly CPUID state leaf mismatch eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        state.eax, state.ebx, state.ecx, state.edx);
      return 1;
    }
    puts("NATIVE_CPUID_POLY_PRESENT");
  }
  puts("NATIVE_CHECK_OK");
  return 0;
}
