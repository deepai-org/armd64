#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_trap_packet_encode.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_trap_packet_encode.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = re.sub(r"/\*.*?\*/", "", expr.strip().replace("(uint32_t) ", ""))
        shift = re.search(r"(\d+)U?(?:LL)?\s*<<\s*(\d+)", expr)
        if shift:
            constants[name] = int(shift.group(1), 0) << int(shift.group(2), 0)
            continue
        expr = expr.rstrip("UuLl")
        if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", expr):
            constants[name] = int(expr, 0)
            continue
        pending.append((name, expr.strip()))

    changed = True
    while changed:
        changed = False
        for name, expr in pending:
            if name in constants:
                continue
            if re.fullmatch(r"POLY_[A-Z0-9_]+", expr) and expr in constants:
                constants[name] = constants[expr]
                changed = True
                continue
            parts = [part.strip() for part in expr.split("|")]
            if len(parts) > 1 and all(part in constants for part in parts):
                value = 0
                for part in parts:
                    value |= constants[part]
                constants[name] = value
                changed = True
    return constants


def parse_sv_localparams(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pattern = re.compile(
        r"localparam\s+logic\s+(?:\[[^\]]+\]\s+)?([A-Z0-9_]+)\s*=\s*([^;]+);"
    )
    for name, expr in pattern.findall(text):
        expr = expr.strip()
        sv = re.fullmatch(r"(\d+)'([hdb])([0-9a-fA-F_]+)", expr)
        if sv:
            _, base, value = sv.groups()
            constants[name] = int(value.replace("_", ""), {"h": 16, "d": 10, "b": 2}[base])
        else:
            constants[name] = int(expr, 0)
    return constants


def trap_flags(c: dict[str, int]) -> int:
    return (
        c["POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY"] |
        c["POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS"] |
        c["POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE"] |
        c["POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS"] |
        c["POLY_TRAP_PACKET_FLAG_OPAQUE_SYSCALLS"] |
        c["POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY"] |
        c["POLY_TRAP_PACKET_FLAG_OPAQUE_IMPORTS"]
    )


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def encode(
    valid: bool,
    enabled: bool,
    addr: int,
    reason: int,
    source_mode: int,
    number: int,
    selector: int,
    trap_pc: int,
    resume_pc: int,
    args: list[int],
    c: dict[str, int],
) -> dict[str, object]:
    last = (addr + 127) & 0xFFFFFFFFFFFFFFFF
    wrap = last < addr
    disabled = valid and not enabled
    noncanonical = valid and enabled and (not canonical(addr) or not canonical(last))
    align = valid and enabled and (addr & 0x7) != 0
    range_fault = valid and enabled and wrap
    invalid_reason = valid and enabled and reason not in {
        c["POLY_TRAP_SYSCALL"],
        c["POLY_TRAP_BREAK"],
        c["POLY_TRAP_IMPORT"],
        c["POLY_TRAP_ILLEGAL"],
    }
    invalid_source = valid and enabled and source_mode not in {
        c["POLY_MODE_RAW_AARCH64"],
        c["POLY_MODE_RAW_RISCV"],
    }
    error = (
        disabled or noncanonical or align or range_fault or invalid_reason or
        invalid_source
    )
    qwords = [
        ((source_mode & 0xFFFFFFFF) << 32) | (reason & 0xFFFFFFFF),
        number,
        selector,
        trap_pc,
        resume_pc,
        trap_flags(c),
        0,
        0,
        *args,
    ]
    return {
        "packet_valid": valid and not error,
        "addr": addr,
        "bytes": 128,
        "qwords": qwords,
        "error": error,
        "disabled": disabled,
        "noncanonical": noncanonical,
        "align": align,
        "range": range_fault,
        "invalid_reason": invalid_reason,
        "invalid_source": invalid_source,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    assert sv["POLY_MODE_RAW_AARCH64"] == c["POLY_MODE_RAW_AARCH64"]
    assert sv["POLY_MODE_RAW_RISCV"] == c["POLY_MODE_RAW_RISCV"]
    assert sv["POLY_TRAP_SYSCALL"] == c["POLY_TRAP_SYSCALL"]
    assert sv["POLY_TRAP_BREAK"] == c["POLY_TRAP_BREAK"]
    assert sv["POLY_TRAP_IMPORT"] == c["POLY_TRAP_IMPORT"]
    assert sv["POLY_TRAP_ILLEGAL"] == c["POLY_TRAP_ILLEGAL"]
    assert sv["POLY_MONITOR_PACKET_BYTES"] == (
        c["POLY_STATE_XSAVE_TRAP_PACKET_BYTES"] +
        c["POLY_STATE_XSAVE_TRAP_ARGS_BYTES"]
    )
    assert sv["POLY_MONITOR_PACKET_ALIGN"] == 8
    assert sv["POLY_TRAP_PACKET_REQUIRED_FLAGS"] == trap_flags(c)

    args = [0x100 + n for n in range(c["POLY_V2_EVENT_ARG_COUNT"])]
    ok = encode(
        True, True, 0x0000000000457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 7, 0x4000, 0x4004, args, c
    )
    assert ok["packet_valid"] and not ok["error"]
    assert ok["bytes"] == 128
    assert ok["qwords"] == [
        (c["POLY_MODE_RAW_AARCH64"] << 32) | c["POLY_TRAP_SYSCALL"],
        172, 7, 0x4000, 0x4004, trap_flags(c), 0, 0, *args
    ]

    ok_8_aligned = encode(
        True, True, 0x0000000000457008, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert ok_8_aligned["packet_valid"] and not ok_8_aligned["error"]

    disabled = encode(
        True, False, 0x0000000000457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert disabled["error"] and disabled["disabled"] and not disabled["packet_valid"]

    bad_noncanonical = encode(
        True, True, 0x0000800000000000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert bad_noncanonical["error"] and bad_noncanonical["noncanonical"]

    bad_align = encode(
        True, True, 0x0000000000457001, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert bad_align["error"] and bad_align["align"]

    cross_canonical = (1 << 47) - 64
    bad_range = encode(
        True, True, cross_canonical, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_AARCH64"], 8, 0, 0x4000, 0x4004, args, c
    )
    assert bad_range["error"] and bad_range["noncanonical"]

    wrap = encode(
        True, True, 0xFFFFFFFFFFFFFFC0, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_AARCH64"], 8, 0, 0x4000, 0x4004, args, c
    )
    assert wrap["error"] and wrap["range"]

    invalid_reason = encode(
        True, True, 0x0000000000457000, 99,
        c["POLY_MODE_RAW_AARCH64"], 0, 0, 0x4000, 0x4004, args, c
    )
    assert invalid_reason["error"] and invalid_reason["invalid_reason"]

    invalid_source = encode(
        True, True, 0x0000000000457000, c["POLY_TRAP_ILLEGAL"],
        c["POLY_MODE_X86"], 0, 0, 0x4000, 0x4004, args, c
    )
    assert invalid_source["error"] and invalid_source["invalid_source"]

    print("POLY_RTL_TRAP_PACKET_ENCODE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
