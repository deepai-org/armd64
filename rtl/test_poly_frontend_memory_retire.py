#!/usr/bin/env python3
"""Fetch-to-retire integration checks for rtl/poly_frontend_memory_retire.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_frontend_memory_retire.sv"


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


def decode(
    valid: bool,
    frontend: int,
    insn: int,
    c: dict[str, int],
) -> tuple[bool, int, bool, int]:
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
            True,
            subop,
            call_sig,
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
            True,
            subop,
            call_sig,
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
            True,
            subop,
            call_sig,
            subop - c["POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE"]
            if call_sig else 0,
        )
    return (False, 0, False, 0)


def raw_request(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, bool | int]:
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    last = (pc + 3) & 0xFFFFFFFFFFFFFFFF
    noncanonical = valid and raw and (not canonical(pc) or not canonical(last))
    align_fault = valid and raw and (
        (frontend == c["POLY_FRONTEND_AARCH64"] and (pc & 3) != 0) or
        (frontend == c["POLY_FRONTEND_RISCV"] and (pc & 1) != 0)
    )
    range_fault = valid and raw and last < pc
    error = noncanonical or align_fault or range_fault
    return {
        "valid": valid and raw and not error,
        "noncanonical": noncanonical,
        "align": align_fault,
        "range": range_fault,
        "error": error,
        "bytes": 4 if raw else 0,
    }


def x86_request(valid: bool, frontend: int, pc: int, c: dict[str, int]) -> dict[str, bool | int]:
    x86 = frontend == c["POLY_FRONTEND_X86"]
    last = (pc + 15) & 0xFFFFFFFFFFFFFFFF
    noncanonical = valid and x86 and (not canonical(pc) or not canonical(last))
    range_fault = valid and x86 and last < pc
    error = noncanonical or range_fault
    return {
        "valid": valid and x86 and not error,
        "noncanonical": noncanonical,
        "range": range_fault,
        "error": error,
        "bytes": 16 if x86 else 0,
    }


def raw_plan(frontend: int, pc: int, word: int, c: dict[str, int]) -> dict[str, int | bool]:
    if frontend == c["POLY_FRONTEND_AARCH64"]:
        return {"insn": word & 0xFFFFFFFF, "next_pc": pc + 4, "bytes": 4}
    is_32 = (word & 3) == 3
    size = 4 if is_32 else 2
    return {
        "insn": word & 0xFFFFFFFF if is_32 else word & 0xFFFF,
        "next_pc": pc + size,
        "bytes": size,
    }


def retire_step(
    valid: bool,
    fetch_valid: bool,
    fetch_fault: bool,
    older_fault: bool,
    execute_ready: bool,
    block_retire: bool,
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
    trap_return_restore_valid: bool = False,
) -> dict[str, int | bool]:
    wait = (
        valid and not fetch_valid and not older_fault and not fetch_fault and
        not execute_fault and not block_retire
    )
    wait_execute = (
        valid and fetch_valid and not execute_ready and not older_fault and
        not fetch_fault and not execute_fault and not block_retire
    )
    wait_retire = (
        valid and block_retire and not older_fault and not fetch_fault and
        not execute_fault
    )
    step_valid = (
        valid and fetch_valid and execute_ready and not block_retire and
        not older_fault and not fetch_fault and not execute_fault
    )
    if not step_valid:
        older = valid and older_fault
        fetch = valid and not older_fault and fetch_fault
        execute = valid and not older_fault and not fetch_fault and execute_fault
        fault = older or fetch or execute
        return {
            "wait": wait, "retire": False, "transition": False, "push": False,
            "wait_execute": wait_execute,
            "wait_retire": wait_retire,
            "commit_frontend": frontend, "commit_pc": pc, "slot": 0,
            "fault": fault, "fault_pc": pc if fault else 0,
            "fetch_fault": fetch, "execute_fault": execute,
            "control_fault": False, "poly": False, "invalid_frontend": False,
        }

    if frontend == c["POLY_FRONTEND_X86"]:
        fallthrough = x86_fallthrough
        insn = word
    elif frontend == c["POLY_FRONTEND_AARCH64"]:
        fallthrough = pc + 4
        insn = word
    else:
        is_32 = (word & 3) == 3
        fallthrough = pc + (4 if is_32 else 2)
        insn = word

    poly, subop, call_sig, slot = decode(True, frontend, insn, c)
    switch = call = trap_return = landing = sig_required = False
    effective_target = target_frontend
    if poly and frontend == c["POLY_FRONTEND_X86"]:
        switch = subop in {c["POLY_X86_CTRL_PENTER_MODE"], c["POLY_X86_CTRL_PSWITCH_MODE"]}
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
        sig_required = subop == c["POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE"] or call_sig
        trap_return = subop == c["POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN"]
        landing = subop == c["POLY_AARCH64_CTRL_SUBOP_LANDING"]
    elif poly and frontend == c["POLY_FRONTEND_RISCV"]:
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

    if trap_return and not trap_return_restore_valid:
        return {
            "wait": False, "retire": False, "transition": False,
            "wait_execute": True,
            "wait_retire": False,
            "push": False,
            "commit_frontend": frontend, "commit_pc": pc, "slot": 0,
            "fault": False, "fault_pc": 0,
            "fetch_fault": False, "execute_fault": False,
            "control_fault": False, "poly": False,
            "invalid_frontend": False,
            "trap_return_decode": True,
            "trap_return_retire": False,
        }

    recognized = switch or call or trap_return or landing
    target_checked = switch or call or landing
    invalid_frontend = poly and target_checked and effective_target not in {
        c["POLY_FRONTEND_X86"], c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]
    }
    control_fault = (
        (poly and not recognized) or
        invalid_frontend or
        (poly and target_checked and not invalid_frontend and not canonical(target_pc)) or
        (poly and target_checked and not invalid_frontend and canonical(target_pc) and
         not aligned(effective_target, target_pc, c)) or
        (poly and sig_required and not signature_valid) or
        (poly and call and stack_full)
    )
    transition = poly and not control_fault and (switch or call)
    retire = not control_fault
    return {
        "wait": False, "retire": retire, "transition": retire and transition,
        "wait_execute": False,
        "wait_retire": False,
        "push": retire and poly and call,
        "commit_frontend": effective_target if transition and retire else frontend,
        "commit_pc": target_pc if transition and retire else fallthrough,
        "slot": slot if retire and sig_required else 0,
        "fault": control_fault, "fault_pc": pc if control_fault else 0,
        "fetch_fault": False, "execute_fault": False,
        "control_fault": control_fault, "poly": poly,
        "invalid_frontend": invalid_frontend,
        "trap_return_decode": trap_return,
        "trap_return_retire": retire and poly and trap_return,
    }


def pipeline(
    valid: bool,
    frontend: int,
    pc: int,
    x86_valid: bool,
    x86_fault: bool,
    x86_word: int,
    x86_fallthrough: int,
    raw_resp_valid: bool,
    raw_resp_fault: bool,
    raw_word: int,
    older_fault: bool,
    execute_ready: bool,
    block_retire: bool,
    execute_fault: bool,
    target_frontend: int,
    target_pc: int,
    signature_valid: bool,
    stack_full: bool,
    c: dict[str, int],
    trap_return_restore_valid: bool = False,
) -> dict[str, int | bool]:
    x86 = frontend == c["POLY_FRONTEND_X86"]
    raw = frontend in {c["POLY_FRONTEND_AARCH64"], c["POLY_FRONTEND_RISCV"]}
    frontend_valid = x86 or raw
    x86_req = x86_request(valid, frontend, pc, c)
    req = raw_request(valid and raw, frontend, pc, c)
    raw_fault = req["error"] or (req["valid"] and raw_resp_valid and raw_resp_fault)
    raw_valid = req["valid"] and raw_resp_valid and not raw_resp_fault
    raw_insn = raw_plan(frontend, pc, raw_word, c)["insn"] if raw_valid else 0
    x86_wait = x86_req["valid"] and not x86_valid
    x86_mem_fault = x86_req["valid"] and x86_valid and x86_fault
    x86_insn_valid = x86_req["valid"] and x86_valid and not x86_fault
    fetch_valid = (x86 and x86_insn_valid) or (raw and raw_valid)
    pipeline_invalid = valid and not frontend_valid
    fetch_fault = (
        pipeline_invalid or
        (x86 and (x86_req["error"] or x86_mem_fault)) or
        (raw and raw_fault)
    )
    fetch_word = raw_insn if raw else x86_word
    retired = retire_step(
        valid, fetch_valid, fetch_fault, older_fault, execute_ready,
        block_retire, execute_fault, frontend,
        pc, fetch_word, x86_fallthrough, target_frontend, target_pc,
        signature_valid, stack_full, c,
        trap_return_restore_valid=trap_return_restore_valid
    )
    return {
        **retired,
        "x86_req": x86_req["valid"],
        "x86_addr": pc,
        "x86_bytes": x86_req["bytes"],
        "x86_request_error": x86_req["error"],
        "x86_wait": x86_wait,
        "x86_mem_fault": x86_mem_fault,
        "x86_noncanonical": x86_req["noncanonical"],
        "x86_range": x86_req["range"],
        "raw_req": req["valid"],
        "raw_addr": pc,
        "raw_bytes": req["bytes"],
        "raw_wait": req["valid"] and not raw_resp_valid,
        "raw_request_error": req["error"],
        "raw_mem_fault": req["valid"] and raw_resp_valid and raw_resp_fault,
        "invalid_frontend": pipeline_invalid or retired["invalid_frontend"],
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    assert "poly_frontend_fetch_decode_pipeline fetch_decode_pipeline" in text
    assert "output logic [3:0]  raw_memory_access_bytes_o" in text
    assert ".raw_memory_access_bytes_o(decode_pipeline_raw_memory_access_bytes)" in text
    assert "raw_memory_access_bytes_o =" in text
    assert "poly_frontend_predecoded_retire frontend_predecoded_retire" in text
    assert ".x86_fetch_req_valid_o(x86_fetch_req_valid_o)" in text
    assert ".raw_mem_req_valid_o(raw_mem_req_valid_o)" in text
    assert ".fetch_valid_o(fetch_pipeline_valid)" in text
    assert ".fetch_fault_o(fetch_pipeline_fault)" in text
    assert ".decode_valid_o(decode_pipeline_valid)" in text
    assert ".poly_ctrl_o(decode_pipeline_poly_ctrl)" in text
    assert ".subop_o(decode_pipeline_subop)" in text
    assert ".call_sig_imm_o(decode_pipeline_call_sig_imm)" in text
    assert ".signature_slot_o(decode_pipeline_signature_slot)" in text
    assert ".decode_valid_i(decode_pipeline_valid)" in text
    assert ".poly_ctrl_i(decode_pipeline_poly_ctrl)" in text
    assert ".x86_fetch_wait_o(x86_fetch_wait_o)" in text
    assert ".raw_fetch_wait_o(raw_fetch_wait_o)" in text
    assert ".execute_ready_i(execute_ready_i)" in text
    assert ".block_retire_i(block_retire_i)" in text
    assert ".trap_return_restore_valid_i(trap_return_restore_valid_i)" in text
    assert ".trap_return_decode_o(trap_return_decode_o)" in text
    assert ".trap_return_retire_o(trap_return_retire_o)" in text
    assert ".wait_execute_o(wait_execute_o)" in text
    assert ".wait_retire_o(wait_retire_o)" in text

    x86_switch = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000, True, False,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert x86_switch["x86_req"] and x86_switch["x86_bytes"] == 16
    assert not x86_switch["raw_req"]
    assert x86_switch["retire"] and x86_switch["transition"]
    assert x86_switch["commit_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert x86_switch["commit_pc"] == 0x4000

    x86_wait = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000, False, False,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert x86_wait["x86_req"] and x86_wait["x86_wait"]
    assert x86_wait["wait"] and not x86_wait["fault"] and not x86_wait["retire"]

    raw_wait = pipeline(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, 0, 0,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert raw_wait["raw_req"] and raw_wait["raw_wait"]
    assert not raw_wait["x86_req"]
    assert raw_wait["wait"] and not raw_wait["retire"] and not raw_wait["fault"]

    raw_non_control = pipeline(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, 0, 0,
        True, False, 0x52800000, False, True, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert raw_non_control["raw_req"] and raw_non_control["retire"]
    assert not raw_non_control["transition"]
    assert raw_non_control["commit_frontend"] == c["POLY_FRONTEND_AARCH64"]
    assert raw_non_control["commit_pc"] == 0x4004

    raw_switch = pipeline(
        True, c["POLY_FRONTEND_AARCH64"], 0x4000, False, False, 0, 0,
        True, False, aarch64_ctrl_word(c["POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE"]),
        False, True, False, False,
        c["POLY_FRONTEND_RISCV"], 0x8000, True, False, c
    )
    assert raw_switch["retire"] and raw_switch["transition"]
    assert raw_switch["commit_frontend"] == c["POLY_FRONTEND_RISCV"]
    assert raw_switch["commit_pc"] == 0x8000

    x86_trap_wait = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1800, True, False,
        x86_ctrl_word(c["POLY_X86_CTRL_TRAP_RETURN"]), 0x1804,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x7000, True, False, c,
        trap_return_restore_valid=False
    )
    assert x86_trap_wait["trap_return_decode"]
    assert x86_trap_wait["wait_execute"] and not x86_trap_wait["retire"]

    x86_trap_ready = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1800, True, False,
        x86_ctrl_word(c["POLY_X86_CTRL_TRAP_RETURN"]), 0x1804,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x7000, True, False, c,
        trap_return_restore_valid=True
    )
    assert x86_trap_ready["retire"] and x86_trap_ready["trap_return_retire"]
    assert x86_trap_ready["trap_return_decode"] and not x86_trap_ready["transition"]

    raw_mem_fault = pipeline(
        True, c["POLY_FRONTEND_RISCV"], 0x8000, False, False, 0, 0,
        True, True, 0, False, True, False, False,
        c["POLY_FRONTEND_X86"], 0x1000, True, False, c
    )
    assert raw_mem_fault["raw_req"] and raw_mem_fault["raw_mem_fault"]
    assert raw_mem_fault["fault"] and raw_mem_fault["fetch_fault"]
    assert not raw_mem_fault["retire"]

    x86_fetch_fault = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000, True, True, 0, 0x1004,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert x86_fetch_fault["x86_mem_fault"]
    assert x86_fetch_fault["fault"] and x86_fetch_fault["fetch_fault"]
    assert not x86_fetch_fault["retire"] and not x86_fetch_fault["raw_req"]

    x86_bad_pc = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x0000800000000000,
        True, False, 0, 0x1004,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_AARCH64"], 0x4000, True, False, c
    )
    assert x86_bad_pc["x86_request_error"] and x86_bad_pc["x86_noncanonical"]
    assert x86_bad_pc["fault"] and x86_bad_pc["fetch_fault"]
    assert not x86_bad_pc["x86_req"] and not x86_bad_pc["retire"]

    bad_target = pipeline(
        True, c["POLY_FRONTEND_X86"], 0x1000, True, False,
        x86_ctrl_word(c["POLY_X86_CTRL_PSWITCH_MODE"]), 0x1004,
        False, False, 0, False, True, False, False,
        3, 0x4000, True, False, c
    )
    assert bad_target["fault"] and bad_target["control_fault"]
    assert bad_target["invalid_frontend"] and not bad_target["retire"]

    bad_frontend = pipeline(
        True, 3, 0x1000, True, False, 0, 0x1004,
        False, False, 0, False, True, False, False,
        c["POLY_FRONTEND_X86"], 0x2000, True, False, c
    )
    assert bad_frontend["fault"] and bad_frontend["fetch_fault"]
    assert bad_frontend["invalid_frontend"] and not bad_frontend["retire"]

    print("POLY_RTL_FRONTEND_MEMORY_RETIRE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
