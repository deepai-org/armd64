#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_frontend_state.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_state.sv"


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


def canonical(pc: int) -> bool:
    high = (pc >> 48) & 0xFFFF
    sign = (pc >> 47) & 1
    return high == (0xFFFF if sign else 0)


def aligned(frontend: int, pc: int, c: dict[str, int]) -> bool:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return (pc & 3) == 0
    if frontend == c["POLY_FRONTEND_RISCV"]:
        return (pc & 1) == 0
    return True


class FrontendState:
    def __init__(self, c: dict[str, int]):
        self.c = c
        self.frontend = c["POLY_FRONTEND_X86"]
        self.pc = 0

    def step(
        self,
        *,
        init: tuple[int, int] | None = None,
        commit: tuple[int, int] | None = None,
        interrupt_restore: tuple[int, int] | None = None,
        return_resume: tuple[int, int] | None = None,
        fault: bool = False,
        stall: bool = False,
    ) -> dict[str, int | bool]:
        requests = [
            value is not None for value in (commit, interrupt_restore, return_resume)
        ]
        conflict = init is None and sum(requests) > 1
        selected = (
            init if init is not None else
            return_resume if return_resume is not None else
            interrupt_restore if interrupt_restore is not None else
            commit if commit is not None else
            (self.frontend, self.pc)
        )
        request_valid = init is not None or any(requests)
        frontend, pc = selected
        valid_frontend = frontend in {
            self.c["POLY_FRONTEND_X86"],
            self.c["POLY_FRONTEND_AARCH64"],
            self.c["POLY_FRONTEND_RISCV"],
        }
        invalid_frontend = request_valid and not conflict and not valid_frontend
        invalid_pc = (
            request_valid and not conflict and
            (not canonical(pc) or not aligned(frontend, pc, self.c))
        )
        error = conflict or invalid_frontend or invalid_pc
        update = request_valid and not fault and not stall and not error
        if update:
            self.frontend, self.pc = frontend, pc
        return {
            "frontend": self.frontend,
            "pc": self.pc,
            "update": update,
            "hold": not update,
            "conflict": conflict,
            "invalid_frontend": invalid_frontend,
            "invalid_pc": invalid_pc,
            "error": error,
        }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_state",
        "always_ff @(posedge clk_i or negedge rst_ni)",
        "frontend_q <= POLY_FRONTEND_X86;",
        "commit_i && interrupt_restore_i",
        "return_resume_i",
        "canonical64",
        "frontend_aligned",
        "update_o =",
        "fault_i",
        "stall_i",
    ]:
        if needle not in text:
            raise AssertionError(f"missing frontend-state wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    state = FrontendState(c)
    assert state.step(init=(c["POLY_FRONTEND_X86"], 0x1000)) == {
        "frontend": c["POLY_FRONTEND_X86"], "pc": 0x1000,
        "update": True, "hold": False, "conflict": False,
        "invalid_frontend": False, "invalid_pc": False, "error": False,
    }

    switch = state.step(commit=(c["POLY_FRONTEND_AARCH64"], 0x4000))
    assert switch["update"]
    assert switch["frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert switch["pc"] == 0x4000

    bad_align = state.step(commit=(c["POLY_FRONTEND_AARCH64"], 0x4002))
    assert bad_align["error"] and bad_align["invalid_pc"]
    assert not bad_align["update"]
    assert state.frontend == c["POLY_FRONTEND_AARCH64"] and state.pc == 0x4000

    stalled = state.step(commit=(c["POLY_FRONTEND_RISCV"], 0x8000), stall=True)
    assert stalled["hold"] and not stalled["update"]
    assert state.frontend == c["POLY_FRONTEND_AARCH64"] and state.pc == 0x4000

    faulted = state.step(commit=(c["POLY_FRONTEND_RISCV"], 0x8000), fault=True)
    assert faulted["hold"] and not faulted["update"]
    assert state.frontend == c["POLY_FRONTEND_AARCH64"] and state.pc == 0x4000

    restored_interrupt = state.step(
        interrupt_restore=(c["POLY_FRONTEND_RISCV"], 0x8000)
    )
    assert restored_interrupt["update"]
    assert state.frontend == c["POLY_FRONTEND_RISCV"] and state.pc == 0x8000

    return_resume = state.step(return_resume=(c["POLY_FRONTEND_X86"], 0x1200))
    assert return_resume["update"]
    assert state.frontend == c["POLY_FRONTEND_X86"] and state.pc == 0x1200

    conflict = state.step(
        commit=(c["POLY_FRONTEND_AARCH64"], 0x4000),
        return_resume=(c["POLY_FRONTEND_X86"], 0x1300),
    )
    assert conflict["error"] and conflict["conflict"]
    assert not conflict["update"]
    assert state.frontend == c["POLY_FRONTEND_X86"] and state.pc == 0x1200

    bad_frontend = state.step(commit=(3, 0x1000))
    assert bad_frontend["error"] and bad_frontend["invalid_frontend"]
    assert not bad_frontend["update"]

    bad_canonical = state.step(commit=(c["POLY_FRONTEND_X86"], 0x0000800000000000))
    assert bad_canonical["error"] and bad_canonical["invalid_pc"]
    assert not bad_canonical["update"]

    init_overrides_conflict = state.step(
        init=(c["POLY_FRONTEND_X86"], 0x2000),
        commit=(c["POLY_FRONTEND_AARCH64"], 0x4000),
        return_resume=(c["POLY_FRONTEND_RISCV"], 0x8000),
    )
    assert init_overrides_conflict["update"] and not init_overrides_conflict["conflict"]
    assert state.frontend == c["POLY_FRONTEND_X86"] and state.pc == 0x2000

    print("POLY_RTL_FRONTEND_STATE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
