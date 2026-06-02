#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_raw_fetch_request.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_raw_fetch_request.sv"


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


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def request(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, int | bool]:
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    invalid_frontend = valid and frontend != c["POLY_FRONTEND_X86"] and not raw
    last = (pc + 3) & 0xFFFFFFFFFFFFFFFF
    noncanonical = valid and raw and (not canonical(pc) or not canonical(last))
    align = valid and raw and (
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    range_fault = valid and raw and last < pc
    error = invalid_frontend or noncanonical or align or range_fault
    return {
        "valid": valid and raw and not error,
        "addr": pc,
        "bytes": 4 if raw else 0,
        "error": error,
        "invalid_frontend": invalid_frontend,
        "noncanonical": noncanonical,
        "align": align,
        "range": range_fault,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]
    assert sv["POLY_AARCH64_FETCH_REQUEST_BYTES"] == c["POLY_TRANSITION_AARCH64_ALIGN"]
    assert sv["POLY_RISCV_FETCH_REQUEST_BYTES"] == 4

    assert request(False, c["POLY_FRONTEND_AARCH64"], 0x4000, c) == {
        "valid": False, "addr": 0x4000, "bytes": 4, "error": False,
        "invalid_frontend": False, "noncanonical": False, "align": False,
        "range": False,
    }
    assert request(True, c["POLY_FRONTEND_X86"], 0x1000, c) == {
        "valid": False, "addr": 0x1000, "bytes": 0, "error": False,
        "invalid_frontend": False, "noncanonical": False, "align": False,
        "range": False,
    }
    assert request(True, c["POLY_FRONTEND_AARCH64"], 0x4000, c) == {
        "valid": True, "addr": 0x4000, "bytes": 4, "error": False,
        "invalid_frontend": False, "noncanonical": False, "align": False,
        "range": False,
    }
    assert request(True, c["POLY_FRONTEND_RISCV"], 0x8002, c) == {
        "valid": True, "addr": 0x8002, "bytes": 4, "error": False,
        "invalid_frontend": False, "noncanonical": False, "align": False,
        "range": False,
    }

    invalid = request(True, 3, 0x4000, c)
    assert invalid["error"] and invalid["invalid_frontend"] and not invalid["valid"]

    bad_start = request(True, c["POLY_FRONTEND_AARCH64"], 0x0000800000000000, c)
    assert bad_start["error"] and bad_start["noncanonical"] and not bad_start["valid"]

    cross_canonical = (1 << 47) - 2
    bad_end = request(True, c["POLY_FRONTEND_RISCV"], cross_canonical, c)
    assert bad_end["error"] and bad_end["noncanonical"] and not bad_end["valid"]

    bad_a64_align = request(True, c["POLY_FRONTEND_AARCH64"], 0x4002, c)
    assert bad_a64_align["error"] and bad_a64_align["align"]

    bad_rv_align = request(True, c["POLY_FRONTEND_RISCV"], 0x8001, c)
    assert bad_rv_align["error"] and bad_rv_align["align"]

    wrap = request(True, c["POLY_FRONTEND_RISCV"], 0xFFFFFFFFFFFFFFFE, c)
    assert wrap["error"] and wrap["range"]

    print("POLY_RTL_RAW_FETCH_REQUEST_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
