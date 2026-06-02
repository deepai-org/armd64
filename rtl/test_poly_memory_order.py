#!/usr/bin/env python3
"""Memory-ordering policy checks for rtl/poly_memory_order.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_memory_order.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().replace("(uint32_t) ", "")
        shift = re.search(r"(\d+)U?(?:LL)?\s*<<\s*(\d+)", expr)
        if shift:
            constants[name] = int(shift.group(1), 0) << int(shift.group(2), 0)
            continue
        expr = expr.rstrip("UuLl")
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


def order(
    valid: bool,
    frontend: int,
    load: bool,
    store: bool,
    atomic: bool,
    barrier: bool,
    older_store: bool,
    store_buffer_full: bool,
    c: dict[str, int],
) -> dict[str, bool]:
    frontend_valid = frontend in {
        c["POLY_FRONTEND_X86"],
        c["POLY_FRONTEND_AARCH64"],
        c["POLY_FRONTEND_RISCV"],
    }
    raw = frontend in {
        c["POLY_FRONTEND_AARCH64"],
        c["POLY_FRONTEND_RISCV"],
    }
    op_present = load or store or atomic or barrier
    invalid_frontend = valid and not frontend_valid
    invalid_op = valid and not op_present
    fault = invalid_frontend or invalid_op
    wait_store_buffer = valid and not fault and store_buffer_full and (store or atomic)
    wait_atomic_order = valid and not fault and atomic and older_store
    retire = valid and not fault and not wait_store_buffer and not wait_atomic_order
    barrier_noop = retire and barrier and raw
    return {
        "retire": retire,
        "enqueue_store": retire and store,
        "wait_store_buffer": wait_store_buffer,
        "wait_atomic_order": wait_atomic_order,
        "barrier_noop": barrier_noop,
        "aarch64_barrier_noop": (
            barrier_noop and frontend == c["POLY_FRONTEND_AARCH64"]
        ),
        "riscv_fence_noop": (
            barrier_noop and frontend == c["POLY_FRONTEND_RISCV"]
        ),
        "weak_reorder_allowed": False,
        "invalid_frontend": invalid_frontend,
        "invalid_op": invalid_op,
        "fault": fault,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    text = RTL.read_text()

    assert sv["POLY_FRONTEND_X86"] == c["POLY_FRONTEND_X86"]
    assert sv["POLY_FRONTEND_AARCH64"] == c["POLY_FRONTEND_AARCH64"]
    assert sv["POLY_FRONTEND_RISCV"] == c["POLY_FRONTEND_RISCV"]
    assert sv["POLY_MEMORY_MODEL_X86_TSO"] == c["POLY_MEMORY_MODEL_X86_TSO"]
    assert "weak_reorder_allowed_o =" in text
    assert "POLY_MEMORY_MODEL_X86_TSO != 32'd1" in text

    load_with_older_store = order(
        True, c["POLY_FRONTEND_AARCH64"], True, False, False, False,
        True, False, c
    )
    assert load_with_older_store["retire"]
    assert not load_with_older_store["weak_reorder_allowed"]
    assert not load_with_older_store["wait_atomic_order"]

    store_ok = order(
        True, c["POLY_FRONTEND_RISCV"], False, True, False, False,
        False, False, c
    )
    assert store_ok["retire"] and store_ok["enqueue_store"]

    store_full = order(
        True, c["POLY_FRONTEND_RISCV"], False, True, False, False,
        False, True, c
    )
    assert not store_full["retire"]
    assert store_full["wait_store_buffer"] and not store_full["enqueue_store"]

    atomic_waits_for_older_store = order(
        True, c["POLY_FRONTEND_AARCH64"], True, True, True, False,
        True, False, c
    )
    assert not atomic_waits_for_older_store["retire"]
    assert atomic_waits_for_older_store["wait_atomic_order"]

    atomic_waits_for_store_buffer = order(
        True, c["POLY_FRONTEND_AARCH64"], True, True, True, False,
        False, True, c
    )
    assert not atomic_waits_for_store_buffer["retire"]
    assert atomic_waits_for_store_buffer["wait_store_buffer"]

    atomic_ok = order(
        True, c["POLY_FRONTEND_X86"], True, True, True, False,
        False, False, c
    )
    assert atomic_ok["retire"] and atomic_ok["enqueue_store"]

    aarch64_barrier = order(
        True, c["POLY_FRONTEND_AARCH64"], False, False, False, True,
        True, True, c
    )
    assert aarch64_barrier["retire"]
    assert aarch64_barrier["barrier_noop"]
    assert aarch64_barrier["aarch64_barrier_noop"]

    riscv_fence = order(
        True, c["POLY_FRONTEND_RISCV"], False, False, False, True,
        True, True, c
    )
    assert riscv_fence["retire"]
    assert riscv_fence["barrier_noop"] and riscv_fence["riscv_fence_noop"]

    invalid_frontend = order(
        True, 3, True, False, False, False, False, False, c
    )
    assert invalid_frontend["fault"] and invalid_frontend["invalid_frontend"]
    assert not invalid_frontend["retire"]

    invalid_op = order(
        True, c["POLY_FRONTEND_X86"], False, False, False, False,
        False, False, c
    )
    assert invalid_op["fault"] and invalid_op["invalid_op"]
    assert not invalid_op["retire"]

    print("POLY_RTL_MEMORY_ORDER_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
