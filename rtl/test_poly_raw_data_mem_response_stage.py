#!/usr/bin/env python3
"""Behavioral checks for rtl/poly_raw_data_mem_response_stage.sv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_raw_data_mem_response_stage.sv"


def response(
    valid: bool,
    request_valid: bool,
    request_error: bool,
    mem_resp_valid: bool,
    mem_resp_fault: bool,
) -> dict[str, bool]:
    request_fault = valid and request_error
    memory_fault = valid and request_valid and mem_resp_valid and mem_resp_fault
    return {
        "wait": valid and request_valid and not mem_resp_valid and not request_error,
        "resolved": (
            valid and request_valid and mem_resp_valid and
            not mem_resp_fault and not request_error
        ),
        "fault": request_fault or memory_fault,
        "request_fault": request_fault,
        "memory_fault": memory_fault,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_raw_data_mem_response_stage",
        "input  logic request_valid_i",
        "input  logic request_error_i",
        "input  logic mem_resp_valid_i",
        "input  logic mem_resp_fault_i",
        "output logic resolved_o",
        "output logic request_fault_o",
        "output logic memory_fault_o",
        "wait_response_o =",
        "resolved_o =",
        "fault_o = request_fault_o || memory_fault_o",
    ]:
        if needle not in text:
            raise AssertionError(f"missing raw data-memory response structure: {needle}")


def main() -> int:
    require_structural_wiring()

    idle = response(False, True, False, True, False)
    assert not idle["wait"] and not idle["resolved"] and not idle["fault"]

    waiting = response(True, True, False, False, False)
    assert waiting["wait"] and not waiting["resolved"] and not waiting["fault"]

    resolved = response(True, True, False, True, False)
    assert resolved["resolved"] and not resolved["wait"] and not resolved["fault"]

    mem_fault = response(True, True, False, True, True)
    assert mem_fault["fault"] and mem_fault["memory_fault"] and not mem_fault["resolved"]

    req_fault = response(True, False, True, False, False)
    assert req_fault["fault"] and req_fault["request_fault"] and not req_fault["wait"]

    req_fault_wins = response(True, True, True, True, False)
    assert req_fault_wins["fault"] and req_fault_wins["request_fault"]
    assert not req_fault_wins["resolved"] and not req_fault_wins["wait"]

    print("POLY_RTL_RAW_DATA_MEM_RESPONSE_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
