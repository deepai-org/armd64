#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_frontend_predecoded_retire.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_predecoded_retire.sv"


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


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def aligned(frontend: int, addr: int, c: dict[str, int]) -> bool:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return (addr & 3) == 0
    if frontend == c["POLY_FRONTEND_RISCV"]:
        return (addr & 1) == 0
    return True


def retire(
    *,
    valid: bool,
    fetch_valid: bool,
    decode_valid: bool,
    execute_ready: bool,
    block_retire: bool,
    older_fault: bool,
    fetch_fault: bool,
    execute_fault: bool,
    frontend: int,
    pc: int,
    fallthrough: int,
    poly: bool,
    subop: int,
    call_sig: bool,
    slot: int,
    target_frontend: int,
    target_pc: int,
    signature_valid: bool,
    stack_full: bool,
    c: dict[str, int],
) -> dict[str, int | bool]:
    wait_fetch = (
        valid and not fetch_valid and not older_fault and not fetch_fault and
        not execute_fault and not block_retire
    )
    wait_execute = (
        valid and fetch_valid and decode_valid and not execute_ready and
        not older_fault and not fetch_fault and not execute_fault and
        not block_retire
    )
    wait_retire = (
        valid and block_retire and not older_fault and not fetch_fault and
        not execute_fault
    )
    step_valid = (
        valid and fetch_valid and decode_valid and execute_ready and
        not block_retire and not older_fault and not fetch_fault and
        not execute_fault
    )
    older = valid and older_fault
    fetch = valid and not older_fault and fetch_fault
    execute = valid and not older_fault and not fetch_fault and execute_fault

    switch = call = trap_return = landing = sig_required = False
    effective_target = target_frontend
    if step_valid and poly:
        if frontend == c["POLY_FRONTEND_X86"]:
            switch = subop in {c["POLY_X86_CTRL_PENTER_MODE"], c["POLY_X86_CTRL_PSWITCH_MODE"]}
            call = subop == c["POLY_X86_CTRL_PCALL_SIG_MODE"] or call_sig
            sig_required = call
            trap_return = subop == c["POLY_X86_CTRL_TRAP_RETURN"]
            landing = subop == c["POLY_X86_CTRL_LANDING"]
        elif frontend == c["POLY_FRONTEND_AARCH64"]:
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
            sig_required = subop == c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
            trap_return = subop == c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_AARCH64_CTRL_SUBOP_LANDING"]
        elif frontend == c["POLY_FRONTEND_RISCV"]:
            if subop == c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"]:
                effective_target = c["POLY_FRONTEND_X86"]
            switch = subop in {
                c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"],
                c["POLY_RISCV_CTRL_SUBOP_SWITCH_MODE"],
            }
            call = subop in {
                c["POLY_RISCV_CTRL_SUBOP_CALL_MODE"],
                c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"],
            } or call_sig
            sig_required = subop == c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
            trap_return = subop == c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_RISCV_CTRL_SUBOP_LANDING"]

    target_checked = switch or call or landing
    invalid_frontend = poly and target_checked and effective_target not in {
        c["POLY_FRONTEND_X86"], c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    control_fault = step_valid and (
        (poly and not (switch or call or trap_return or landing)) or
        invalid_frontend or
        (poly and target_checked and not invalid_frontend and not canonical(target_pc)) or
        (poly and target_checked and not invalid_frontend and canonical(target_pc) and
         not aligned(effective_target, target_pc, c)) or
        (poly and sig_required and not signature_valid) or
        (poly and call and stack_full)
    )
    retire_ok = step_valid and not control_fault
    transition = retire_ok and poly and (switch or call)
    fault = older or fetch or execute or control_fault
    return {
        "wait_fetch": wait_fetch,
        "wait_execute": wait_execute,
        "wait_retire": wait_retire,
        "retire": retire_ok,
        "transition": transition,
        "push": retire_ok and poly and call,
        "commit_frontend": effective_target if transition else frontend,
        "commit_pc": target_pc if transition else (fallthrough if retire_ok else pc),
        "slot": slot if retire_ok and sig_required else 0,
        "fault": fault,
        "fetch_fault": fetch,
        "execute_fault": execute,
        "control_fault": control_fault,
        "poly": step_valid and poly,
        "subop": subop if step_valid and poly else 0,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_predecoded_retire",
        "poly_frontend_handoff frontend_handoff",
        ".poly_ctrl_i(poly_ctrl_i)",
        ".subop_i(subop_i)",
        ".call_sig_imm_i(call_sig_imm_i)",
        ".signature_slot_i(signature_slot_i)",
        "poly_ctrl_o = step_valid && poly_ctrl_i",
        "raw_align_fault_o = 1'b0",
    ]:
        if needle not in text:
            raise AssertionError(f"missing predecoded-retire wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    non_poly = retire(
        valid=True, fetch_valid=True, decode_valid=True, execute_ready=True,
        block_retire=False, older_fault=False, fetch_fault=False,
        execute_fault=False, frontend=c["POLY_FRONTEND_X86"], pc=0x1000,
        fallthrough=0x1004, poly=False, subop=0, call_sig=False, slot=0,
        target_frontend=c["POLY_FRONTEND_AARCH64"], target_pc=0x4000,
        signature_valid=True, stack_full=False, c=c
    )
    assert non_poly["retire"] and not non_poly["transition"]
    assert non_poly["commit_pc"] == 0x1004

    switch = retire(
        valid=True, fetch_valid=True, decode_valid=True, execute_ready=True,
        block_retire=False, older_fault=False, fetch_fault=False,
        execute_fault=False, frontend=c["POLY_FRONTEND_X86"], pc=0x1000,
        fallthrough=0x1004, poly=True, subop=c["POLY_X86_CTRL_PSWITCH_MODE"],
        call_sig=False, slot=0, target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x4000, signature_valid=True, stack_full=False, c=c
    )
    assert switch["retire"] and switch["transition"]
    assert switch["commit_frontend"] == c["POLY_FRONTEND_AARCH64"]

    bad_sig = retire(
        valid=True, fetch_valid=True, decode_valid=True, execute_ready=True,
        block_retire=False, older_fault=False, fetch_fault=False,
        execute_fault=False, frontend=c["POLY_FRONTEND_X86"], pc=0x1000,
        fallthrough=0x1004, poly=True,
        subop=c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 3,
        call_sig=True, slot=3, target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x4000, signature_valid=False, stack_full=False, c=c
    )
    assert bad_sig["fault"] and bad_sig["control_fault"]
    assert not bad_sig["retire"]

    wait = retire(
        valid=True, fetch_valid=False, decode_valid=False, execute_ready=True,
        block_retire=False, older_fault=False, fetch_fault=False,
        execute_fault=False, frontend=c["POLY_FRONTEND_X86"], pc=0x1000,
        fallthrough=0x1004, poly=False, subop=0, call_sig=False, slot=0,
        target_frontend=c["POLY_FRONTEND_AARCH64"], target_pc=0x4000,
        signature_valid=True, stack_full=False, c=c
    )
    assert wait["wait_fetch"] and not wait["retire"] and not wait["fault"]

    fetch_fault_case = retire(
        valid=True, fetch_valid=False, decode_valid=False, execute_ready=True,
        block_retire=False, older_fault=False, fetch_fault=True,
        execute_fault=False, frontend=c["POLY_FRONTEND_X86"], pc=0x1000,
        fallthrough=0x1004, poly=False, subop=0, call_sig=False, slot=0,
        target_frontend=c["POLY_FRONTEND_AARCH64"], target_pc=0x4000,
        signature_valid=True, stack_full=False, c=c
    )
    assert fetch_fault_case["fault"] and fetch_fault_case["fetch_fault"]

    print("POLY_RTL_FRONTEND_PREDECODED_RETIRE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
