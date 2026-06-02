#!/usr/bin/env python3
"""Integration checks for rtl/poly_frontend_stateful_core.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_stateful_core.sv"


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


class StatefulCoreModel:
    def __init__(self, c: dict[str, int]):
        self.c = c
        self.frontend = c["POLY_FRONTEND_X86"]
        self.pc = 0

    def init(self, frontend: int, pc: int) -> None:
        self.frontend = frontend
        self.pc = pc

    def commit(self, frontend: int, pc: int, fault: bool = False, stall: bool = False) -> bool:
        if fault or stall:
            return False
        self.frontend = frontend
        self.pc = pc
        return True

    def interrupt_update(self, frontend: int, pc: int, fault: bool = False) -> bool:
        if fault:
            return False
        self.frontend = frontend
        self.pc = pc
        return True

    def return_resume(self, frontend: int, pc: int, fault: bool = False) -> bool:
        if fault:
            return False
        self.frontend = frontend
        self.pc = pc
        return True


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_stateful_core",
        "poly_frontend_core frontend_core",
        "poly_frontend_state frontend_state",
        ".frontend_i(state_frontend)",
        ".pc_i(state_pc)",
        "redirect_valid_o",
        "redirect_frontend_o",
        "redirect_pc_o",
        "redirect_reason_o",
        ".x86_fetch_req_valid_o(x86_fetch_req_valid_o)",
        ".x86_fetch_req_addr_o(x86_fetch_req_addr_o)",
        ".x86_fetch_req_bytes_o(x86_fetch_req_bytes_o)",
        "input  logic        raw_branch_resolved_i",
        "input  logic        raw_branch_taken_i",
        "input  logic [63:0] raw_branch_target_i",
        "input  logic        raw_memory_resolved_i",
        "input  logic        raw_memory_fault_i",
        ".raw_branch_resolved_i(raw_branch_resolved_i)",
        ".raw_branch_taken_i(raw_branch_taken_i)",
        ".raw_branch_target_i(raw_branch_target_i)",
        ".raw_memory_resolved_i(raw_memory_resolved_i)",
        ".raw_memory_fault_i(raw_memory_fault_i)",
        "assign state_interrupt_update =",
        "interrupt_enter_x86_o || interrupt_restore_raw_o",
        "assign state_stall = wait_fetch_o || wait_execute_o;",
        "assign state_commit_frontend =",
        "raw_branch_target_valid ? state_frontend : commit_frontend_o;",
        "assign state_commit_pc =",
        "raw_branch_target_valid ? raw_branch_target : commit_pc_o;",
        ".raw_branch_target_valid_o(raw_branch_target_valid)",
        ".raw_branch_target_o(raw_branch_target)",
        ".commit_i(retire_o)",
        ".commit_frontend_i(state_commit_frontend)",
        ".commit_pc_i(state_commit_pc)",
        ".interrupt_restore_i(state_interrupt_update)",
        ".interrupt_frontend_i(interrupt_next_frontend_o)",
        ".interrupt_pc_i(interrupt_next_pc_o)",
        ".return_resume_i(return_resume_o)",
        ".return_frontend_i(return_resume_frontend_o)",
        ".return_pc_i(return_resume_pc_o)",
        ".redirect_valid_o(redirect_valid_o)",
        ".redirect_frontend_o(redirect_frontend_o)",
        ".redirect_pc_o(redirect_pc_o)",
        ".redirect_reason_o(redirect_reason_o)",
        ".x86_fetch_wait_o(x86_fetch_wait_o)",
        ".x86_request_error_o(x86_request_error_o)",
        ".x86_mem_fault_o(x86_mem_fault_o)",
        ".x86_noncanonical_pc_o(x86_noncanonical_pc_o)",
        ".x86_range_fault_o(x86_range_fault_o)",
        ".fault_i(fault_o)",
        ".stall_i(state_stall)",
    ]:
        if needle not in text:
            raise AssertionError(f"missing stateful-core wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    model = StatefulCoreModel(c)
    model.init(c["POLY_FRONTEND_X86"], 0x1000)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_X86"], 0x1000)

    assert model.commit(c["POLY_FRONTEND_AARCH64"], 0x4000)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_AARCH64"], 0x4000)

    assert not model.commit(c["POLY_FRONTEND_RISCV"], 0x8000, stall=True)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_AARCH64"], 0x4000)

    assert not model.commit(c["POLY_FRONTEND_RISCV"], 0x8000, fault=True)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_AARCH64"], 0x4000)

    assert model.interrupt_update(c["POLY_FRONTEND_X86"], 0x4000)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_X86"], 0x4000)

    assert model.interrupt_update(c["POLY_FRONTEND_RISCV"], 0x8000)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_RISCV"], 0x8000)

    assert model.return_resume(c["POLY_FRONTEND_X86"], 0x1200)
    assert (model.frontend, model.pc) == (c["POLY_FRONTEND_X86"], 0x1200)

    print("POLY_RTL_FRONTEND_STATEFUL_CORE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
