#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_frontend_fetch_issue.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_fetch_issue.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending_aliases: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().replace("(uint32_t) ", "").rstrip("UuLl")
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


def issue(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, int | bool]:
    x86 = frontend == c["POLY_FRONTEND_X86"]
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}

    x86_last = (pc + 15) & 0xFFFFFFFFFFFFFFFF
    x86_noncanonical = valid and x86 and (not canonical(pc) or not canonical(x86_last))
    x86_range = valid and x86 and x86_last < pc

    raw_last = (pc + 3) & 0xFFFFFFFFFFFFFFFF
    raw_noncanonical = valid and raw and (not canonical(pc) or not canonical(raw_last))
    raw_align = valid and raw and (
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    raw_range = valid and raw and raw_last < pc
    raw_error = raw_noncanonical or raw_align or raw_range

    invalid = valid and not x86 and not raw
    fault = invalid or x86_noncanonical or x86_range or raw_error

    return {
        "x86_valid": valid and x86 and not x86_noncanonical and not x86_range,
        "x86_addr": pc,
        "x86_bytes": 16 if x86 else 0,
        "raw_valid": valid and raw and not raw_error,
        "raw_addr": pc,
        "raw_bytes": 4 if raw else 0,
        "fault": fault,
        "invalid": invalid,
        "x86_noncanonical": x86_noncanonical,
        "x86_range": x86_range,
        "raw_error": raw_error,
        "raw_noncanonical": raw_noncanonical,
        "raw_align": raw_align,
        "raw_range": raw_range,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_fetch_issue",
        "poly_raw_fetch_request raw_fetch_request",
        ".valid_i(valid_i && raw_frontend)",
        "POLY_X86_FETCH_REQUEST_BYTES = 5'd16",
        "x86_last_addr = pc_i + POLY_X86_LAST_OFFSET",
        "x86_fetch_req_valid_o =",
        "raw_request_error_o",
        "fault_o =",
    ]:
        if needle not in text:
            raise AssertionError(f"missing fetch-issue wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    require_structural_wiring()

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]
    assert sv["POLY_X86_FETCH_REQUEST_BYTES"] == 16

    assert issue(False, c["POLY_FRONTEND_X86"], 0x1000, c) == {
        "x86_valid": False, "x86_addr": 0x1000, "x86_bytes": 16,
        "raw_valid": False, "raw_addr": 0x1000, "raw_bytes": 0,
        "fault": False, "invalid": False, "x86_noncanonical": False,
        "x86_range": False, "raw_error": False, "raw_noncanonical": False,
        "raw_align": False, "raw_range": False,
    }
    assert issue(True, c["POLY_FRONTEND_X86"], 0x1000, c) == {
        "x86_valid": True, "x86_addr": 0x1000, "x86_bytes": 16,
        "raw_valid": False, "raw_addr": 0x1000, "raw_bytes": 0,
        "fault": False, "invalid": False, "x86_noncanonical": False,
        "x86_range": False, "raw_error": False, "raw_noncanonical": False,
        "raw_align": False, "raw_range": False,
    }
    assert issue(True, c["POLY_FRONTEND_AARCH64"], 0x4000, c) == {
        "x86_valid": False, "x86_addr": 0x4000, "x86_bytes": 0,
        "raw_valid": True, "raw_addr": 0x4000, "raw_bytes": 4,
        "fault": False, "invalid": False, "x86_noncanonical": False,
        "x86_range": False, "raw_error": False, "raw_noncanonical": False,
        "raw_align": False, "raw_range": False,
    }
    assert issue(True, c["POLY_FRONTEND_RISCV"], 0x8002, c) == {
        "x86_valid": False, "x86_addr": 0x8002, "x86_bytes": 0,
        "raw_valid": True, "raw_addr": 0x8002, "raw_bytes": 4,
        "fault": False, "invalid": False, "x86_noncanonical": False,
        "x86_range": False, "raw_error": False, "raw_noncanonical": False,
        "raw_align": False, "raw_range": False,
    }

    invalid = issue(True, 3, 0x1000, c)
    assert invalid["fault"] and invalid["invalid"]
    assert not invalid["x86_valid"] and not invalid["raw_valid"]

    x86_bad_start = issue(True, c["POLY_FRONTEND_X86"], 0x0000800000000000, c)
    assert x86_bad_start["fault"] and x86_bad_start["x86_noncanonical"]
    assert not x86_bad_start["x86_valid"]

    x86_wrap = issue(True, c["POLY_FRONTEND_X86"], 0xFFFFFFFFFFFFFFF8, c)
    assert x86_wrap["fault"] and x86_wrap["x86_range"]
    assert not x86_wrap["x86_valid"]

    a64_bad_align = issue(True, c["POLY_FRONTEND_AARCH64"], 0x4002, c)
    assert a64_bad_align["fault"] and a64_bad_align["raw_align"]
    assert not a64_bad_align["raw_valid"]

    rv_bad_align = issue(True, c["POLY_FRONTEND_RISCV"], 0x8001, c)
    assert rv_bad_align["fault"] and rv_bad_align["raw_align"]
    assert not rv_bad_align["raw_valid"]

    raw_bad_end = issue(True, c["POLY_FRONTEND_RISCV"], (1 << 47) - 2, c)
    assert raw_bad_end["fault"] and raw_bad_end["raw_noncanonical"]
    assert not raw_bad_end["raw_valid"]

    print("POLY_RTL_FRONTEND_FETCH_ISSUE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
