#!/usr/bin/env python3
"""Integration checks for rtl/poly_raw_fetch_stage.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_raw_fetch_stage.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending_aliases: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().rstrip("Uu")
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


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def raw_request(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, int | bool]:
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    last = (pc + 3) & 0xFFFFFFFFFFFFFFFF
    invalid_frontend = valid and frontend != c["POLY_FRONTEND_X86"] and not raw
    noncanonical = valid and raw and (not canonical(pc) or not canonical(last))
    align = valid and raw and (
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    range_fault = valid and raw and last < pc
    error = invalid_frontend or noncanonical or align or range_fault
    return {
        "valid": valid and raw and not error,
        "error": error,
        "invalid_frontend": invalid_frontend,
        "noncanonical": noncanonical,
        "align": align,
        "range": range_fault,
        "bytes": 4 if raw else 0,
    }


def raw_plan(
    valid: bool,
    frontend: int,
    pc: int,
    word: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    if not valid:
        return {"valid": False, "bytes": 0, "insn": 0, "next_pc": pc, "align": False}
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return {
            "valid": (pc & 3) == 0,
            "bytes": 4,
            "insn": word & 0xFFFFFFFF,
            "next_pc": pc + 4,
            "align": (pc & 3) != 0,
        }
    if frontend == c["POLY_FRONTEND_RISCV"]:
        is_32 = (word & 3) == 3
        size = 4 if is_32 else 2
        return {
            "valid": (pc & 1) == 0,
            "bytes": size,
            "insn": word & 0xFFFFFFFF if is_32 else word & 0xFFFF,
            "next_pc": pc + size,
            "align": (pc & 1) != 0,
        }
    return {"valid": False, "bytes": 0, "insn": 0, "next_pc": pc, "align": False}


def stage(
    valid: bool,
    frontend: int,
    pc: int,
    resp_valid: bool,
    resp_fault: bool,
    word: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    req = raw_request(valid, frontend, pc, c)
    plan_valid = req["valid"] and resp_valid and not resp_fault
    plan = raw_plan(plan_valid, frontend, pc, word, c)
    mem_fault = req["valid"] and resp_valid and resp_fault
    fault = req["error"] or mem_fault or (plan_valid and plan["align"])
    return {
        "mem_req": req["valid"],
        "mem_addr": pc,
        "mem_bytes": req["bytes"],
        "wait": req["valid"] and not resp_valid,
        "insn_valid": plan["valid"],
        "insn": plan["insn"],
        "insn_bytes": plan["bytes"],
        "next_pc": plan["next_pc"],
        "fault": fault,
        "fault_pc": pc if fault else 0,
        "request_error": req["error"],
        "mem_fault": mem_fault,
        "invalid_frontend": req["invalid_frontend"],
        "noncanonical": req["noncanonical"],
        "align": req["align"],
        "range": req["range"],
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    assert "poly_raw_fetch_request fetch_request" in text
    assert "poly_raw_fetch_plan fetch_plan" in text

    idle = stage(False, c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, 0, c)
    assert not idle["mem_req"] and not idle["fault"] and not idle["insn_valid"]

    waiting = stage(True, c["POLY_FRONTEND_RISCV"], 0x8000, False, False, 0, c)
    assert waiting["mem_req"] and waiting["wait"]
    assert not waiting["fault"] and not waiting["insn_valid"]

    aarch64 = stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, 0xD503201F, c
    )
    assert aarch64["mem_req"] and not aarch64["wait"]
    assert aarch64["insn_valid"] and aarch64["insn"] == 0xD503201F
    assert aarch64["insn_bytes"] == 4 and aarch64["next_pc"] == 0x4004

    riscv32 = stage(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, True, False, 0x0000700B, c
    )
    assert riscv32["insn_valid"] and riscv32["insn_bytes"] == 4
    assert riscv32["next_pc"] == 0x8004

    riscv16 = stage(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, True, False, 0xFFFF0001, c
    )
    assert riscv16["insn_valid"] and riscv16["insn"] == 1
    assert riscv16["insn_bytes"] == 2 and riscv16["next_pc"] == 0x8002

    mem_fault = stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, True, 0, c
    )
    assert mem_fault["mem_req"] and mem_fault["fault"] and mem_fault["mem_fault"]
    assert not mem_fault["insn_valid"] and mem_fault["fault_pc"] == 0x4000

    bad_align = stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4002, True, False, 0, c
    )
    assert not bad_align["mem_req"]
    assert bad_align["fault"] and bad_align["request_error"] and bad_align["align"]

    bad_frontend = stage(True, 3, 0x4000, True, False, 0, c)
    assert bad_frontend["fault"] and bad_frontend["invalid_frontend"]

    bad_range = stage(
        True, c["POLY_FRONTEND_RISCV"], 0xFFFFFFFFFFFFFFFE, True, False, 0, c
    )
    assert bad_range["fault"] and bad_range["range"]

    print("POLY_RTL_RAW_FETCH_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
