#!/usr/bin/env python3
"""Integration checks for rtl/poly_frontend_core.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_core.sv"
STACK_RTL = ROOT / "rtl/poly_transition_stack.sv"
BOCHS = ROOT / "bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"


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


def parse_default_depth(path: Path) -> int:
    text = path.read_text()
    match = re.search(r"parameter\s+int\s+DEPTH\s*=\s*(\d+)", text)
    if not match:
        raise AssertionError("missing transition-stack DEPTH parameter")
    return int(match.group(1))


def parse_bochs_return_cookie(path: Path) -> int:
    text = path.read_text()
    match = re.search(r"BX_POLY_RETURN_COOKIE\s*=\s*BX_CONST64\((0x[0-9a-fA-F]+)\)", text)
    if not match:
        raise AssertionError("missing BX_POLY_RETURN_COOKIE")
    return int(match.group(1), 0)


def x86_ctrl_word(subop: int) -> int:
    return 0x0F | (0x3A << 8) | (0xFC << 16) | ((subop & 0x7F) << 24)


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def aligned(frontend: int, pc: int, c: dict[str, int]) -> bool:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return (pc & 3) == 0
    if frontend == c["POLY_FRONTEND_RISCV"]:
        return (pc & 1) == 0
    return True


class TransitionStack:
    def __init__(self, depth: int):
        self.depth = depth
        self.frames: list[tuple[int, int, int, int]] = []
        self.overflow = False
        self.underflow = False
        self.conflict = False

    def full(self) -> bool:
        return len(self.frames) >= self.depth

    def push(self, frame: tuple[int, int, int, int]) -> None:
        if self.full():
            self.overflow = True
            return
        self.frames.append(frame)

    def pop(self) -> tuple[int, int, int, int] | None:
        if not self.frames:
            self.underflow = True
            return None
        return self.frames.pop()


def core_step(
    stack: TransitionStack,
    *,
    valid: bool,
    frontend: int,
    pc: int,
    sp: int,
    return_pc: int,
    flags: int,
    word: int,
    fetch_valid: bool,
    target_frontend: int,
    target_pc: int,
    signature_valid: bool,
    pop: bool,
    return_valid: bool,
    return_target: int,
    cookie: int,
    c: dict[str, int],
    memory_order_valid: bool = False,
    memory_load: bool = False,
    memory_store: bool = False,
    memory_atomic: bool = False,
    memory_barrier: bool = False,
    older_store_pending: bool = False,
    store_buffer_full: bool = False,
    older_fault: bool = False,
) -> dict[str, int | bool | tuple[int, int, int, int] | None]:
    return_hit = return_valid and return_target == cookie
    return_missing = return_hit and not stack.frames
    return_blocked = return_hit and pop
    return_pop_raw = return_hit and not return_missing
    return_pop = return_pop_raw and not pop
    stack_unavailable = stack.full() or pop or return_pop
    is_pcall = (
        valid and
        fetch_valid and
        frontend == c["POLY_FRONTEND_X86"] and
        word == x86_ctrl_word(c["POLY_X86_CTRL_PCALL_SIG_MODE"])
    )
    target_error = (
        target_frontend not in {
            c["POLY_FRONTEND_X86"],
            c["POLY_FRONTEND_AARCH64"],
            c["POLY_FRONTEND_RISCV"],
        } or
        not canonical(target_pc) or
        not aligned(target_frontend, target_pc, c)
    )
    memory_op_present = memory_load or memory_store or memory_atomic or memory_barrier
    memory_fault = memory_order_valid and not memory_op_present
    memory_wait_store = (
        memory_order_valid and not memory_fault and store_buffer_full and
        (memory_store or memory_atomic)
    )
    memory_wait_atomic = (
        memory_order_valid and not memory_fault and memory_atomic and
        older_store_pending
    )
    memory_retire_allowed = (
        memory_order_valid and not memory_fault and not memory_wait_store and
        not memory_wait_atomic
    )
    execute_ready = not memory_order_valid or memory_retire_allowed
    execute_fault = memory_fault
    control_fault = bool(
        is_pcall and execute_ready and
        (target_error or not signature_valid or stack_unavailable)
    )
    retire = bool(
        valid and fetch_valid and execute_ready and not execute_fault and
        not older_fault and not control_fault
    )
    commit_push = bool(is_pcall and retire)
    popped = stack.pop() if (pop or return_pop) else None
    if commit_push:
        stack.push((frontend, return_pc, sp, flags))
    return {
        "retire": retire,
        "transition": bool(is_pcall and retire),
        "push": commit_push,
        "fault": control_fault,
        "execute_ready": execute_ready,
        "wait_execute": bool(valid and fetch_valid and not execute_ready and not execute_fault),
        "execute_fault": execute_fault,
        "memory_retire_allowed": memory_retire_allowed,
        "memory_enqueue_store": retire and memory_retire_allowed and memory_store,
        "memory_wait_store": memory_wait_store,
        "memory_wait_atomic": memory_wait_atomic,
        "memory_fault": memory_fault,
        "stack_unavailable": stack_unavailable,
        "popped": popped,
        "return_hit": return_hit,
        "return_resume": return_pop,
        "return_error": return_missing or return_blocked,
        "return_missing": return_missing,
        "return_blocked": return_blocked,
        "depth": len(stack.frames),
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "poly_frontend_memory_retire frontend_memory_retire",
        "poly_transition_stack transition_stack",
        "transition_return_pc_i",
        "assign stack_unavailable = stack_full || stack_pop_request;",
        ".transition_stack_full_i(stack_unavailable)",
        ".push_i(commit_push_transition)",
        ".push_frontend_i(frontend_i)",
        ".push_pc_i(transition_return_pc_i)",
        ".push_sp_i(sp_i)",
        ".push_flags_i(transition_flags_i)",
        "poly_return_cookie_recover return_cookie_recover",
        "assign stack_pop_request = transition_pop_i || (return_pop_raw && !transition_pop_i);",
        ".peek_frontend_o(peek_frontend)",
        ".transition_empty_i(!peek_valid)",
        "poly_memory_order memory_order",
        "assign execute_ready = !memory_order_valid_i || memory_retire_allowed_raw;",
        "assign execute_fault = execute_fault_i || memory_fault_o;",
        "assign memory_enqueue_store_o = retire_o && memory_enqueue_store_raw;",
        "assign memory_barrier_noop_o = retire_o && memory_barrier_noop_raw;",
        ".execute_ready_i(execute_ready)",
        ".wait_execute_o(wait_execute_o)",
    ]:
        if needle not in text:
            raise AssertionError(f"missing core wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    depth = parse_default_depth(STACK_RTL)
    cookie = parse_bochs_return_cookie(BOCHS)
    assert depth == c["POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH"]
    require_structural_wiring()

    pcall = x86_ctrl_word(c["POLY_X86_CTRL_PCALL_SIG_MODE"])
    stack = TransitionStack(depth)
    first = core_step(
        stack,
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x1000,
        sp=0x8000,
        return_pc=0x1004,
        flags=0x55,
        word=pcall,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x2000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        c=c,
    )
    assert first["retire"] and first["transition"] and first["push"]
    assert first["depth"] == 1
    assert stack.frames[-1] == (
        c["POLY_FRONTEND_X86"], 0x1004, 0x8000, 0x55
    )

    popped = core_step(
        stack,
        valid=False,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0,
        sp=0,
        return_pc=0,
        flags=0,
        word=0,
        fetch_valid=False,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0,
        signature_valid=False,
        pop=True,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        c=c,
    )
    assert popped["popped"] == (c["POLY_FRONTEND_X86"], 0x1004, 0x8000, 0x55)
    assert popped["depth"] == 0

    full_stack = TransitionStack(depth)
    for index in range(depth):
        full_stack.push((c["POLY_FRONTEND_X86"], 0x3000 + index, 0x9000, 0))
    full = core_step(
        full_stack,
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x1200,
        sp=0x8100,
        return_pc=0x1204,
        flags=0,
        word=pcall,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x2400,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        c=c,
    )
    assert not full["retire"] and not full["push"] and full["fault"]
    assert full["stack_unavailable"]
    assert len(full_stack.frames) == depth

    conflict_free = core_step(
        full_stack,
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x1300,
        sp=0x8200,
        return_pc=0x1304,
        flags=0,
        word=pcall,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_RISCV"],
        target_pc=0x2600,
        signature_valid=True,
        pop=True,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        c=c,
    )
    assert not conflict_free["retire"] and not conflict_free["push"]
    assert conflict_free["popped"] is not None
    assert not full_stack.conflict

    return_stack = TransitionStack(depth)
    return_stack.push((c["POLY_FRONTEND_AARCH64"], 0x4400, 0x8800, 0x80))
    recovered = core_step(
        return_stack,
        valid=False,
        frontend=c["POLY_FRONTEND_RISCV"],
        pc=0,
        sp=0,
        return_pc=0,
        flags=0,
        word=0,
        fetch_valid=False,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0,
        signature_valid=False,
        pop=False,
        return_valid=True,
        return_target=cookie,
        cookie=cookie,
        c=c,
    )
    assert recovered["return_hit"] and recovered["return_resume"]
    assert recovered["popped"] == (
        c["POLY_FRONTEND_AARCH64"], 0x4400, 0x8800, 0x80
    )
    assert recovered["depth"] == 0

    blocked_stack = TransitionStack(depth)
    blocked_stack.push((c["POLY_FRONTEND_X86"], 0x5000, 0x9000, 0x81))
    blocked = core_step(
        blocked_stack,
        valid=False,
        frontend=c["POLY_FRONTEND_AARCH64"],
        pc=0,
        sp=0,
        return_pc=0,
        flags=0,
        word=0,
        fetch_valid=False,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0,
        signature_valid=False,
        pop=True,
        return_valid=True,
        return_target=cookie,
        cookie=cookie,
        c=c,
    )
    assert blocked["return_hit"] and blocked["return_blocked"]
    assert blocked["return_error"] and not blocked["return_resume"]
    assert blocked["popped"] == (c["POLY_FRONTEND_X86"], 0x5000, 0x9000, 0x81)

    memory_wait = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_RISCV"],
        pc=0x8000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0x00000033,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        memory_order_valid=True,
        memory_store=True,
        store_buffer_full=True,
        c=c,
    )
    assert memory_wait["memory_wait_store"] and memory_wait["wait_execute"]
    assert not memory_wait["retire"] and not memory_wait["fault"]
    assert not memory_wait["memory_enqueue_store"]

    memory_store = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_RISCV"],
        pc=0x8000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0x00000033,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        memory_order_valid=True,
        memory_store=True,
        c=c,
    )
    assert memory_store["retire"] and memory_store["memory_enqueue_store"]

    older_memory_store = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_RISCV"],
        pc=0x8000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0x00000033,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        memory_order_valid=True,
        memory_store=True,
        older_fault=True,
        c=c,
    )
    assert older_memory_store["memory_retire_allowed"]
    assert not older_memory_store["retire"]
    assert not older_memory_store["memory_enqueue_store"]

    memory_fault = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_AARCH64"],
        pc=0x4000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0x52800000,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        memory_order_valid=True,
        c=c,
    )
    assert memory_fault["memory_fault"] and memory_fault["execute_fault"]
    assert not memory_fault["retire"] and not memory_fault["wait_execute"]

    print("POLY_RTL_FRONTEND_CORE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
