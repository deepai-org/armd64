#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_frontend_handoff.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_handoff.sv"


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


def canonical(addr: int) -> bool:
    high = (addr >> 48) & 0xFFFF
    sign = (addr >> 47) & 1
    return high == (0xFFFF if sign else 0)


def aligned(frontend: int, pc: int) -> bool:
    if frontend == 1:
        return (pc & 3) == 0
    if frontend == 2:
        return (pc & 1) == 0
    return True


def handoff(
    valid: bool,
    current: int,
    poly_ctrl: bool,
    subop: int,
    call_sig_imm: bool,
    signature_slot: int,
    signature_valid: bool,
    target_frontend: int,
    target_pc: int,
    fallthrough_pc: int,
    stack_full: bool,
    c: dict[str, int],
) -> dict[str, int | bool]:
    frontend_valid = current in {
        c["POLY_FRONTEND_X86"],
        c["POLY_FRONTEND_AARCH64"],
        c["POLY_FRONTEND_RISCV"],
    }
    x86_escape = (
        current == c["POLY_FRONTEND_AARCH64"] and
        subop == c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"]
    ) or (
        current == c["POLY_FRONTEND_RISCV"] and
        subop == c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"]
    )
    effective_target = c["POLY_FRONTEND_X86"] if x86_escape else target_frontend
    target_valid = effective_target in {
        c["POLY_FRONTEND_X86"],
        c["POLY_FRONTEND_AARCH64"],
        c["POLY_FRONTEND_RISCV"],
    }

    switch = call = trap_return = landing = signature_required = False
    if valid and poly_ctrl and frontend_valid:
        if current == c["POLY_FRONTEND_X86"]:
            switch = subop in {
                c["POLY_X86_CTRL_PENTER_MODE"],
                c["POLY_X86_CTRL_PSWITCH_MODE"],
            }
            call = subop == c["POLY_X86_CTRL_PCALL_SIG_MODE"] or call_sig_imm
            signature_required = call
            trap_return = subop == c["POLY_X86_CTRL_TRAP_RETURN"]
            landing = subop == c["POLY_X86_CTRL_LANDING"]
        elif current == c["POLY_FRONTEND_AARCH64"]:
            switch = subop in {
                c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"],
                c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"],
            }
            call = subop in {
                c["POLY_AARCH64_CTRL_SUBOP_CALL_MODE"],
                c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"],
            } or call_sig_imm
            signature_required = (
                subop == c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig_imm
            )
            trap_return = subop == c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_AARCH64_CTRL_SUBOP_LANDING"]
        elif current == c["POLY_FRONTEND_RISCV"]:
            switch = subop in {
                c["POLY_RISCV_CTRL_SUBOP_X86_ESCAPE"],
                c["POLY_RISCV_CTRL_SUBOP_SWITCH_MODE"],
            }
            call = subop in {
                c["POLY_RISCV_CTRL_SUBOP_CALL_MODE"],
                c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"],
            } or call_sig_imm
            signature_required = (
                subop == c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig_imm
            )
            trap_return = subop == c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"]
            landing = subop == c["POLY_RISCV_CTRL_SUBOP_LANDING"]

    op_recognized = switch or call or trap_return or landing
    target_checked = switch or call or landing
    invalid_frontend = valid and poly_ctrl and (
        not frontend_valid or (target_checked and not target_valid)
    )
    invalid_subop = valid and poly_ctrl and frontend_valid and not op_recognized
    noncanonical = (
        valid and poly_ctrl and target_checked and not invalid_frontend and
        not canonical(target_pc)
    )
    align_fault = (
        valid and poly_ctrl and target_checked and not invalid_frontend and
        canonical(target_pc) and not aligned(effective_target, target_pc)
    )
    invalid_sig = valid and poly_ctrl and signature_required and not signature_valid
    stack_fault = valid and poly_ctrl and call and stack_full
    error = (
        invalid_frontend or invalid_subop or noncanonical or align_fault or
        invalid_sig or stack_fault
    )
    transition = valid and poly_ctrl and not error and (switch or call)
    return {
        "transition": transition,
        "call": call,
        "switch": switch,
        "trap_return": trap_return,
        "landing": landing,
        "push": valid and poly_ctrl and not error and call,
        "next_frontend": effective_target if transition else current,
        "next_pc": target_pc if transition else fallthrough_pc,
        "slot": signature_slot if signature_required else 0,
        "error": error,
        "invalid_frontend": invalid_frontend,
        "invalid_subop": invalid_subop,
        "noncanonical": noncanonical,
        "align_fault": align_fault,
        "invalid_sig": invalid_sig,
        "stack_fault": stack_fault,
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)
    for name in [
        "POLY_X86_CTRL_PENTER_MODE",
        "POLY_X86_CTRL_PSWITCH_MODE",
        "POLY_X86_CTRL_PCALL_SIG_MODE",
        "POLY_X86_CTRL_TRAP_RETURN",
        "POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE",
        "POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE",
        "POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE",
        "POLY_RISCV_CTRL_SUBOP_X86_ESCAPE",
        "POLY_RISCV_CTRL_SUBOP_SWITCH_MODE",
        "POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE",
    ]:
        assert sv[name] == c[name], f"{name}: rtl={sv[name]:#x} c={c[name]:#x}"

    base = dict(
        valid=True,
        poly_ctrl=True,
        call_sig_imm=False,
        signature_slot=3,
        signature_valid=True,
        target_pc=0x4000,
        fallthrough_pc=0x1004,
        stack_full=False,
        c=c,
    )

    assert handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_PSWITCH_MODE"],
        target_frontend=c["POLY_FRONTEND_AARCH64"],
        **base,
    ) == {
        "transition": True, "call": False, "switch": True,
        "trap_return": False, "landing": False, "push": False,
        "next_frontend": c["POLY_FRONTEND_AARCH64"], "next_pc": 0x4000,
        "slot": 0, "error": False, "invalid_frontend": False,
        "invalid_subop": False, "noncanonical": False, "align_fault": False,
        "invalid_sig": False, "stack_fault": False,
    }

    call = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 2,
        call_sig_imm=True,
        signature_slot=2,
        target_frontend=c["POLY_FRONTEND_RISCV"],
        **{k: v for k, v in base.items() if k not in {"call_sig_imm", "signature_slot"}},
    )
    assert call["transition"] and call["call"] and call["push"]
    assert call["next_frontend"] == c["POLY_FRONTEND_RISCV"]
    assert call["slot"] == 2 and not call["error"]

    escaped = handoff(
        current=c["POLY_FRONTEND_AARCH64"],
        subop=c["POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE"],
        target_frontend=3,
        **base,
    )
    assert escaped["transition"] and escaped["next_frontend"] == c["POLY_FRONTEND_X86"]
    assert not escaped["invalid_frontend"] and not escaped["error"]

    bad_frontend = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_PSWITCH_MODE"],
        target_frontend=3,
        **base,
    )
    assert bad_frontend["error"] and bad_frontend["invalid_frontend"]
    assert not bad_frontend["transition"]

    bad_pc = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_PSWITCH_MODE"],
        target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x0000800000000000,
        **{k: v for k, v in base.items() if k != "target_pc"},
    )
    assert bad_pc["error"] and bad_pc["noncanonical"] and not bad_pc["transition"]

    bad_align = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_PSWITCH_MODE"],
        target_frontend=c["POLY_FRONTEND_AARCH64"],
        target_pc=0x4002,
        **{k: v for k, v in base.items() if k != "target_pc"},
    )
    assert bad_align["error"] and bad_align["align_fault"] and not bad_align["transition"]

    bad_sig = handoff(
        current=c["POLY_FRONTEND_RISCV"],
        subop=c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE"],
        signature_valid=False,
        target_frontend=c["POLY_FRONTEND_X86"],
        **{k: v for k, v in base.items() if k != "signature_valid"},
    )
    assert bad_sig["error"] and bad_sig["invalid_sig"] and not bad_sig["push"]

    full = handoff(
        current=c["POLY_FRONTEND_AARCH64"],
        subop=c["POLY_AARCH64_CTRL_SUBOP_CALL_MODE"],
        target_frontend=c["POLY_FRONTEND_RISCV"],
        stack_full=True,
        **{k: v for k, v in base.items() if k != "stack_full"},
    )
    assert full["error"] and full["stack_fault"] and not full["push"]

    bad_subop = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=0x7F,
        target_frontend=c["POLY_FRONTEND_X86"],
        **base,
    )
    assert bad_subop["error"] and bad_subop["invalid_subop"]

    trap = handoff(
        current=c["POLY_FRONTEND_X86"],
        subop=c["POLY_X86_CTRL_TRAP_RETURN"],
        target_frontend=3,
        **base,
    )
    assert trap["trap_return"] and not trap["error"] and not trap["transition"]
    assert trap["next_pc"] == base["fallthrough_pc"]

    print("POLY_RTL_FRONTEND_HANDOFF_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
