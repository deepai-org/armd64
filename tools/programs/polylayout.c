#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/polycpuid.h"

struct poly_layout_row {
  const char *name;
  uint32_t expected_offset;
  uint32_t expected_bytes;
  size_t actual_offset;
  size_t actual_bytes;
};

#define POLY_LAYOUT_ROW(field, offset_name, bytes_name) \
  { #field, offset_name, bytes_name, \
    offsetof(struct poly_xsave_state, field), \
    sizeof(((struct poly_xsave_state *) 0)->field) }

static const struct poly_layout_row poly_layout_rows[] = {
  POLY_LAYOUT_ROW(header, POLY_STATE_XSAVE_HEADER_OFFSET,
    POLY_STATE_XSAVE_HEADER_BYTES),
  POLY_LAYOUT_ROW(trap, POLY_STATE_XSAVE_TRAP_PACKET_OFFSET,
    POLY_STATE_XSAVE_TRAP_PACKET_BYTES),
  POLY_LAYOUT_ROW(trap_args, POLY_STATE_XSAVE_TRAP_ARGS_OFFSET,
    POLY_STATE_XSAVE_TRAP_ARGS_BYTES),
  POLY_LAYOUT_ROW(transition, POLY_STATE_XSAVE_TRANSITION_OFFSET,
    POLY_STATE_XSAVE_TRANSITION_BYTES),
  POLY_LAYOUT_ROW(aarch64_gpr, POLY_STATE_XSAVE_AARCH64_GPR_OFFSET,
    POLY_STATE_XSAVE_AARCH64_GPR_BYTES),
  POLY_LAYOUT_ROW(aarch64_fp, POLY_STATE_XSAVE_AARCH64_FP_OFFSET,
    POLY_STATE_XSAVE_AARCH64_FP_BYTES),
  POLY_LAYOUT_ROW(aarch64_status, POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET,
    POLY_STATE_XSAVE_AARCH64_STATUS_BYTES),
  POLY_LAYOUT_ROW(riscv_gpr, POLY_STATE_XSAVE_RISCV_GPR_OFFSET,
    POLY_STATE_XSAVE_RISCV_GPR_BYTES),
  POLY_LAYOUT_ROW(riscv_fp, POLY_STATE_XSAVE_RISCV_FP_OFFSET,
    POLY_STATE_XSAVE_RISCV_FP_BYTES),
  POLY_LAYOUT_ROW(riscv_status, POLY_STATE_XSAVE_RISCV_STATUS_OFFSET,
    POLY_STATE_XSAVE_RISCV_STATUS_BYTES),
  POLY_LAYOUT_ROW(import_return, POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET,
    POLY_STATE_XSAVE_IMPORT_RETURN_BYTES),
  POLY_LAYOUT_ROW(abi_signature, POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET,
    POLY_STATE_XSAVE_ABI_SIGNATURE_BYTES),
  POLY_LAYOUT_ROW(cross_return, POLY_STATE_XSAVE_CROSS_RETURN_OFFSET,
    POLY_STATE_XSAVE_CROSS_RETURN_BYTES),
  POLY_LAYOUT_ROW(frontend_tls, POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET,
    POLY_STATE_XSAVE_FRONTEND_TLS_BYTES),
  POLY_LAYOUT_ROW(landing_policy, POLY_STATE_XSAVE_LANDING_POLICY_OFFSET,
    POLY_STATE_XSAVE_LANDING_POLICY_BYTES),
  POLY_LAYOUT_ROW(state_key, POLY_STATE_XSAVE_STATE_KEY_OFFSET,
    POLY_STATE_XSAVE_STATE_KEY_BYTES),
  POLY_LAYOUT_ROW(trap_restore, POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET,
    POLY_STATE_XSAVE_TRAP_RESTORE_BYTES),
  POLY_LAYOUT_ROW(native_return, POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET,
    POLY_STATE_XSAVE_NATIVE_RETURN_BYTES),
  POLY_LAYOUT_ROW(reserved, POLY_STATE_XSAVE_RESERVED_OFFSET,
    POLY_STATE_XSAVE_RESERVED_BYTES),
};

