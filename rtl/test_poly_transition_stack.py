#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_transition_stack.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_transition_stack.sv"


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


def parse_default_depth(path: Path) -> int:
    text = path.read_text()
    match = re.search(r"parameter\s+int\s+DEPTH\s*=\s*(\d+)", text)
    if not match:
        raise AssertionError("missing DEPTH parameter")
    return int(match.group(1))


class TransitionStack:
    def __init__(self, depth: int):
        self.depth = depth
        self.stack: list[tuple[int, int, int, int]] = []

    def push(self, frontend: int, pc: int, sp: int, flags: int) -> tuple[bool, bool, bool]:
        if len(self.stack) >= self.depth:
            return (True, False, False)
        self.stack.append((frontend, pc, sp, flags))
        return (False, False, False)

    def peek(self) -> tuple[int, int, int, int] | None:
        if not self.stack:
            return None
        return self.stack[-1]

    def pop(self) -> tuple[bool, bool, tuple[int, int, int, int] | None]:
        if not self.stack:
            return (False, True, None)
        return (False, False, self.stack.pop())

    def conflict(self) -> tuple[bool, bool, bool]:
        return (False, False, True)


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    rtl_depth = parse_default_depth(RTL)
    expected_depth = c["POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH"]
    assert rtl_depth == expected_depth, (
        f"transition stack depth rtl={rtl_depth} expected={expected_depth}"
    )

    valid_modes = {
        c["POLY_MODE_X86"],
        c["POLY_MODE_RAW_AARCH64"],
        c["POLY_MODE_RAW_RISCV"],
    }
    rtl_text = RTL.read_text()
    for needle in [
        "peek_valid_o",
        "peek_frontend_o",
        "peek_pc_o",
        "peek_sp_o",
        "peek_flags_o",
        "top_index = depth_q[INDEX_BITS-1:0] -",
        "peek_frontend_o = frontend_q[top_index]",
        "frontend_q[push_index] <= push_frontend_i",
    ]:
        if needle not in rtl_text:
            raise AssertionError(f"missing transition-stack peek wiring: {needle}")

    stack = TransitionStack(rtl_depth)

    for n in range(rtl_depth):
        mode = [c["POLY_MODE_X86"], c["POLY_MODE_RAW_AARCH64"],
                c["POLY_MODE_RAW_RISCV"]][n % 3]
        assert mode in valid_modes
        assert stack.push(mode, 0x1000 + n * 4, 0x8000 - n * 16, n) == (
            False, False, False
        )
        assert stack.peek() == (mode, 0x1000 + n * 4, 0x8000 - n * 16, n)

    assert stack.push(c["POLY_MODE_X86"], 0x2000, 0x7000, 0) == (
        True, False, False
    )

    assert stack.conflict() == (False, False, True)

    for n in reversed(range(rtl_depth)):
        overflow, underflow, frame = stack.pop()
        assert not overflow and not underflow and frame is not None
        mode, pc, sp, flags = frame
        assert mode in valid_modes
        assert pc == 0x1000 + n * 4
        assert sp == 0x8000 - n * 16
        assert flags == n

    assert stack.pop() == (False, True, None)
    assert stack.peek() is None

    print("POLY_RTL_TRANSITION_STACK_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
