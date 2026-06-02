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


def trap_flags(c: dict[str, int]) -> int:
    return (
        c["POLY_TRAP_PACKET_FLAG_VECTOR_DELIVERY"] |
        c["POLY_TRAP_PACKET_FLAG_NO_VECTOR_X86_EXCEPTIONS"] |
        c["POLY_TRAP_PACKET_FLAG_TRAP_RETURN_RESTORE"] |
        c["POLY_TRAP_PACKET_FLAG_ALL_FRONTEND_HANDLERS"] |
        c["POLY_TRAP_PACKET_FLAG_OPAQUE_SYSCALLS"] |
        c["POLY_TRAP_PACKET_FLAG_MONITOR_MEMORY"] |
        c["POLY_TRAP_PACKET_FLAG_OPAQUE_IMPORTS"]
    )


def vendor_regs() -> tuple[int, int, int]:
    vendor = b"PolyglotCPU!"
    ebx = int.from_bytes(vendor[0:4], "little")
    edx = int.from_bytes(vendor[4:8], "little")
    ecx = int.from_bytes(vendor[8:12], "little")
    return ebx, ecx, edx


def core_cpuid(valid: bool, leaf: int, c: dict[str, int]) -> tuple[bool, int, int, int, int]:
    if not valid:
        return (False, 0, 0, 0, 0)
    if leaf == c["POLY_CPUID_BASE"]:
        ebx, ecx, edx = vendor_regs()
        return (True, c["POLY_CPUID_MAX"], ebx, ecx, edx)
    return (False, 0, 0, 0, 0)