static int poly_layout_check_rows(void) {
  int ok = 1;
  uint32_t previous_end = 0;
  for (size_t n = 0; n < sizeof(poly_layout_rows) / sizeof(poly_layout_rows[0]);
       n++) {
    const struct poly_layout_row *row = &poly_layout_rows[n];
    const uint32_t row_end = row->expected_offset + row->expected_bytes;
    if (row->expected_offset != row->actual_offset ||
        row->expected_bytes != row->actual_bytes) {
      fprintf(stderr,
        "POLY_LAYOUT_FAIL: %s expected offset=0x%x bytes=0x%x actual offset=0x%zx bytes=0x%zx\n",
        row->name, row->expected_offset, row->expected_bytes,
        row->actual_offset, row->actual_bytes);
      ok = 0;
    }
    if (row->expected_offset < previous_end) {
      fprintf(stderr,
        "POLY_LAYOUT_FAIL: %s overlaps previous region offset=0x%x previous_end=0x%x\n",
        row->name, row->expected_offset, previous_end);
      ok = 0;
    }
    previous_end = row_end;
  }
  if (sizeof(struct poly_xsave_state) != POLY_STATE_XSAVE_BYTES_ARCH) {
    fprintf(stderr,
      "POLY_LAYOUT_FAIL: poly_xsave_state size expected=0x%x actual=0x%zx\n",
      POLY_STATE_XSAVE_BYTES_ARCH, sizeof(struct poly_xsave_state));
    ok = 0;
  }
  if ((POLY_STATE_XSAVE_OFFSET_ARCH & (POLY_STATE_XSAVE_ALIGN_ARCH - 1)) != 0 ||
      (POLY_STATE_XSAVE_BYTES_ARCH & (POLY_STATE_XSAVE_ALIGN_ARCH - 1)) != 0) {
    fprintf(stderr,
      "POLY_LAYOUT_FAIL: arch XSAVE component offset/bytes are not %u-byte aligned\n",
      POLY_STATE_XSAVE_ALIGN_ARCH);
    ok = 0;
  }
  return ok;
}

static void poly_layout_print_table(void) {
  printf("name,offset,bytes,actual_offset,actual_bytes\n");
  for (size_t n = 0; n < sizeof(poly_layout_rows) / sizeof(poly_layout_rows[0]);
       n++) {
    const struct poly_layout_row *row = &poly_layout_rows[n];
    printf("%s,0x%x,0x%x,0x%zx,0x%zx\n", row->name,
      row->expected_offset, row->expected_bytes, row->actual_offset,
      row->actual_bytes);
  }
}

static int poly_layout_verify_expected_cpuid_manifest(void) {
  struct poly_cpuid_contract_check check;
  size_t count = 0;
  while (poly_cpuid_arch_state_contract_check(count, &check))
    count++;
  if (count != 22) {
    fprintf(stderr,
      "POLY_LAYOUT_FAIL: expected 22 architectural CPUID state checks, got %zu\n",
      count);
    return 0;
  }
  return 1;
}

static int poly_layout_verify_live_cpuid(void) {
  struct poly_cpuid_contract_failure failure;
  if (!poly_cpuid_present()) {
    fprintf(stderr, "POLY_LAYOUT_FAIL: Poly CPUID vendor leaf is not present\n");
    return 0;
  }
  if (!poly_cpuid_verify_arch_state_contract(&failure)) {
    fprintf(stderr,
      "POLY_LAYOUT_FAIL: %s leaf=0x%x subleaf=%u actual=%08x:%08x:%08x:%08x expected=%08x:%08x:%08x:%08x\n",
      failure.name, failure.leaf, failure.subleaf,
      failure.actual.eax, failure.actual.ebx, failure.actual.ecx,
      failure.actual.edx, failure.expected.eax, failure.expected.ebx,
      failure.expected.ecx, failure.expected.edx);
    return 0;
  }
  return 1;
}

int main(int argc, char **argv) {
  int print_table = 1;
  int check = 0;
  int verify_cpuid = 0;

  for (int n = 1; n < argc; n++) {
    if (strcmp(argv[n], "--check") == 0) {
      check = 1;
      print_table = 0;
    } else if (strcmp(argv[n], "--verify-cpuid") == 0) {
      verify_cpuid = 1;
      print_table = 0;
    } else if (strcmp(argv[n], "--table") == 0) {
      print_table = 1;
    } else {
      fprintf(stderr,
        "usage: %s [--table] [--check] [--verify-cpuid]\n", argv[0]);
      return 2;
    }
  }

  if (print_table)
    poly_layout_print_table();

  if (check &&
      (!poly_layout_check_rows() ||
       !poly_layout_verify_expected_cpuid_manifest()))
    return 1;

  if (verify_cpuid && !poly_layout_verify_live_cpuid())
    return 1;

  if (check)
    puts("POLY_STATE_LAYOUT_OK");
  if (verify_cpuid)
    puts("POLY_STATE_LAYOUT_CPUID_OK");
  return 0;
}
