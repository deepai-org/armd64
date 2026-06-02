#!/usr/bin/env python3
"""Fetch/decode dispatch checks for rtl/poly_frontend_decode_dispatch.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_decode_dispatch.sv"


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


def raw_plan(valid: bool, frontend: int, pc: int, word: int, c: dict[str, int]) -> dict[str, int | bool]:
    if not valid:
        return {"raw": False, "align": False, "addr": pc, "bytes": 0, "insn": 0, "next": pc}
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return {
            "raw": True, "align": (pc & 3) != 0, "addr": pc,
            "bytes": 4, "insn": word & 0xFFFFFFFF, "next": pc + 4,
        }
    if frontend == c["POLY_FRONTEND_RISCV"]:
        is_32 = (word & 3) == 3
        size = 4 if is_32 else 2
        return {
            "raw": True, "align": (pc & 1) != 0, "addr": pc,
            "bytes": size,
            "insn": word & 0xFFFFFFFF if is_32 else word & 0xFFFF,
            "next": pc + size,
        }
    return {"raw": False, "align": False, "addr": pc, "bytes": 0, "insn": 0, "next": pc}


def decode(valid: bool, frontend: int, insn: int, c: dict[str, int]) -> tuple[bool, int, bool, int]:
    if not valid:
        return (False, 0, False, 0)
    if (
        frontend == c["POLY_FRONTEND_X86"] and
        (insn & 0x00FFFFFF) == 0x00FC3A0F and
        not (insn >> 31)
    ):
        subop = (insn >> 24) & 0x7F
        call_sig = c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] +
            c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (
            True, subop, call_sig,
            subop - c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] if call_sig else 0,
        )
    if (
        frontend == c["POLY_FRONTEND_AARCH64"] and
        (insn & 0xFFFFF01F) == 0xD503201F
    ):
        subop = (insn >> 5) & 0x7F
        call_sig = c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] +
            c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (
            True, subop, call_sig,
            subop - c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"]
            if call_sig else 0,
        )
    if (
        frontend == c["POLY_FRONTEND_RISCV"] and
        (insn & 0x01FFFFFF) == 0x0000700B
    ):
        subop = (insn >> 25) & 0x7F
        call_sig = c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] <= subop < (
            c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] +
            c["POLY_ABI_SIGNATURE_SLOT_COUNT"]
        )
        return (
            True, subop, call_sig,
            subop - c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"]
            if call_sig else 0,
        )
    return (False, 0, False, 0)


def dispatch(
    valid: bool,
    frontend: int,
    pc: int,
    word: int,
    x86_fallthrough: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    plan = raw_plan(valid, frontend, pc, word, c)
    insn = plan["insn"] if plan["raw"] else word
    decode_valid = valid and not plan["align"]
    poly, subop, call_sig, slot = decode(decode_valid, frontend, int(insn), c)
    return {
        "raw": plan["raw"],
        "addr": plan["addr"],
        "bytes": plan["bytes"],
        "insn": insn,
        "fallthrough": plan["next"] if plan["raw"] else x86_fallthrough,
        "decode_valid": decode_valid,
        "poly": poly,
        "subop": subop,
        "call_sig": call_sig,
        "slot": slot,
        "raw_align": plan["align"],
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_decode_dispatch",
        "poly_raw_fetch_plan raw_fetch_plan",
        "poly_ctrl_decode ctrl_decode",
        "decode_insn = raw_fetch_o ? raw_insn : fetch_word_i",
        "decode_valid_o = valid_i && !raw_align_fault",
        "fallthrough_pc_o = raw_fetch_o ? raw_next_pc : x86_fallthrough_pc_i",
    ]:
        if needle not in text:
            raise AssertionError(f"missing decode-dispatch wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    x86 = dispatch(
        True, c["POLY_FRONTEND_X86"], 0x1000,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004, c
    )
    assert not x86["raw"] and x86["bytes"] == 0
    assert x86["decode_valid"] and x86["poly"]
    assert x86["subop"] == c["POLY_X86_CTRL_PSWITCH_MODE"]
    assert x86["fallthrough"] == 0x1004

    a64 = dispatch(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 2),
        0, c
    )
    assert a64["raw"] and a64["bytes"] == 4
    assert a64["decode_valid"] and a64["poly"] and a64["call_sig"]
    assert a64["slot"] == 2 and a64["fallthrough"] == 0x4004

    rv16 = dispatch(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, 0xFFFF0001, 0, c
    )
    assert rv16["raw"] and rv16["bytes"] == 2
    assert rv16["insn"] == 1 and rv16["fallthrough"] == 0x8002
    assert rv16["decode_valid"] and not rv16["poly"]

    rv32 = dispatch(
        True, c["POLY_FRONTEND_RISCV"], 0x8000,
        riscv_ctrl_word(c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 3),
        0, c
    )
    assert rv32["raw"] and rv32["bytes"] == 4
    assert rv32["poly"] and rv32["call_sig"] and rv32["slot"] == 3
    assert rv32["fallthrough"] == 0x8004

    bad_align = dispatch(
        True, c["POLY_FRONTEND_AARCH64"], 0x4002,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]), 0, c
    )
    assert bad_align["raw"] and bad_align["raw_align"]
    assert not bad_align["decode_valid"] and not bad_align["poly"]

    disabled = dispatch(
        False, c["POLY_FRONTEND_X86"], 0x1000,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004, c
    )
    assert not disabled["raw"] and not disabled["decode_valid"]
    assert not disabled["poly"]

    print("POLY_RTL_FRONTEND_DECODE_DISPATCH_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
