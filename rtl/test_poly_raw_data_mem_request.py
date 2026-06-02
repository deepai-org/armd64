#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_raw_data_mem_request.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_raw_data_mem_request.sv"


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


def aligned(addr: int, width: int) -> bool:
    if width == 1:
        return True
    if width == 2:
        return (addr & 1) == 0
    if width == 4:
        return (addr & 3) == 0
    if width == 8:
        return (addr & 7) == 0
    return False


def request(
    valid: bool,
    frontend: int,
    addr: int,
    load: bool,
    store: bool,
    atomic: bool,
    width: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    invalid_frontend = valid and frontend != c["POLY_FRONTEND_X86"] and not raw
    one_op = (load + store + atomic) == 1
    valid_width = width in {1, 2, 4, 8}
    last = (addr + (width - 1 if valid_width else 0)) & 0xFFFFFFFFFFFFFFFF
    invalid_op = valid and raw and not one_op
    invalid_width = valid and raw and one_op and not valid_width
    noncanonical = valid and raw and one_op and valid_width and (
        not canonical(addr) or not canonical(last)
    )
    align_fault = (
        valid and raw and one_op and valid_width and atomic and
        not aligned(addr, width)
    )
    range_fault = valid and raw and one_op and valid_width and last < addr
    error = (
        invalid_frontend or invalid_op or invalid_width or noncanonical or
        align_fault or range_fault
    )
    request_valid = valid and raw and not error
    return {
        "valid": request_valid,
        "addr": addr,
        "bytes": width if request_valid else 0,
        "load": request_valid and load,
        "store": request_valid and store,
        "atomic": request_valid and atomic,
        "error": error,
        "invalid_frontend": invalid_frontend,
        "invalid_op": invalid_op,
        "invalid_width": invalid_width,
        "noncanonical": noncanonical,
        "align": align_fault,
        "range": range_fault,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_raw_data_mem_request",
        "input  logic [63:0] addr_i",
        "input  logic [3:0]  access_bytes_i",
        "output logic        request_atomic_o",
        "function automatic logic canonical64",
        "function automatic logic naturally_aligned",
        "request_last_addr = addr_i + last_offset",
        "request_valid_o = valid_i && raw_frontend && !error_o",
    ]:
        if needle not in text:
            raise AssertionError(f"missing raw data-memory request structure: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    require_structural_wiring()

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]

    idle = request(False, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, False, 8, c)
    assert not idle["valid"] and not idle["error"]

    x86 = request(True, c["POLY_FRONTEND_X86"], 0x1000, True, False, False, 8, c)
    assert not x86["valid"] and not x86["error"]

    load = request(True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, False, 8, c)
    assert load == {
        "valid": True, "addr": 0x4000, "bytes": 8,
        "load": True, "store": False, "atomic": False, "error": False,
        "invalid_frontend": False, "invalid_op": False,
        "invalid_width": False, "noncanonical": False,
        "align": False, "range": False,
    }

    store = request(True, c["POLY_FRONTEND_RISCV"], 0x8004, False, True, False, 4, c)
    assert store["valid"] and store["store"] and store["bytes"] == 4

    atomic = request(True, c["POLY_FRONTEND_RISCV"], 0x8010, False, False, True, 8, c)
    assert atomic["valid"] and atomic["atomic"] and atomic["bytes"] == 8

    invalid_frontend = request(True, 3, 0x4000, True, False, False, 8, c)
    assert invalid_frontend["error"] and invalid_frontend["invalid_frontend"]

    no_op = request(True, c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, False, 8, c)
    assert no_op["error"] and no_op["invalid_op"]

    multi_op = request(True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, True, False, 8, c)
    assert multi_op["error"] and multi_op["invalid_op"]

    bad_width = request(True, c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, False, 3, c)
    assert bad_width["error"] and bad_width["invalid_width"]

    unaligned_load = request(True, c["POLY_FRONTEND_AARCH64"], 0x4002, True, False, False, 8, c)
    assert unaligned_load["valid"] and unaligned_load["load"] and not unaligned_load["align"]

    bad_align = request(True, c["POLY_FRONTEND_AARCH64"], 0x4002, False, False, True, 8, c)
    assert bad_align["error"] and bad_align["align"]

    bad_start = request(True, c["POLY_FRONTEND_AARCH64"], 0x0000800000000000, True, False, False, 8, c)
    assert bad_start["error"] and bad_start["noncanonical"]

    cross_canonical = (1 << 47) - 4
    bad_end = request(True, c["POLY_FRONTEND_RISCV"], cross_canonical, True, False, False, 8, c)
    assert bad_end["error"] and bad_end["noncanonical"]

    wrap = request(True, c["POLY_FRONTEND_RISCV"], 0xFFFFFFFFFFFFFFFC, True, False, False, 8, c)
    assert wrap["error"] and wrap["range"]

    print("POLY_RTL_RAW_DATA_MEM_REQUEST_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
