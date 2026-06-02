#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_abi_signature_slots.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_abi_signature_slots.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending_aliases: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().replace("(uint32_t) ", "")
        shift = re.fullmatch(r"(\d+)U?\s*<<\s*(\d+)", expr)
        if shift:
            constants[name] = int(shift.group(1), 0) << int(shift.group(2), 0)
            continue
        expr = expr.rstrip("Uu")
        if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", expr):
            constants[name] = int(expr, 0)
            continue
        if re.fullmatch(r"POLY_[A-Z0-9_]+", expr):
            pending_aliases.append((name, expr))
    changed = True
    while changed:
        changed = False
        for name, alias in pending_aliases:
            if name not in constants and alias in constants:
                constants[name] = constants[alias]
                changed = True
    return constants


def parse_sv_params(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending_aliases: list[tuple[str, str]] = []
    for name, expr in re.findall(r"parameter\s+int\s+([A-Z0-9_]+)\s*=\s*(\d+)", text):
        constants[name] = int(expr, 0)
    pattern = re.compile(
        r"localparam\s+logic\s+(?:\[[^\]]+\]\s+)?([A-Z0-9_]+)\s*=\s*([^;]+);"
    )
    for name, expr in pattern.findall(text):
        expr = expr.strip()
        sv = re.fullmatch(r"(\d+)'([hdb])([0-9a-fA-F_]+)", expr)
        if sv:
            _, base, value = sv.groups()
            constants[name] = int(value.replace("_", ""), {"h": 16, "d": 10, "b": 2}[base])
        elif re.fullmatch(r"[A-Z0-9_]+", expr):
            pending_aliases.append((name, expr))
        else:
            constants[name] = int(expr, 0)
    changed = True
    while changed:
        changed = False
        for name, alias in pending_aliases:
            if name not in constants and alias in constants:
                constants[name] = constants[alias]
                changed = True
    return constants


class SignatureSlots:
    def __init__(self, c: dict[str, int]):
        self.slot_count = c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        self.last_kind = c["POLY_ABI_SIGNATURE_KIND_LAST_VALID"]
        self.last_map = c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_AARCH64_HFA4_F64_RET"]
        self.tls_flag = c["POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE"]
        self.slots = [
            (c["POLY_ABI_SIGNATURE_KIND_EXCHANGE"], c["POLY_ABI_REGISTER_MAP_EXCHANGE"]),
            (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS"],
             c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE"]),
            (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128"],
             c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE_I128"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS"], c["POLY_ABI_REGISTER_MAP_NATIVE"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_I128"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_VEC128_U32"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_COMPACT_U32_F32"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_COMPACT_F32_U32"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_FP64"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_FP32"]),
            (c["POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS"],
             c["POLY_ABI_REGISTER_MAP_SRET_X86_SYSV_TO_NATIVE"]),
            (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FP128_RET"],
             c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE_FP128_RET"]),
            (c["POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS"],
             c["POLY_ABI_REGISTER_MAP_NATIVE_SRET"]),
        ]

    def select(self, slot: int) -> tuple[bool, int, int, bool]:
        if slot >= self.slot_count:
            return (False, 0, 0, False)
        kind, register_map = self.slots[slot]
        return (True, kind, register_map & 0x7F, bool(register_map & self.tls_flag))

    def set(self, slot: int, kind: int, register_map: int) -> tuple[bool, bool]:
        map_without_tls = register_map & ~self.tls_flag
        invalid_high = map_without_tls & ~0x7F
        if slot >= self.slot_count or kind > self.last_kind or invalid_high:
            return (False, True)
        if (map_without_tls & 0x7F) > self.last_map:
            return (False, True)
        self.slots[slot] = (kind, register_map)
        return (True, False)


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_params(RTL)

    assert sv["SLOT_COUNT"] == c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
    assert sv["POLY_ABI_SIGNATURE_KIND_LAST_VALID"] == c[
        "POLY_ABI_SIGNATURE_KIND_LAST_VALID"
    ]
    assert sv["POLY_ABI_REGISTER_MAP_LAST_VALID"] == c[
        "POLY_ABI_REGISTER_MAP_X86_SYSV_TO_AARCH64_HFA4_F64_RET"
    ]
    assert sv["POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE"] == c[
        "POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE"
    ]

    slots = SignatureSlots(c)
    for slot in range(c["POLY_ABI_SIGNATURE_SLOT_COUNT"]):
        valid, kind, register_map, tls = slots.select(slot)
        assert valid and not tls
        expected_kind, expected_map = slots.slots[slot]
        assert kind == expected_kind
        assert register_map == expected_map

    assert slots.select(c["POLY_ABI_SIGNATURE_SLOT_COUNT"]) == (False, 0, 0, False)

    tls_map = c["POLY_ABI_REGISTER_MAP_NATIVE"] | c["POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE"]
    assert slots.set(3, c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS"], tls_map) == (
        True, False
    )
    assert slots.select(3) == (
        True, c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS"],
        c["POLY_ABI_REGISTER_MAP_NATIVE"], True
    )

    assert slots.set(c["POLY_ABI_SIGNATURE_SLOT_COUNT"], 0, 0) == (False, True)
    assert slots.set(0, c["POLY_ABI_SIGNATURE_KIND_LAST_VALID"] + 1, 0) == (
        False, True
    )
    assert slots.set(0, 0, c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_AARCH64_HFA4_F64_RET"] + 1) == (
        False, True
    )
    assert slots.set(0, 0, 0x100) == (False, True)

    print("POLY_RTL_ABI_SIGNATURE_SLOTS_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
