#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_event_frame_encode.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_event_frame_encode.sv"

EVENT_MAGIC = 0x32545645594C4F50
EVENT_BYTES = 512
EVENT_VERSION = 2
EVENT_HEADER_BYTES = 408
EVENT_ARG_COUNT = 8


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


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def encode(
    valid: bool,
    enabled: bool,
    addr: int,
    event_kind: int,
    source_frontend: int,
    number: int,
    selector: int,
    insn_pc: int,
    resume_pc: int,
    args: list[int],
    c: dict[str, int],
) -> dict[str, object]:
    last = (addr + EVENT_BYTES - 1) & 0xFFFFFFFFFFFFFFFF
    wrap = last < addr
    disabled = valid and not enabled
    noncanonical = valid and enabled and (not canonical(addr) or not canonical(last))
    align = valid and enabled and (addr & 0x3F) != 0
    range_fault = valid and enabled and wrap
    invalid_kind = valid and enabled and event_kind not in {
        c["POLY_TRAP_SYSCALL"],
        c["POLY_TRAP_BREAK"],
        c["POLY_TRAP_IMPORT"],
        c["POLY_TRAP_ILLEGAL"],
    }
    invalid_source = valid and enabled and source_frontend not in {
        c["POLY_MODE_RAW_AARCH64"],
        c["POLY_MODE_RAW_RISCV"],
    }
    error = (
        disabled or noncanonical or align or range_fault or invalid_kind or
        invalid_source
    )
    qwords = [0] * 64
    qwords[0] = EVENT_MAGIC
    qwords[1] = (
        (EVENT_HEADER_BYTES << 48) |
        (EVENT_VERSION << 32) |
        EVENT_BYTES
    )
    qwords[3] = ((event_kind & 0xFFFF) << 32) | (source_frontend & 0xFFFFFFFF)
    qwords[5] = EVENT_ARG_COUNT << 48
    qwords[6] = insn_pc
    qwords[7] = resume_pc
    qwords[8] = resume_pc
    qwords[14] = selector
    for index, arg in enumerate(args):
        qwords[15 + index] = arg
    qwords[24] = number
    return {
        "frame_valid": valid and not error,
        "addr": addr,
        "bytes": EVENT_BYTES,
        "qwords": qwords,
        "error": error,
        "disabled": disabled,
        "noncanonical": noncanonical,
        "align": align,
        "range": range_fault,
        "invalid_kind": invalid_kind,
        "invalid_source": invalid_source,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    assert sv["POLY_FRONTEND_RAW_AARCH64"] == c["POLY_MODE_RAW_AARCH64"]
    assert sv["POLY_FRONTEND_RAW_RISCV"] == c["POLY_MODE_RAW_RISCV"]
    assert sv["POLY_EVENT_SYSCALL"] == c["POLY_TRAP_SYSCALL"]
    assert sv["POLY_EVENT_BREAK"] == c["POLY_TRAP_BREAK"]
    assert sv["POLY_EVENT_IMPORT"] == c["POLY_TRAP_IMPORT"]
    assert sv["POLY_EVENT_ILLEGAL"] == c["POLY_TRAP_ILLEGAL"]
    assert sv["POLY_V2_EVENT_BYTES"] == EVENT_BYTES
    assert sv["POLY_V2_EVENT_VERSION"] == EVENT_VERSION
    assert sv["POLY_V2_EVENT_HEADER_BYTES"] == EVENT_HEADER_BYTES
    assert sv["POLY_V2_EVENT_ARG_COUNT"] == c["POLY_V2_EVENT_ARG_COUNT"]

    args = [0x100 + n for n in range(c["POLY_V2_EVENT_ARG_COUNT"])]
    ok = encode(
        True, True, 0x0000000000457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 7, 0x4000, 0x4004, args, c
    )
    assert ok["frame_valid"] and not ok["error"]
    assert ok["bytes"] == EVENT_BYTES
    assert ok["qwords"][0] == EVENT_MAGIC
    assert ok["qwords"][1] == (EVENT_HEADER_BYTES << 48) | (EVENT_VERSION << 32) | EVENT_BYTES
    assert ok["qwords"][3] == (c["POLY_TRAP_SYSCALL"] << 32) | c["POLY_MODE_RAW_AARCH64"]
    assert ok["qwords"][5] == EVENT_ARG_COUNT << 48
    assert ok["qwords"][6:9] == [0x4000, 0x4004, 0x4004]
    assert ok["qwords"][14:23] == [7, *args]
    assert ok["qwords"][24] == 172

    ok_64_aligned = encode(
        True, True, 0x0000000000457040, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert ok_64_aligned["frame_valid"] and not ok_64_aligned["error"]

    disabled = encode(
        True, False, 0x0000000000457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert disabled["error"] and disabled["disabled"] and not disabled["frame_valid"]

    bad_noncanonical = encode(
        True, True, 0x0000800000000000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert bad_noncanonical["error"] and bad_noncanonical["noncanonical"]

    bad_align = encode(
        True, True, 0x0000000000457008, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 5, 0, 0x8000, 0x8002, args, c
    )
    assert bad_align["error"] and bad_align["align"]

    wrap = encode(
        True, True, 0xFFFFFFFFFFFFFE40, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_AARCH64"], 8, 0, 0x4000, 0x4004, args, c
    )
    assert wrap["error"] and wrap["range"]

    invalid_kind = encode(
        True, True, 0x0000000000457000, 99,
        c["POLY_MODE_RAW_AARCH64"], 0, 0, 0x4000, 0x4004, args, c
    )
    assert invalid_kind["error"] and invalid_kind["invalid_kind"]

    invalid_source = encode(
        True, True, 0x0000000000457000, c["POLY_TRAP_ILLEGAL"],
        c["POLY_MODE_X86"], 0, 0, 0x4000, 0x4004, args, c
    )
    assert invalid_source["error"] and invalid_source["invalid_source"]

    print("POLY_RTL_EVENT_FRAME_ENCODE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
