#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_interrupt_boundary.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_interrupt_boundary.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().rstrip("UuLl")
        shift = re.search(r"(\d+)U?(?:LL)?\s*<<\s*(\d+)", expr)
        if shift:
            constants[name] = int(shift.group(1), 0) << int(shift.group(2), 0)
            continue
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


def canonical(pc: int) -> bool:
    high = (pc >> 48) & 0xFFFF
    sign = (pc >> 47) & 1
    return high == (0xFFFF if sign else 0)


def aligned(frontend: int, pc: int) -> bool:
    if frontend == 1:
        return (pc & 3) == 0
    if frontend == 2:
        return (pc & 1) == 0
    return True


def boundary(
    valid: bool,
    enabled: bool,
    cpl3: bool,
    interrupt: bool,
    user_return: bool,
    current: int,
    current_pc: int,
    interrupted_valid: bool,
    interrupted_frontend: int,
    interrupted_pc: int,
    user_return_pc: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    current_raw = current in {
        c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    interrupted_raw = interrupted_frontend in {
        c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    interrupt_candidate = valid and enabled and cpl3 and interrupt and current_raw
    return_candidate = valid and enabled and cpl3 and user_return and interrupted_valid
    invalid_current_frontend = valid and interrupt and current not in {
        c["POLY_FRONTEND_X86"], c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    invalid_current_pc = (
        interrupt_candidate and
        (not canonical(current_pc) or not aligned(current, current_pc))
    )
    invalid_interrupted_frontend = return_candidate and not interrupted_raw
    invalid_interrupted_pc = (
        return_candidate and
        (not canonical(interrupted_pc) or
         not aligned(interrupted_frontend, interrupted_pc))
    )
    error = (
        invalid_current_frontend or invalid_current_pc or
        invalid_interrupted_frontend or invalid_interrupted_pc
    )
    enter = interrupt_candidate and not error
    restore = return_candidate and not error and user_return_pc == interrupted_pc
    return {
        "enter": enter,
        "save": enter,
        "saved_frontend": current if enter else c["POLY_FRONTEND_X86"],
        "saved_pc": current_pc if enter else 0,
        "restore": restore,
        "clear": restore,
        "next_frontend": (
            c["POLY_FRONTEND_X86"] if enter else
            interrupted_frontend if restore else
            current
        ),
        "next_pc": current_pc if enter or not restore else interrupted_pc,
        "error": error,
        "invalid_current_frontend": invalid_current_frontend,
        "invalid_current_pc": invalid_current_pc,
        "invalid_interrupted_frontend": invalid_interrupted_frontend,
        "invalid_interrupted_pc": invalid_interrupted_pc,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]

    raw_enter = boundary(
        True, True, True, True, False,
        c["POLY_FRONTEND_AARCH64"], 0x4000, False,
        c["POLY_FRONTEND_X86"], 0, 0, c
    )
    assert raw_enter == {
        "enter": True, "save": True,
        "saved_frontend": c["POLY_FRONTEND_AARCH64"], "saved_pc": 0x4000,
        "restore": False, "clear": False,
        "next_frontend": c["POLY_FRONTEND_X86"], "next_pc": 0x4000,
        "error": False, "invalid_current_frontend": False,
        "invalid_current_pc": False, "invalid_interrupted_frontend": False,
        "invalid_interrupted_pc": False,
    }

    x86_interrupt = boundary(
        True, True, True, True, False,
        c["POLY_FRONTEND_X86"], 0x1000, False,
        c["POLY_FRONTEND_X86"], 0, 0, c
    )
    assert not x86_interrupt["enter"] and not x86_interrupt["error"]

    cpl0_interrupt = boundary(
        True, True, False, True, False,
        c["POLY_FRONTEND_RISCV"], 0x8000, False,
        c["POLY_FRONTEND_X86"], 0, 0, c
    )
    assert not cpl0_interrupt["enter"] and not cpl0_interrupt["error"]

    bad_pc = boundary(
        True, True, True, True, False,
        c["POLY_FRONTEND_AARCH64"], 0x4002, False,
        c["POLY_FRONTEND_X86"], 0, 0, c
    )
    assert bad_pc["error"] and bad_pc["invalid_current_pc"]
    assert not bad_pc["enter"]

    restore = boundary(
        True, True, True, False, True,
        c["POLY_FRONTEND_X86"], 0x4000, True,
        c["POLY_FRONTEND_RISCV"], 0x8000, 0x8000, c
    )
    assert restore["restore"] and restore["clear"]
    assert restore["next_frontend"] == c["POLY_FRONTEND_RISCV"]
    assert restore["next_pc"] == 0x8000

    mismatch = boundary(
        True, True, True, False, True,
        c["POLY_FRONTEND_X86"], 0x4000, True,
        c["POLY_FRONTEND_RISCV"], 0x8000, 0x8002, c
    )
    assert not mismatch["restore"] and not mismatch["clear"]
    assert not mismatch["error"]

    invalid_mode = boundary(
        True, True, True, False, True,
        c["POLY_FRONTEND_X86"], 0x4000, True,
        c["POLY_FRONTEND_X86"], 0x4000, 0x4000, c
    )
    assert invalid_mode["error"] and invalid_mode["invalid_interrupted_frontend"]

    invalid_interrupted_pc = boundary(
        True, True, True, False, True,
        c["POLY_FRONTEND_X86"], 0x4000, True,
        c["POLY_FRONTEND_RISCV"], 0x0000800000000000, 0x0000800000000000, c
    )
    assert invalid_interrupted_pc["error"]
    assert invalid_interrupted_pc["invalid_interrupted_pc"]

    print("POLY_RTL_INTERRUPT_BOUNDARY_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
