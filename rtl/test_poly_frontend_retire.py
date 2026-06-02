#!/usr/bin/env python3
"""Retirement-ordering checks for rtl/poly_frontend_retire.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_retire.sv"


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


def decode_x86_ctrl(word: int, c: dict[str, int]) -> tuple[bool, int, bool, int]:
    if (word & 0x00FFFFFF) != 0x00FC3A0F or (word >> 31):
        return (False, 0, False, 0)
    subop = (word >> 24) & 0x7F
    call_sig = c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] <= subop < (
        c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] +
        c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
    )
    return (
        True,
        subop,
        call_sig,
        subop - c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] if call_sig else 0,
    )


def decode_aarch64_ctrl(word: int, c: dict[str, int]) -> tuple[bool, int, bool, int]:
    if (word & 0xFFFFF01F) != 0xD503201F:
        return (False, 0, False, 0)
    subop = (word >> 5) & 0x7F
    call_sig = c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] <= subop < (
        c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] +
        c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
    )
    return (
        True,
        subop,
        call_sig,
        subop - c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] if call_sig else 0,
    )


def step_error_and_transition(
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
    raw_align = (
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    if frontend == c["POLY_FRONTEND_X86"]:
        poly, subop, call_sig, slot = decode_x86_ctrl(word, c)
        fallthrough = x86_fallthrough
    elif frontend == c["POLY_FRONTEND_AARCH64"]:
        poly, subop, call_sig, slot = (
            (False, 0, False, 0) if raw_align else decode_aarch64_ctrl(word, c)
        )
        fallthrough = pc + 4
    else:
        poly, subop, call_sig, slot = (False, 0, False, 0)
        fallthrough = pc + 4

    switch = call = trap_return = landing = sig_required = False
    effective_target = target_frontend
    if poly and frontend == c["POLY_FRONTEND_X86"]:
        switch = subop in {
            c["POLY_X86_CTRL_PENTER_MODE"],
            c["POLY_X86_CTRL_PSWITCH_MODE"],
        }
        call = subop == c["POLY_X86_CTRL_PCALL_SIG_MODE"] or call_sig
        sig_required = call
        trap_return = subop == c["POLY_X86_CTRL_TRAP_RETURN"]
        landing = subop == c["POLY_X86_CTRL_LANDING"]
    elif poly and frontend == c["POLY_FRONTEND_AARCH64"]:
        if subop == c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"]:
            effective_target = c["POLY_FRONTEND_X86"]
        switch = subop in {
            c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"],
            c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"],
        }
        call = subop in {
            c["POLY_AARCH64_CTRL_SUBOP_CALL_MODE"],
            c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"],
        } or call_sig
        sig_required = (
            subop == c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
        )
        trap_return = subop == c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]
        landing = subop == c["POLY_AARCH64_CTRL_SUBOP_LANDING"]

    target_checked = switch or call or landing
    invalid_frontend = poly and target_checked and effective_target not in {
        c["POLY_FRONTEND_X86"],
        c["POLY_FRONTEND_AARCH64"],
        c["POLY_FRONTEND_RISCV"],
    }
    invalid_subop = poly and not (switch or call or trap_return or landing)
    noncanonical = poly and target_checked and not invalid_frontend and not canonical(target_pc)
    target_align = (
        poly and target_checked and not invalid_frontend and canonical(target_pc) and
        not aligned(effective_target, target_pc, c)
    )
    invalid_sig = poly and sig_required and not signature_valid
    stack_fault = poly and call and stack_full
    error = (
        raw_align or invalid_frontend or invalid_subop or noncanonical or
        target_align or invalid_sig or stack_fault
    )
    transition = poly and not error and (switch or call)
    return {
        "poly": poly,
        "subop": subop,
        "slot": slot if sig_required else 0,
        "error": error,
        "raw_align": raw_align,
        "transition": transition,
        "push": poly and not error and call,
        "next_frontend": effective_target if transition else frontend,
        "next_pc": target_pc if transition else fallthrough,
        "invalid_sig": invalid_sig,
    }


def retire(
    valid: bool,
    fetch_valid: bool,
    older_fault: bool,
    fetch_fault: bool,
    execute_fault: bool,
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
    wait = (
        valid and not fetch_valid and not older_fault and not fetch_fault and
        not execute_fault
    )
    step_valid = (
        valid and fetch_valid and not older_fault and not fetch_fault and
        not execute_fault
    )
    step = step_error_and_transition(
        frontend, pc, word, x86_fallthrough, target_frontend, target_pc,
        signature_valid, stack_full, c
    ) if step_valid else {
        "poly": False,
        "subop": 0,
        "slot": 0,
        "error": False,
        "raw_align": False,
        "transition": False,
        "push": False,
        "next_frontend": frontend,
        "next_pc": pc,
        "invalid_sig": False,
    }
    older = valid and older_fault
    fetch = valid and not older_fault and fetch_fault
    execute = valid and not older_fault and not fetch_fault and execute_fault
    control = step_valid and bool(step["error"])
    fault = older or fetch or execute or control
    can_retire = step_valid and not step["error"]
    return {
        "wait": wait,
        "retire": can_retire,
        "commit_transition": can_retire and step["transition"],
        "commit_push": can_retire and step["push"],
        "commit_frontend": step["next_frontend"] if can_retire else frontend,
        "commit_pc": step["next_pc"] if can_retire else pc,
        "slot": step["slot"] if can_retire else 0,
        "fault": fault,
        "fault_pc": pc if fault else 0,
        "older": older,
        "fetch": fetch,
        "execute": execute,
        "control": control,
        "poly": step["poly"],
        "subop": step["subop"],
        "raw_align": step["raw_align"],
        "invalid_sig": step["invalid_sig"],
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    assert "poly_frontend_step frontend_step" in text
    assert "commit_transition_o = retire_o && step_transition" in text
    assert "commit_push_transition_o = retire_o && step_push_transition" in text

    pcall_word = x86_ctrl_word(c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 2)
    good = retire(
        True, True, False, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert good["retire"] and good["commit_transition"] and good["commit_push"]
    assert good["commit_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert good["commit_pc"] == 0x4000 and good["slot"] == 2

    waiting = retire(
        True, False, False, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert waiting["wait"] and not waiting["retire"] and not waiting["fault"]
    assert not waiting["commit_transition"] and not waiting["commit_push"]

    older = retire(
        True, True, True, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert older["fault"] and older["older"] and older["fault_pc"] == 0x1000
    assert not older["retire"] and not older["commit_transition"]

    fetch_fault = retire(
        True, True, False, True, False,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert fetch_fault["fault"] and fetch_fault["fetch"]
    assert not fetch_fault["retire"] and not fetch_fault["commit_push"]

    execute_fault = retire(
        True, True, False, False, True,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert execute_fault["fault"] and execute_fault["execute"]
    assert not execute_fault["retire"] and not execute_fault["commit_transition"]

    invalid_sig = retire(
        True, True, False, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, pcall_word, 0x1004,
        c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, c
    )
    assert invalid_sig["fault"] and invalid_sig["control"]
    assert invalid_sig["poly"] and invalid_sig["invalid_sig"]
    assert not invalid_sig["retire"] and not invalid_sig["commit_push"]

    raw_align = retire(
        True, True, False, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x4002,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]), 0,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert raw_align["fault"] and raw_align["control"] and raw_align["raw_align"]
    assert not raw_align["retire"] and not raw_align["commit_transition"]

    print("POLY_RTL_FRONTEND_RETIRE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
