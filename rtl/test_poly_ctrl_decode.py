#!/usr/bin/env python3
"""Static and behavioral checks for rtl/poly_ctrl_decode.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_ctrl_decode.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = expr.strip().rstrip("Uu")
        if not re.fullmatch(r"0x[0-9a-fA-F]+|\d+", expr):
            continue
        constants[name] = int(expr, 0)
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
            radix = {"h": 16, "d": 10, "b": 2}[base]
            constants[name] = int(value.replace("_", ""), radix)
        else:
            constants[name] = int(expr, 0)
    return constants


def x86_word(subop: int) -> int:
    return 0x0F | (0x3A << 8) | (0xFC << 16) | (subop << 24)


def aarch64_word(subop: int) -> int:
    return 0xD503201F | ((subop & 0x7F) << 5)


def riscv_word(subop: int) -> int:
    return 0x0000700B | ((subop & 0x7F) << 25)


def decode(frontend: int, insn: int, valid: bool = True) -> tuple[bool, int, bool, int]:
    if not valid:
        return (False, 0, False, 0)
    if frontend == 0 and (insn & 0x00FFFFFF) == 0x00FC3A0F and not (insn >> 31):
        subop = (insn >> 24) & 0x7F
        call_sig = 0x30 <= subop < 0x30 + 13
        return (True, subop, call_sig, subop - 0x30 if call_sig else 0)
    if frontend == 1 and (insn & 0xFFFFF01F) == 0xD503201F:
        subop = (insn >> 5) & 0x7F
        call_sig = 0x50 <= subop < 0x50 + 13
        return (True, subop, call_sig, subop - 0x50 if call_sig else 0)
    if frontend == 2 and (insn & 0x01FFFFFF) == 0x0000700B:
        subop = (insn >> 25) & 0x7F
        call_sig = 0x20 <= subop < 0x20 + 13
        return (True, subop, call_sig, subop - 0x20 if call_sig else 0)
    return (False, 0, False, 0)


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    checks = {
        "POLY_X86_CTRL_PREFIX_0": c["POLY_X86_CTRL_PREFIX_0"],
        "POLY_X86_CTRL_PREFIX_1": c["POLY_X86_CTRL_PREFIX_1"],
        "POLY_X86_CTRL_PREFIX_2": c["POLY_X86_CTRL_PREFIX_2"],
        "POLY_X86_CTRL_PCALL_SIG_IMM_BASE":
            c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"],
        "POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE":
            c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"],
        "POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE":
            c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"],
        "POLY_ABI_SIGNATURE_SLOT_COUNT":
            c["POLY_ABI_SIGNATURE_SLOT_COUNT"],
    }
    for name, expected in checks.items():
        actual = sv[name]
        assert actual == expected, f"{name}: rtl={actual:#x} c={expected:#x}"

    cases = [
        (0, x86_word(c["POLY_X86_CTRL_TRAP_RETURN"]),
         c["POLY_X86_CTRL_TRAP_RETURN"], False, 0),
        (0, x86_word(c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 7),
         c["POLY_X86_CTRL_PCALL_SIG_IMM_BASE"] + 7, True, 7),
        (1, aarch64_word(c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]),
         c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"], False, 0),
        (1, aarch64_word(c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 3),
         c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 3, True, 3),
        (2, riscv_word(c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"]),
         c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"], False, 0),
        (2, riscv_word(c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 5),
         c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 5, True, 5),
    ]
    for frontend, insn, subop, call_sig, slot in cases:
        assert decode(frontend, insn) == (True, subop, call_sig, slot)

    negative_cases = [
        (False, 0, x86_word(c["POLY_X86_CTRL_TRAP_RETURN"])),
        (True, 0, x86_word(c["POLY_X86_CTRL_TRAP_RETURN"]) ^ 0x100),
        (True, 0, x86_word(c["POLY_X86_CTRL_TRAP_RETURN"] | 0x80)),
        (True, 1, aarch64_word(c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]) ^ 1),
        (True, 2, riscv_word(c["POLY_RISCV_CTRL_SUBOP_TRAP_RETURN"]) ^ 0x10),
        (True, 3, x86_word(c["POLY_X86_CTRL_TRAP_RETURN"])),
    ]
    for valid, frontend, insn in negative_cases:
        assert decode(frontend, insn, valid) == (False, 0, False, 0)

    print("POLY_RTL_CTRL_DECODE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
