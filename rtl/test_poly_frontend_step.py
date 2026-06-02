#!/usr/bin/env python3
"""Integration checks for rtl/poly_frontend_step.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_step.sv"


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


def x86_ctrl_word(subop: int) -> int:
    return 0x0F | (0x3A << 8) | (0xFC << 16) | ((subop & 0x7F) << 24)


def aarch64_ctrl_word(subop: int) -> int:
    return 0xD503201F | ((subop & 0x7F) << 5)


def riscv_ctrl_word(subop: int) -> int:
    return 0x0000700B | ((subop & 0x7F) << 25)


def raw_plan(valid: bool, frontend: int, pc: int, word: int) -> tuple[bool, bool, int, int, int, int]:
    if not valid:
        return (False, False, pc, 0, 0, pc)
    if frontend == 1:
        return (True, (pc & 3) != 0, pc, 4, word & 0xFFFFFFFF, pc + 4)
    if frontend == 2:
        is_32 = (word & 3) == 3
        size = 4 if is_32 else 2
        insn = word & 0xFFFFFFFF if is_32 else word & 0xFFFF
        return (True, (pc & 1) != 0, pc, size, insn, pc + size)
    return (False, False, pc, 0, 0, pc)


def decode(valid: bool, frontend: int, insn: int, c: dict[str, int]) -> tuple[bool, int, bool, int]:
    if not valid:
        return (False, 0, False, 0)
    if frontend == c["POLY_FRONTEND_X86"] and (insn & 0x00FFFFFF) == 0x00FC3A0F and not (insn >> 31):
        subop = (insn >> 24) & 0x7F
        call_sig = c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (True, subop, call_sig, subop - c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] if call_sig else 0)
    if frontend == c["POLY_FRONTEND_AARCH64"] and (insn & 0xFFFFF01F) == 0xD503201F:
        subop = (insn >> 5) & 0x7F
        call_sig = c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (True, subop, call_sig, subop - c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] if call_sig else 0)
    if frontend == c["POLY_FRONTEND_RISCV"] and (insn & 0x01FFFFFF) == 0x0000700B:
        subop = (insn >> 25) & 0x7F
        call_sig = c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (True, subop, call_sig, subop - c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] if call_sig else 0)
    return (False, 0, False, 0)


def canonical(pc: int) -> bool:
    high = (pc >> 48) & 0xFFFF
    return high == 0 or high == 0xFFFF


def aligned(frontend: int, pc: int) -> bool:
    return (frontend != 1 or (pc & 3) == 0) and (frontend != 2 or (pc & 1) == 0)


def step(
    valid: bool,
    frontend: int,
    pc: int,
    word: int,
    x86_fallthrough: int,
    target_frontend: int,
    target_pc: int,
    signature_valid: bool,
    stack_full: bool,
    c: dict[str, int],
) -> dict[str, int | bool]:
    raw, raw_align, fetch_addr, fetch_bytes, raw_insn, raw_next = raw_plan(
        valid, frontend, pc, word
    )
    insn = raw_insn if raw else word
    fallthrough = raw_next if raw else x86_fallthrough
    poly, subop, call_sig, slot = decode(valid and not raw_align, frontend, insn, c)

    switch = call = trap_return = landing = sig_required = False
    effective_target = target_frontend
    if poly:
        if frontend == c["POLY_FRONTEND_X86"]:
            switch = subop in {c["POLY_X86_CTRL_PENTER_MODE"], c["POLY_X86_CTRL_PSWITCH_MODE"]}
            call = subop == c["POLY_X86_CTRL_PCALL_SIG_MODE"] or call_sig
            sig_required = call
            trap_return = subop == c["POLY_X86_CTRL_TRAP_RETURN"]
            landing = subop == c["POLY_X86_CTRL_LANDING"]
        elif frontend == c["POLY_FRONTEND_AARCH64"]:
            effective_target = c["POLY_FRONTEND_X86"] if subop == c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"] else target_frontend
            switch = subop in {c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"], c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]}
            call = subop in {c["POLY_AARCH64_CTRL_SUBOP_CALL_MODE"], c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"]} or call_sig
            sig_required = subop == c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
            trap_return = subop == c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_AARCH64_CTRL_SUBOP_LANDING"]
        elif frontend == c["POLY_FRONTEND_RISCV"]:
            effective_target = c["POLY_FRONTEND_X86"] if subop == c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"] else target_frontend
            switch = subop in {c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"], c["POLY_RISCV_CTRL_SUBOP_SWITCH_MODE"]}
            call = subop in {c["POLY_RISCV_CTRL_SUBOP_CALL_MODE"], c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"]} or call_sig
            sig_required = subop == c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
            trap_return = subop == c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_RISCV_CTRL_SUBOP_LANDING"]

    target_checked = switch or call or landing
    invalid_subop = poly and not (switch or call or trap_return or landing)
    invalid_frontend = poly and target_checked and effective_target not in {0, 1, 2}
    noncanonical = poly and target_checked and not invalid_frontend and not canonical(target_pc)
    align_fault = (
        poly and target_checked and not invalid_frontend and canonical(target_pc) and
        not aligned(effective_target, target_pc)
    )
    invalid_sig = poly and sig_required and not signature_valid
    stack_fault = poly and call and stack_full
    handoff_error = (
        invalid_subop or invalid_frontend or noncanonical or align_fault or
        invalid_sig or stack_fault
    )
    transition = poly and not handoff_error and (switch or call)
    error = raw_align or handoff_error
    return {
        "raw": raw,
        "fetch_addr": fetch_addr,
        "fetch_bytes": fetch_bytes,
        "insn": insn,
        "poly": poly,
        "subop": subop,
        "transition": False if raw_align else transition,
        "push": False if raw_align else (poly and not handoff_error and call),
        "next_frontend": frontend if raw_align or not transition else effective_target,
        "next_pc": pc if raw_align else (target_pc if transition else fallthrough),
        "slot": slot if sig_required else 0,
        "error": error,
        "raw_align": raw_align,
        "invalid_subop": invalid_subop,
        "invalid_frontend": invalid_frontend,
        "noncanonical": noncanonical,
        "align_fault": align_fault,
        "invalid_sig": invalid_sig,
        "stack_fault": stack_fault,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    for instance in [
        "poly_raw_fetch_plan raw_fetch_plan",
        "poly_ctrl_decode ctrl_decode",
        "poly_frontend_handoff frontend_handoff",
    ]:
        assert instance in text, f"missing integrated instance: {instance}"

    x86_switch = step(
        True, c["POLY_FRONTEND_X86"], 0x1000,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert x86_switch["poly"] and x86_switch["transition"]
    assert x86_switch["next_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert x86_switch["next_pc"] == 0x4000 and not x86_switch["error"]

    aarch64_non_control = step(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000,
        0x52800000, 0,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert aarch64_non_control["raw"] and not aarch64_non_control["poly"]
    assert aarch64_non_control["fetch_bytes"] == 4
    assert aarch64_non_control["next_pc"] == 0x4004

    aarch64_switch = step(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]), 0,
        c["POLY_FRONTEND_RISCV"], 0x8000, True, False, c
    )
    assert aarch64_switch["poly"] and aarch64_switch["transition"]
    assert aarch64_switch["next_frontend"] == c["POLY_FRONTEND_RISCV"]

    riscv16 = step(
        True, c["POLY_FRONTEND_RISCV"], 0x8000,
        0xFFFF0001, 0,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert riscv16["raw"] and riscv16["fetch_bytes"] == 2
    assert riscv16["insn"] == 1 and riscv16["next_pc"] == 0x8002

    raw_align = step(
        True, c["POLY_FRONTEND_AARCH64"], 0x4002,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]), 0,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert raw_align["error"] and raw_align["raw_align"]
    assert not raw_align["poly"] and not raw_align["transition"]
    assert raw_align["next_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert raw_align["next_pc"] == 0x4002

    bad_slot = step(
        True, c["POLY_FRONTEND_RISCV"], 0x8000,
        riscv_ctrl_word(c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 4), 0,
        c["POLY_FRONTEND_X86"], 0x1000, False, False, c
    )
    assert bad_slot["poly"] and bad_slot["invalid_sig"]
    assert bad_slot["error"] and not bad_slot["transition"] and not bad_slot["push"]

    full_stack = step(
        True, c["POLY_FRONTEND_X86"], 0x1000,
        x86_ctrl_word(c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"]), 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, True, c
    )
    assert full_stack["stack_fault"] and full_stack["error"]
    assert not full_stack["transition"] and not full_stack["push"]

    print("POLY_RTL_FRONTEND_STEP_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