def abi_slot(slot: int, c: dict[str, int]) -> tuple[bool, int, int, bool]:
    slots = [
        (c["POLY_ABI_SIGNATURE_KIND_EXCHANGE"], c["POLY_ABI_REGISTER_MAP_EXCHANGE"]),
        (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS"],
         c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE"]),
        (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_I128"],
         c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE_I128"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS"], c["POLY_ABI_REGISTER_MAP_NATIVE"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_I128"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_I128"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_VEC128_U32"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_VEC128_U32"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_U32_F32"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_COMPACT_U32_F32"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_COMPACT_F32_U32"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_COMPACT_F32_U32"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_FP64"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP32"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_FP32"]),
        (c["POLY_ABI_SIGNATURE_KIND_SRET_X86_SYSV_REGS"],
         c["POLY_ABI_REGISTER_MAP_SRET_X86_SYSV_TO_NATIVE"]),
        (c["POLY_ABI_SIGNATURE_KIND_X86_SYSV_REGS_FP128_RET"],
         c["POLY_ABI_REGISTER_MAP_X86_SYSV_TO_NATIVE_FP128_RET"]),
        (c["POLY_ABI_SIGNATURE_KIND_NATIVE_SRET_REGS"],
         c["POLY_ABI_REGISTER_MAP_NATIVE_SRET"]),
    ]
    if slot >= len(slots):
        return (False, 0, 0, False)
    kind, register_map = slots[slot]
    tls_flag = c["POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE"]
    return (True, kind, register_map & 0x7F, bool(register_map & tls_flag))


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


class InterruptState:
    def __init__(self):
        self.valid = False
        self.frontend = 0
        self.pc = 0


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
    interrupt_state: InterruptState | None = None,
    interrupt_enabled: bool = False,
    cpl3: bool = True,
    interrupt: bool = False,
    user_return: bool = False,
    user_return_pc: int = 0,
    trap_valid: bool = False,
    trap_monitor_enabled: bool = True,
    trap_monitor_packet_addr: int = 0x457000,
    trap_reason: int = 1,
    trap_source_mode: int = 1,
    trap_mem_write_resp_valid: bool = False,
    trap_mem_write_fault: bool = False,
    cycle_memory_response_cycles: int = 0,
) -> dict[str, int | bool | tuple[int, int, int, int] | None]:
    interrupt_state = interrupt_state or InterruptState()
    current_raw = frontend in {
        c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    interrupted_raw = interrupt_state.frontend in {
        c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    interrupt_candidate = (
        valid and interrupt_enabled and cpl3 and interrupt and current_raw
    )
    return_candidate = (
        valid and interrupt_enabled and cpl3 and user_return and
        interrupt_state.valid
    )
    interrupt_invalid_current = (
        valid and interrupt and frontend not in {
            c["POLY_FRONTEND_X86"],
            c["POLY_FRONTEND_AARCH64"],
            c["POLY_FRONTEND_RISCV"],
        }
    )
    interrupt_invalid_pc = (
        interrupt_candidate and (not canonical(pc) or not aligned(frontend, pc, c))
    )
    interrupt_invalid_saved_frontend = return_candidate and not interrupted_raw
    interrupt_invalid_saved_pc = (
        return_candidate and
        (not canonical(interrupt_state.pc) or
         not aligned(interrupt_state.frontend, interrupt_state.pc, c))
    )
    interrupt_error = (
        interrupt_invalid_current or interrupt_invalid_pc or
        interrupt_invalid_saved_frontend or interrupt_invalid_saved_pc
    )
    interrupt_enter = interrupt_candidate and not interrupt_error
    interrupt_restore = (
        return_candidate and not interrupt_error and
        user_return_pc == interrupt_state.pc
    )
    trap_last = (trap_monitor_packet_addr + 127) & 0xFFFFFFFFFFFFFFFF
    trap_range_wrap = trap_last < trap_monitor_packet_addr
    trap_monitor_disabled = trap_valid and not trap_monitor_enabled
    trap_noncanonical = (
        trap_valid and trap_monitor_enabled and
        (not canonical(trap_monitor_packet_addr) or not canonical(trap_last))
    )
    trap_align_fault = (
        trap_valid and trap_monitor_enabled and
        (trap_monitor_packet_addr & 0x7) != 0
    )
    trap_range_fault = trap_valid and trap_monitor_enabled and trap_range_wrap
    trap_invalid_reason = (
        trap_valid and trap_monitor_enabled and
        trap_reason not in {
            c["POLY_TRAP_SYSCALL"], c["POLY_TRAP_BREAK"],
            c["POLY_TRAP_IMPORT"], c["POLY_TRAP_ILLEGAL"],
        }
    )
    trap_invalid_source = (
        trap_valid and trap_monitor_enabled and
        trap_source_mode not in {
            c["POLY_MODE_RAW_AARCH64"], c["POLY_MODE_RAW_RISCV"],
        }
    )
    trap_encode_error = (
        trap_monitor_disabled or trap_noncanonical or trap_align_fault or
        trap_range_fault or trap_invalid_reason or trap_invalid_source
    )
    trap_packet_valid = trap_valid and not trap_encode_error
    trap_wait = trap_packet_valid and not trap_mem_write_resp_valid
    trap_delivered = (
        trap_packet_valid and trap_mem_write_resp_valid and
        not trap_mem_write_fault
    )
    trap_packet_mem_fault = (
        trap_packet_valid and trap_mem_write_resp_valid and
        trap_mem_write_fault
    )
    trap_fault = trap_encode_error or trap_packet_mem_fault
    interrupt_block = interrupt_enter or interrupt_restore
    block_retire = interrupt_block or trap_wait or trap_delivered

    return_hit = return_valid and return_target == cookie
    return_missing = return_hit and not stack.frames
    return_blocked = return_hit and pop
    return_pop_raw = return_hit and not return_missing
    return_pop = return_pop_raw and not pop
    stack_unavailable = stack.full() or pop or return_pop
    x86_ctrl = (
        frontend == c["POLY_FRONTEND_X86"] and
        (word & 0x00FFFFFF) == 0x00FC3A0F and
        not (word >> 31)
    )
    x86_subop = (word >> 24) & 0x7F if x86_ctrl else 0
    x86_call_sig_imm = (
        x86_ctrl and
        c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] <= x86_subop <
        c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] +
        c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
    )
    pcall_slot = (
        x86_subop - c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"]
        if x86_call_sig_imm else 0
    )
    is_pcall = (
        valid and fetch_valid and frontend == c["POLY_FRONTEND_X86"] and
        (x86_subop == c["POLY_X86_CTRL_PCALL_SIG_MODE"] or x86_call_sig_imm)
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
    execute_fault = memory_fault or interrupt_error or trap_fault
    control_fault = bool(
        is_pcall and execute_ready and
        (target_error or not signature_valid or stack_unavailable)
    )
    retire = bool(
        valid and fetch_valid and execute_ready and not execute_fault and
        not older_fault and not block_retire and not control_fault
    )
    commit_push = bool(is_pcall and retire)
    popped = stack.pop() if (pop or return_pop) else None
    if commit_push:
        stack.push((frontend, return_pc, sp, flags))
    abi_valid, abi_kind, abi_map, abi_tls = abi_slot(pcall_slot, c)
    abi_signature_valid = commit_push and abi_valid
    cycle_op = 0
    if trap_packet_valid and not trap_fault:
        cycle_op = 4
    elif return_pop:
        cycle_op = 3
    elif commit_push:
        cycle_op = 2
    elif is_pcall and retire:
        cycle_op = 1
    cycle_supported = cycle_op in {1, 2, 3, 4}
    cycle_requires_signature = cycle_op == 2
    cycle_requires_stack = cycle_op in {2, 3}
    cycle_requires_packet = cycle_op == 4
    cycle_stack_ready = (
        (cycle_op == 2 and not stack_unavailable) or
        (cycle_op == 3 and return_pop) or
        cycle_op not in {2, 3}
    )
    cycle_packet_ready = not cycle_requires_packet or trap_packet_valid
    cycle_unsupported = cycle_requires_signature and not abi_signature_valid
    cycle_blocked = (
        cycle_supported and not cycle_unsupported and (
            (cycle_requires_signature and not abi_signature_valid) or
            (cycle_requires_stack and not cycle_stack_ready) or
            (cycle_requires_packet and not cycle_packet_ready)
        )
    )
    cycle_fixed = {1: 3, 2: 4, 3: 3, 4: 2}.get(cycle_op, 0)
    cycle_variable = cycle_memory_response_cycles if cycle_op == 4 else 0
    cycle_valid = cycle_supported and not cycle_unsupported and not cycle_blocked
    if interrupt_enter:
        interrupt_state.valid = True
        interrupt_state.frontend = frontend
        interrupt_state.pc = pc
    elif interrupt_restore:
        interrupt_state.valid = False
        interrupt_state.frontend = 0
        interrupt_state.pc = 0
    return {
        "retire": retire,
        "transition": bool(is_pcall and retire),
        "push": commit_push,
        "slot": pcall_slot if commit_push else 0,
        "abi_apply": commit_push,
        "abi_valid": abi_signature_valid,
        "abi_kind": abi_kind if abi_signature_valid else 0,
        "abi_map": abi_map if abi_signature_valid else 0,
        "abi_tls": abi_signature_valid and abi_tls,
        "cycle_valid": cycle_valid,
        "cycle_fixed": cycle_fixed,
        "cycle_variable": cycle_variable,
        "cycle_total": cycle_fixed + cycle_variable,
        "cycle_few": cycle_valid and cycle_op != 4 and cycle_fixed <= 4,
        "cycle_waits_memory": cycle_op == 4 and cycle_supported,
        "cycle_unsupported": cycle_unsupported,
        "cycle_blocked": cycle_blocked,
        "fault": control_fault,
        "execute_ready": execute_ready,
        "wait_execute": bool(valid and fetch_valid and not execute_ready and not execute_fault),
        "execute_fault": execute_fault,
        "wait_retire": bool(valid and block_retire and not older_fault and not execute_fault),
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
        "interrupt_enter": interrupt_enter,
        "interrupt_restore": interrupt_restore,
        "interrupt_error": interrupt_error,
        "interrupt_state_valid": interrupt_state.valid,
        "interrupt_state_frontend": interrupt_state.frontend,
        "interrupt_state_pc": interrupt_state.pc,
        "trap_mem_write_valid": trap_packet_valid,
        "trap_mem_write_bytes": 128,
        "trap_required_flags": trap_flags(c),
        "trap_wait": trap_wait,
        "trap_delivered": trap_delivered,
        "trap_fault": trap_fault,
        "trap_encode_error": trap_encode_error,
        "trap_packet_mem_fault": trap_packet_mem_fault,
        "depth": len(stack.frames),
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "poly_frontend_memory_retire frontend_memory_retire",
        ".x86_fetch_req_valid_o(x86_fetch_req_valid_o)",
        ".x86_fetch_req_addr_o(x86_fetch_req_addr_o)",
        ".x86_fetch_req_bytes_o(x86_fetch_req_bytes_o)",
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
        "logic effective_memory_order_valid;",
        "assign effective_memory_order_valid =",
        "memory_order_valid_i || raw_memory_order_valid;",
        "assign effective_memory_load = memory_load_i || raw_memory_load;",
        "assign effective_memory_store = memory_store_i || raw_memory_store;",
        "assign effective_memory_atomic = memory_atomic_i || raw_memory_atomic;",
        "assign effective_memory_barrier = memory_barrier_i || raw_memory_barrier;",
        "!effective_memory_order_valid || memory_retire_allowed_raw;",
        "assign memory_enqueue_store_o = retire_o && memory_enqueue_store_raw;",
        "assign memory_barrier_noop_o = retire_o && memory_barrier_noop_raw;",
        "assign raw_branch_target_valid_o =",
        "retire_o && raw_branch_target_valid && !poly_ctrl_o;",
        "raw_branch_target_valid_o ? raw_branch_target : 64'd0;",
        ".raw_branch_target_valid_o(raw_branch_target_valid)",
        ".raw_branch_target_o(raw_branch_target)",
        ".execute_ready_i(execute_ready)",
        ".block_retire_i(block_retire)",
        ".wait_execute_o(wait_execute_o)",
        ".wait_retire_o(wait_retire_o)",
        ".x86_fetch_wait_o(x86_fetch_wait_o)",
        ".x86_request_error_o(x86_request_error_o)",
        ".x86_mem_fault_o(x86_mem_fault_o)",
        ".x86_noncanonical_pc_o(x86_noncanonical_pc_o)",
        ".x86_range_fault_o(x86_range_fault_o)",
        "poly_interrupt_boundary interrupt_boundary",
        "poly_trap_packet_stage trap_packet_stage",
        "interrupted_valid_q",
        "trap_wait_response_o || trap_packet_delivered_o",
        "assign execute_fault =",
        "trap_fault_o",
        ".interrupted_valid_i(interrupted_valid_q)",
        ".enter_x86_interrupt_o(interrupt_enter_x86_o)",
        ".wait_response_o(trap_wait_response_o)",
        ".packet_delivered_o(trap_packet_delivered_o)",
        "poly_abi_signature_slots abi_signature_slots",
        ".select_slot_i(commit_signature_slot_o[3:0])",
        "assign abi_signature_apply_o = commit_push_transition_o;",
        "assign abi_signature_valid_o = commit_push_transition_o && abi_select_valid;",
        "poly_cpuid_rom cpuid_rom",
        ".valid_i(cpuid_valid_i)",
        ".leaf_i(cpuid_leaf_i)",
        ".hit_o(cpuid_hit_o)",
        "poly_transition_cycle_budget transition_cycle_budget",
        ".valid_i(cycle_valid)",
        ".op_i(cycle_op)",
        ".budget_valid_o(cycle_budget_valid_o)",
        "interrupted_valid_q <= 1'b1;",
        "interrupted_valid_q <= 1'b0;",
    ]:
        if needle not in text:
            raise AssertionError(f"missing core wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    depth = parse_default_depth(STACK_RTL)
    cookie = parse_bochs_return_cookie(BOCHS)
    assert depth == c["POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH"]
    require_structural_wiring()

    assert core_cpuid(True, c["POLY_CPUID_BASE"], c) == (
        True, c["POLY_CPUID_MAX"], *vendor_regs()
    )
    assert core_cpuid(True, c["POLY_CPUID_BASE"] + 2, c) == (False, 0, 0, 0, 0)
    assert core_cpuid(False, c["POLY_CPUID_BASE"], c) == (False, 0, 0, 0, 0)

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
    assert first["abi_apply"] and first["abi_valid"]
    assert first["slot"] == 0
    assert first["abi_kind"] == c["POLY_ABI_SIGNATURE_KIND_EXCHANGE"]
    assert first["abi_map"] == c["POLY_ABI_REGISTER_MAP_EXCHANGE"]
    assert first["cycle_valid"] and first["cycle_fixed"] == 4
    assert first["cycle_total"] == 4 and first["cycle_few"]
    assert stack.frames[-1] == (
        c["POLY_FRONTEND_X86"], 0x1004, 0x8000, 0x55
    )

    immediate_slot_call = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x1100,
        sp=0x7ff0,
        return_pc=0x1104,
        flags=0,
        word=x86_ctrl_word(c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 8),
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_RISCV"],
        target_pc=0x8000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        c=c,
    )
    assert immediate_slot_call["retire"] and immediate_slot_call["push"]
    assert immediate_slot_call["slot"] == 8
    assert immediate_slot_call["abi_apply"] and immediate_slot_call["abi_valid"]
    assert immediate_slot_call["abi_kind"] == c["POLY_ABI_SIGNATURE_KIND_NATIVE_REGS_FP64"]
    assert immediate_slot_call["abi_map"] == c["POLY_ABI_REGISTER_MAP_NATIVE_FP64"]
    assert immediate_slot_call["cycle_valid"] and immediate_slot_call["cycle_few"]

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
    assert recovered["cycle_valid"] and recovered["cycle_fixed"] == 3
    assert recovered["cycle_few"]

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

    interrupt_state = InterruptState()
    raw_interrupt = core_step(
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
        interrupt_state=interrupt_state,
        interrupt_enabled=True,
        interrupt=True,
        c=c,
    )
    assert raw_interrupt["interrupt_enter"]
    assert raw_interrupt["interrupt_state_valid"]
    assert raw_interrupt["interrupt_state_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert raw_interrupt["interrupt_state_pc"] == 0x4000
    assert not raw_interrupt["retire"]
    assert raw_interrupt["wait_retire"] and not raw_interrupt["execute_fault"]

    user_restore = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x4000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        interrupt_state=interrupt_state,
        interrupt_enabled=True,
        user_return=True,
        user_return_pc=0x4000,
        c=c,
    )
    assert user_restore["interrupt_restore"]
    assert not user_restore["interrupt_state_valid"]
    assert not user_restore["retire"]
    assert user_restore["wait_retire"] and not user_restore["execute_fault"]

    interrupt_state = InterruptState()
    interrupt_state.valid = True
    interrupt_state.frontend = c["POLY_FRONTEND_RISCV"]
    interrupt_state.pc = 0x8000
    mismatch_restore = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_X86"],
        pc=0x4000,
        sp=0x9000,
        return_pc=0,
        flags=0,
        word=0,
        fetch_valid=True,
        target_frontend=c["POLY_FRONTEND_X86"],
        target_pc=0x1000,
        signature_valid=True,
        pop=False,
        return_valid=False,
        return_target=0,
        cookie=cookie,
        interrupt_state=interrupt_state,
        interrupt_enabled=True,
        user_return=True,
        user_return_pc=0x8002,
        c=c,
    )
    assert not mismatch_restore["interrupt_restore"]
    assert mismatch_restore["interrupt_state_valid"]
    assert mismatch_restore["retire"]

    bad_interrupt = core_step(
        TransitionStack(depth),
        valid=True,
        frontend=c["POLY_FRONTEND_AARCH64"],
        pc=0x4002,
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
        interrupt_enabled=True,
        interrupt=True,
        c=c,
    )
    assert bad_interrupt["interrupt_error"] and bad_interrupt["execute_fault"]
    assert not bad_interrupt["wait_retire"] and not bad_interrupt["retire"]

    trap_wait = core_step(
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
        trap_valid=True,
        trap_reason=c["POLY_TRAP_SYSCALL"],
        trap_source_mode=c["POLY_MODE_RAW_AARCH64"],
        trap_mem_write_resp_valid=False,
        cycle_memory_response_cycles=7,
        c=c,
    )
    assert trap_wait["trap_mem_write_valid"] and trap_wait["trap_wait"]
    assert trap_wait["wait_retire"] and not trap_wait["retire"]
    assert not trap_wait["trap_fault"] and not trap_wait["execute_fault"]
    assert trap_wait["cycle_valid"] and trap_wait["cycle_waits_memory"]
    assert trap_wait["cycle_fixed"] == 2 and trap_wait["cycle_variable"] == 7
    assert trap_wait["cycle_total"] == 9 and not trap_wait["cycle_few"]

    trap_delivered = core_step(
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
        trap_valid=True,
        trap_reason=c["POLY_TRAP_IMPORT"],
        trap_source_mode=c["POLY_MODE_RAW_RISCV"],
        trap_mem_write_resp_valid=True,
        trap_mem_write_fault=False,
        cycle_memory_response_cycles=1,
        c=c,
    )
    assert trap_delivered["trap_delivered"] and trap_delivered["wait_retire"]
    assert trap_delivered["trap_required_flags"] == 0x7F
    assert not trap_delivered["retire"] and not trap_delivered["execute_fault"]
    assert trap_delivered["cycle_valid"] and trap_delivered["cycle_total"] == 3
    assert trap_delivered["cycle_waits_memory"] and not trap_delivered["cycle_few"]

    trap_packet_fault = core_step(
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
        trap_valid=True,
        trap_reason=c["POLY_TRAP_BREAK"],
        trap_source_mode=c["POLY_MODE_RAW_AARCH64"],
        trap_mem_write_resp_valid=True,
        trap_mem_write_fault=True,
        c=c,
    )
    assert trap_packet_fault["trap_fault"]
    assert trap_packet_fault["trap_packet_mem_fault"]
    assert trap_packet_fault["execute_fault"]
    assert not trap_packet_fault["wait_retire"] and not trap_packet_fault["retire"]
    assert not trap_packet_fault["cycle_valid"]

    trap_encode_fault = core_step(
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
        trap_valid=True,
        trap_monitor_enabled=False,
        trap_reason=c["POLY_TRAP_BREAK"],
        trap_source_mode=c["POLY_MODE_RAW_AARCH64"],
        trap_mem_write_resp_valid=True,
        c=c,
    )
    assert trap_encode_fault["trap_fault"] and trap_encode_fault["trap_encode_error"]
    assert not trap_encode_fault["trap_mem_write_valid"]
    assert trap_encode_fault["execute_fault"] and not trap_encode_fault["retire"]

    print("POLY_RTL_FRONTEND_CORE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
