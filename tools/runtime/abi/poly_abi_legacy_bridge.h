#ifndef POLY_ABI_LEGACY_BRIDGE_H
#define POLY_ABI_LEGACY_BRIDGE_H

#include <stdint.h>

#include "poly_abi_type.h"

int poly_decode_legacy_bridge_kind(int bridge_kind,
    struct poly_function_sig *sig);
uint32_t poly_legacy_bridge_signature_kind(int bridge_kind);
const char *poly_legacy_bridge_kind_name(int bridge_kind);

#endif
