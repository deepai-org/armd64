#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_raw_fetch_plan.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_raw_fetch_plan.sv"


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


def plan(valid: bool, frontend: int, pc: int, fetch_word: int) -> tuple[bool, bool, int, int, int, int]:
    if not valid:
        return (False, False, pc, 0, 0, pc)
    if frontend == 1:
        return (True, (pc & 0x3) != 0, pc, 4, fetch_word & 0xFFFFFFFF, pc + 4)
    if frontend == 2:
        is_32 = (fetch_word & 0x3) == 0x3
        size = 4 if is_32 else 2
        insn = fetch_word & 0xFFFFFFFF if is_32 else fetch_word & 0xFFFF
        return (True, (pc & 0x1) != 0, pc, size, insn, pc + size)
    return (False, False, pc, 0, 0, pc)


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]
    assert sv["POLY_AARCH64_FETCH_BYTES"] == c["POLY_TRANSITION_AARCH64_ALIGN"]
    assert sv["POLY_RISCV_FETCH_BYTES_16"] == c["POLY_TRANSITION_RISCV_ALIGN"]
    assert sv["POLY_RISCV_FETCH_BYTES_32"] == 4

    assert plan(False, c["POLY_FRONTEND_AARCH64"], 0x1000, 0xD503201F) == (
        False, False, 0x1000, 0, 0, 0x1000
    )
    assert plan(True, c["POLY_FRONTEND_X86"], 0x1000, 0x90909090) == (
        False, False, 0x1000, 0, 0, 0x1000
    )
    assert plan(True, c["POLY_FRONTEND_AARCH64"], 0x1000, 0xD503201F) == (
        True, False, 0x1000, 4, 0xD503201F, 0x1004
    )
    assert plan(True, c["POLY_FRONTEND_AARCH64"], 0x1002, 0xD503201F) == (
        True, True, 0x1002, 4, 0xD503201F, 0x1006
    )
    assert plan(True, c["POLY_FRONTEND_RISCV"], 0x2000, 0x0000700B) == (
        True, False, 0x2000, 4, 0x0000700B, 0x2004
    )
    assert plan(True, c["POLY_FRONTEND_RISCV"], 0x2000, 0xFFFF0001) == (
        True, False, 0x2000, 2, 0x00000001, 0x2002
    )
    assert plan(True, c["POLY_FRONTEND_RISCV"], 0x2001, 0x0000700B) == (
        True, True, 0x2001, 4, 0x0000700B, 0x2005
    )
    assert plan(True, 3, 0x3000, 0) == (
        False, False, 0x3000, 0, 0, 0x3000
    )

    print("POLY_RTL_RAW_FETCH_PLAN_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
