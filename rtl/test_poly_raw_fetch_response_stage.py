#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_raw_fetch_response_stage.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_raw_fetch_response_stage.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().replace("(uint32_t) ", "").rstrip("UuLl")
        if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", expr):
            constants[name] = int(expr, 0)
            continue
        if re.fullmatch(r"POLY_[A-Z0-9_]+", expr):
            pending.append((name, expr))
    changed = True
    while changed:
        changed = False
        for name, alias in pending:
            if name not in constants and alias in constants:
                constants[name] = constants[alias]
                changed = True
    return constants


def plan(frontend: int, pc: int, word: int, c: dict[str, int]) -> dict[str, int | bool]:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return {
            "raw": True,
            "align": (pc & 3) != 0,
            "bytes": 4,
            "insn": word & 0xFFFFFFFF,
            "next": pc + 4,
        }
    if frontend == c["POLY_FRONTEND_RISCV"]:
        is_32 = (word & 3) == 3
        size = 4 if is_32 else 2
        return {
            "raw": True,
            "align": (pc & 1) != 0,
            "bytes": size,
            "insn": word & 0xFFFFFFFF if is_32 else word & 0xFFFF,
            "next": pc + size,
        }
    return {"raw": False, "align": False, "bytes": 0, "insn": 0, "next": pc}


def stage(
    valid: bool,
    frontend: int,
    pc: int,
    request_valid: bool,
    resp_valid: bool,
    resp_fault: bool,
    word: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    plan_valid = valid and request_valid and resp_valid and not resp_fault
    p = plan(frontend, pc, word, c) if plan_valid else {
        "raw": False, "align": False, "bytes": 0, "insn": 0, "next": pc,
    }
    mem_fault = valid and request_valid and resp_valid and resp_fault
    response_align = bool(plan_valid and p["align"])
    insn_valid = bool(p["raw"] and not p["align"])
    return {
        "wait": valid and request_valid and not resp_valid,
        "insn_valid": insn_valid,
        "insn": p["insn"] if insn_valid else 0,
        "bytes": p["bytes"] if insn_valid else 0,
        "next": p["next"] if insn_valid else pc,
        "fault": mem_fault or response_align,
        "mem_fault": mem_fault,
        "response_align": response_align,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_raw_fetch_response_stage",
        "poly_raw_fetch_plan fetch_plan",
        "wait_response_o = valid_i && request_valid_i && !mem_resp_valid_i",
        "plan_valid =",
        "insn_valid_o = plan_raw_fetch && !plan_align_fault",
        "fault_o = mem_fault_o || response_align_fault_o",
    ]:
        if needle not in text:
            raise AssertionError(f"missing raw-response-stage wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    assert stage(
        False, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, False, 0, c
    ) == {
        "wait": False, "insn_valid": False, "insn": 0, "bytes": 0,
        "next": 0x4000, "fault": False, "mem_fault": False,
        "response_align": False,
    }
    assert stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, False, 0, c
    )["wait"]

    a64 = stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, True, False,
        0x52800000, c
    )
    assert a64["insn_valid"] and a64["insn"] == 0x52800000
    assert a64["bytes"] == 4 and a64["next"] == 0x4004

    rv16 = stage(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, True, True, False,
        0xFFFF0001, c
    )
    assert rv16["insn_valid"] and rv16["insn"] == 1
    assert rv16["bytes"] == 2 and rv16["next"] == 0x8002

    mem_fault = stage(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, True, True, True, 0, c
    )
    assert mem_fault["fault"] and mem_fault["mem_fault"]
    assert not mem_fault["insn_valid"]

    bad_align = stage(
        True, c["POLY_FRONTEND_AARCH64"], 0x4002, True, True, False,
        0x52800000, c
    )
    assert bad_align["fault"] and bad_align["response_align"]
    assert not bad_align["insn_valid"]

    print("POLY_RTL_RAW_FETCH_RESPONSE_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
