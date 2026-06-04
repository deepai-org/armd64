#include "poly_abi_classify.h"

int poly_classify_call_abi(enum poly_abi_arch arch,
    const struct poly_function_sig *sig, struct poly_abi_layout *out) {
  switch (arch) {
    case POLY_ABI_ARCH_X86:
      return poly_classify_x86_sysv_call_abi(sig, out);
    case POLY_ABI_ARCH_AARCH64:
      return poly_classify_aarch64_call_abi(sig, out);
    case POLY_ABI_ARCH_RISCV:
      return poly_classify_riscv_lp64d_call_abi(sig, out);
    default:
      return -1;
  }
}
