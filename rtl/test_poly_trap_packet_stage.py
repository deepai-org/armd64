#!/usr/bin/env python3
"""Integration checks for rtl/poly_trap_packet_stage.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_trap_packet_stage.sv"


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
    return {
        "packet_valid": valid and not error,
        "addr": addr,
        "bytes": 128,
        "qwords": [
            ((source_mode & 0xFFFFFFFF) << 32) | (reason & 0xFFFFFFFF),
            number,
            selector,
            trap_pc,
            resume_pc,
            trap_flags(c),
            0,
            0,
            *args,
        ],
        "error": error,
        "disabled": disabled,
        "noncanonical": noncanonical,
        "align": align,
        "range": range_fault,
        "invalid_reason": invalid_reason,
        "invalid_source": invalid_source,
    }


def stage(
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
    mem_resp_valid: bool,
    mem_fault: bool,
    c: dict[str, int],
) -> dict[str, object]:
    packet = encode(
        valid, enabled, addr, reason, source_mode, number, selector, trap_pc,
        resume_pc, args, c
    )
    packet_valid = bool(packet["packet_valid"])
    packet_mem_fault = packet_valid and mem_resp_valid and mem_fault
    return {
        "mem_write_valid": packet_valid,
        "mem_addr": packet["addr"],
        "mem_bytes": packet["bytes"],
        "qwords": packet["qwords"],
        "wait": packet_valid and not mem_resp_valid,
        "delivered": packet_valid and mem_resp_valid and not mem_fault,
        "fault": packet["error"] or packet_mem_fault,
        "encode_error": packet["error"],
        "packet_mem_fault": packet_mem_fault,
        "disabled": packet["disabled"],
        "noncanonical": packet["noncanonical"],
        "align": packet["align"],
        "range": packet["range"],
        "invalid_reason": packet["invalid_reason"],
        "invalid_source": packet["invalid_source"],
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    assert "poly_trap_packet_encode packet_encode" in text
    assert "packet_delivered_o =" in text
    assert "packet_mem_fault_o =" in text

    args = [0x100 + n for n in range(c["POLY_V2_EVENT_ARG_COUNT"])]

    idle = stage(
        False, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 0, 0x4000, 0x4004, args,
        False, False, c
    )
    assert not idle["mem_write_valid"]
    assert not idle["wait"] and not idle["fault"] and not idle["delivered"]

    waiting = stage(
        True, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 0, 0x4000, 0x4004, args,
        False, False, c
    )
    assert waiting["mem_write_valid"] and waiting["wait"]
    assert not waiting["fault"] and not waiting["delivered"]

    delivered = stage(
        True, True, 0x457000, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_RISCV"], 5, 9, 0x8000, 0x8002, args,
        True, False, c
    )
    assert delivered["mem_write_valid"] and delivered["delivered"]
    assert not delivered["fault"] and not delivered["wait"]
    assert delivered["mem_bytes"] == 128
    assert delivered["qwords"] == [
        (c["POLY_MODE_RAW_RISCV"] << 32) | c["POLY_TRAP_IMPORT"],
        5, 9, 0x8000, 0x8002, trap_flags(c), 0, 0, *args
    ]

    packet_fault = stage(
        True, True, 0x457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, True, c
    )
    assert packet_fault["mem_write_valid"]
    assert packet_fault["fault"] and packet_fault["packet_mem_fault"]
    assert not packet_fault["delivered"]

    disabled = stage(
        True, False, 0x457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, False, c
    )
    assert disabled["fault"] and disabled["encode_error"] and disabled["disabled"]
    assert not disabled["mem_write_valid"] and not disabled["delivered"]

    bad_align = stage(
        True, True, 0x457001, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 1, 0, 0x8000, 0x8002, args,
        True, False, c
    )
    assert bad_align["fault"] and bad_align["align"]
    assert not bad_align["mem_write_valid"]

    bad_source = stage(
        True, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_X86"], 1, 0, 0x1000, 0x1004, args,
        True, False, c
    )
    assert bad_source["fault"] and bad_source["invalid_source"]
    assert not bad_source["mem_write_valid"]

    bad_range = stage(
        True, True, 0xFFFFFFFFFFFFFFC0, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, False, c
    )
    assert bad_range["fault"] and bad_range["range"]
    assert not bad_range["mem_write_valid"]

    print("POLY_RTL_TRAP_PACKET_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
