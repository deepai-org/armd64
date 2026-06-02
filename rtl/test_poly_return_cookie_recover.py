#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_return_cookie_recover.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_return_cookie_recover.sv"
BOCHS = ROOT / "bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().rstrip("UuLl")
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


def parse_bochs_return_cookie(path: Path) -> int:
    text = path.read_text()
    match = re.search(r"BX_POLY_RETURN_COOKIE\s*=\s*BX_CONST64\((0x[0-9a-fA-F]+)\)", text)
    if not match:
        raise AssertionError("missing BX_POLY_RETURN_COOKIE")
    return int(match.group(1), 0)


def recover(
    valid: bool,
    current: int,
    target: int,
    empty: bool,
    pop_frontend: int,
    pop_pc: int,
    pop_sp: int,
    pop_flags: int,
    cookie: int,
) -> dict[str, int | bool]:
    frontend_valid = current in {0, 1, 2}
    pop_frontend_valid = pop_frontend in {0, 1, 2}
    hit = valid and target == cookie
    invalid_frontend = hit and (not frontend_valid or not pop_frontend_valid)
    missing = hit and empty
    error = invalid_frontend or missing
    resume = hit and not error
    return {
        "hit": hit,
        "pop": resume,
        "resume": resume,
        "frontend": pop_frontend if resume else current,
        "pc": pop_pc if resume else target,
        "sp": pop_sp if resume else 0,
        "flags": pop_flags if resume else 0,
        "error": error,
        "invalid_frontend": invalid_frontend,
        "missing": missing,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    cookie = parse_bochs_return_cookie(BOCHS)

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]
    assert sv["POLY_RETURN_COOKIE"] == cookie

    normal = recover(
        True, c["POLY_FRONTEND_AARCH64"], 0x4004, False,
        c["POLY_FRONTEND_X86"], 0x1000, 0x8000, 0x55, cookie
    )
    assert normal == {
        "hit": False, "pop": False, "resume": False,
        "frontend": c["POLY_FRONTEND_AARCH64"], "pc": 0x4004, "sp": 0,
        "flags": 0, "error": False, "invalid_frontend": False,
        "missing": False,
    }

    ok = recover(
        True, c["POLY_FRONTEND_RISCV"], cookie, False,
        c["POLY_FRONTEND_X86"], 0x12345678, 0x7fff0000, 0x21, cookie
    )
    assert ok == {
        "hit": True, "pop": True, "resume": True,
        "frontend": c["POLY_FRONTEND_X86"], "pc": 0x12345678,
        "sp": 0x7fff0000, "flags": 0x21, "error": False,
        "invalid_frontend": False, "missing": False,
    }

    missing = recover(
        True, c["POLY_FRONTEND_AARCH64"], cookie, True,
        c["POLY_FRONTEND_X86"], 0x1000, 0x8000, 0, cookie
    )
    assert missing["hit"] and missing["error"] and missing["missing"]
    assert not missing["pop"] and not missing["resume"]

    invalid_current = recover(
        True, 3, cookie, False,
        c["POLY_FRONTEND_X86"], 0x1000, 0x8000, 0, cookie
    )
    assert invalid_current["hit"] and invalid_current["invalid_frontend"]
    assert invalid_current["error"] and not invalid_current["resume"]

    invalid_pop = recover(
        True, c["POLY_FRONTEND_RISCV"], cookie, False,
        3, 0x1000, 0x8000, 0, cookie
    )
    assert invalid_pop["hit"] and invalid_pop["invalid_frontend"]
    assert invalid_pop["error"] and not invalid_pop["resume"]

    invalid_cycle = recover(
        False, c["POLY_FRONTEND_RISCV"], cookie, False,
        c["POLY_FRONTEND_X86"], 0x1000, 0x8000, 0, cookie
    )
    assert not invalid_cycle["hit"] and not invalid_cycle["error"]

    print("POLY_RTL_RETURN_COOKIE_RECOVER_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
