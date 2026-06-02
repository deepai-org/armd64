#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_x86_fetch_stage.sv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_x86_fetch_stage.sv"


def stage(
    valid: bool,
    request_valid: bool,
    resp_valid: bool,
    resp_fault: bool,
    word: int,
    fallthrough: int,
) -> dict[str, int | bool]:
    insn_valid = valid and request_valid and resp_valid and not resp_fault
    mem_fault = valid and request_valid and resp_valid and resp_fault
    return {
        "wait": valid and request_valid and not resp_valid,
        "insn_valid": insn_valid,
        "insn": word if insn_valid else 0,
        "fallthrough": fallthrough if insn_valid else 0,
        "fault": mem_fault,
        "mem_fault": mem_fault,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_x86_fetch_stage",
        "wait_response_o = valid_i && request_valid_i && !fetch_resp_valid_i",
        "insn_valid_o =",
        "!fetch_resp_fault_i",
        "mem_fault_o =",
        "fault_o = mem_fault_o",
    ]:
        if needle not in text:
            raise AssertionError(f"missing x86-fetch-stage wiring: {needle}")


def main() -> int:
    require_structural_wiring()

    assert stage(False, True, False, False, 0x1234, 0x1004) == {
        "wait": False, "insn_valid": False, "insn": 0,
        "fallthrough": 0, "fault": False, "mem_fault": False,
    }
    assert stage(True, False, True, False, 0x1234, 0x1004) == {
        "wait": False, "insn_valid": False, "insn": 0,
        "fallthrough": 0, "fault": False, "mem_fault": False,
    }
    assert stage(True, True, False, False, 0x1234, 0x1004) == {
        "wait": True, "insn_valid": False, "insn": 0,
        "fallthrough": 0, "fault": False, "mem_fault": False,
    }
    assert stage(True, True, True, False, 0xFC3A0F, 0x1004) == {
        "wait": False, "insn_valid": True, "insn": 0xFC3A0F,
        "fallthrough": 0x1004, "fault": False, "mem_fault": False,
    }
    assert stage(True, True, True, True, 0xFC3A0F, 0x1004) == {
        "wait": False, "insn_valid": False, "insn": 0,
        "fallthrough": 0, "fault": True, "mem_fault": True,
    }

    print("POLY_RTL_X86_FETCH_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
