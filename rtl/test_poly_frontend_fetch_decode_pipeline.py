#!/usr/bin/env python3
"""Integration checks for rtl/poly_frontend_fetch_decode_pipeline.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_fetch_decode_pipeline.sv"


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


def x86_ctrl_word(subop: int) -> int:
    return 0x0F | (0x3A << 8) | (0xFC << 16) | ((subop & 0x7F) << 24)


def aarch64_ctrl_word(subop: int) -> int:
    return 0xD503201F | ((subop & 0x7F) << 5)


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


def issue(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, int | bool]:
    x86 = frontend == c["POLY_FRONTEND_X86"]
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    x86_last = (pc + 15) & 0xFFFFFFFFFFFFFFFF
    raw_last = (pc + 3) & 0xFFFFFFFFFFFFFFFF
    x86_error = valid and x86 and (
        not canonical(pc) or not canonical(x86_last) or x86_last < pc
    )
    raw_error = valid and raw and (
        not canonical(pc) or not canonical(raw_last) or raw_last < pc or
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    return {
        "x86_req": valid and x86 and not x86_error,
        "raw_req": valid and raw and not raw_error,
        "invalid": valid and not x86 and not raw,
        "x86_error": x86_error,
        "raw_error": raw_error,
    }


def raw_plan(frontend: int, pc: int, word: int, c: dict[str, int]) -> dict[str, int | bool]:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return {"insn": word & 0xFFFFFFFF, "bytes": 4, "next": pc + 4}
    is_32 = (word & 3) == 3
    size = 4 if is_32 else 2
    return {
        "insn": word & 0xFFFFFFFF if is_32 else word & 0xFFFF,
        "bytes": size,
        "next": pc + size,
    }


def pipeline(
    valid: bool,
    frontend: int,
    pc: int,
    x86_resp_valid: bool,
    x86_resp_fault: bool,
    x86_word: int,
    x86_fallthrough: int,
    raw_resp_valid: bool,
    raw_resp_fault: bool,
    raw_word: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    x86 = frontend == c["POLY_FRONTEND_X86"]
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    req = issue(valid, frontend, pc, c)
    x86_wait = req["x86_req"] and not x86_resp_valid
    raw_wait = req["raw_req"] and not raw_resp_valid
    x86_mem_fault = req["x86_req"] and x86_resp_valid and x86_resp_fault
    raw_mem_fault = req["raw_req"] and raw_resp_valid and raw_resp_fault
    x86_valid = req["x86_req"] and x86_resp_valid and not x86_resp_fault
    raw_valid = req["raw_req"] and raw_resp_valid and not raw_resp_fault
    raw = bool(raw)
    raw_insn = raw_plan(frontend, pc, raw_word, c)["insn"] if raw_valid else 0
    fetch_valid = (x86 and x86_valid) or (raw and raw_valid)
    fetch_word = raw_insn if raw else x86_word
    poly, subop, call_sig, slot = decode(fetch_valid, frontend, int(fetch_word), c)
    return {
        "x86_req": req["x86_req"],
        "raw_req": req["raw_req"],
        "wait": x86_wait or raw_wait,
        "fetch_valid": fetch_valid,
        "fetch_fault": (
            req["invalid"] or req["x86_error"] or req["raw_error"] or
            (x86 and x86_mem_fault) or (raw and raw_mem_fault)
        ),
        "fetch_word": fetch_word if fetch_valid else 0,
        "decode_valid": fetch_valid,
        "poly": poly,
        "subop": subop,
        "call_sig": call_sig,
        "slot": slot,
        "invalid": req["invalid"],
        "x86_mem_fault": x86_mem_fault,
        "raw_mem_fault": raw_mem_fault,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_frontend_fetch_decode_pipeline",
        "poly_frontend_fetch_issue fetch_issue",
        "poly_x86_fetch_stage x86_fetch_stage",
        "poly_raw_fetch_response_stage raw_fetch_response_stage",
        "poly_frontend_decode_dispatch decode_dispatch",
        "wait_fetch_o = x86_fetch_wait_o || raw_fetch_wait_o",
        "fetch_valid_o =",
        "fetch_fault_o =",
    ]:
        if needle not in text:
            raise AssertionError(f"missing fetch-decode-pipeline wiring: {needle}")


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    require_structural_wiring()

    x86 = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000,
        True, False, x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        False, False, 0, c
    )
    assert x86["x86_req"] and not x86["raw_req"]
    assert x86["fetch_valid"] and x86["decode_valid"]
    assert x86["poly"] and x86["subop"] == c["POLY_X86_CTRL_PSWITCH_MODE"]

    x86_wait = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000,
        False, False, x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        False, False, 0, c
    )
    assert x86_wait["wait"] and not x86_wait["fetch_valid"]
    assert not x86_wait["fetch_fault"]

    a64 = pipeline(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000,
        False, False, 0, 0,
        True, False,
        aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE"] + 2),
        c,
    )
    assert a64["raw_req"] and not a64["x86_req"]
    assert a64["fetch_valid"] and a64["poly"] and a64["call_sig"]
    assert a64["slot"] == 2

    raw_fault = pipeline(
        True, c["POLY_FRONTEND_RISCV"], 0x8000,
        False, False, 0, 0,
        True, True, 0, c
    )
    assert raw_fault["raw_mem_fault"]
    assert raw_fault["fetch_fault"] and not raw_fault["fetch_valid"]

    bad_frontend = pipeline(
        True, 3, 0x1000,
        False, False, 0, 0,
        False, False, 0, c
    )
    assert bad_frontend["invalid"] and bad_frontend["fetch_fault"]

    print("POLY_RTL_FRONTEND_FETCH_DECODE_PIPELINE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
