#ifndef POLY_ABI_DESCRIPTOR_H
#define POLY_ABI_DESCRIPTOR_H

#include <stdint.h>

#include "poly_abi_type.h"

int poly_abi_descriptor_decode_kind(int bridge_kind,
    struct poly_function_sig *sig);
uint32_t poly_abi_descriptor_signature_kind(int bridge_kind);
const char *poly_abi_descriptor_kind_name(int bridge_kind);

#endif
