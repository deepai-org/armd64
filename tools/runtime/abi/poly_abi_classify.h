#ifndef POLY_ABI_CLASSIFY_H
#define POLY_ABI_CLASSIFY_H

#include "poly_abi_layout.h"

int poly_classify_call_abi(enum poly_abi_arch arch,
    const struct poly_function_sig *sig, struct poly_abi_layout *out);

int poly_classify_x86_sysv_call_abi(const struct poly_function_sig *sig,
    struct poly_abi_layout *out);
int poly_classify_aarch64_call_abi(const struct poly_function_sig *sig,
    struct poly_abi_layout *out);
int poly_classify_riscv_lp64d_call_abi(const struct poly_function_sig *sig,
    struct poly_abi_layout *out);

#endif
