#!/usr/bin/env python3
"""Integration checks for rtl/poly_frontend_fpga_top.sv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_frontend_fpga_top.sv"
MAKEFILE = ROOT / "Makefile"


def main() -> int:
    text = RTL.read_text()
    makefile = MAKEFILE.read_text()

    required_fragments = [
        "module poly_frontend_fpga_top",
        "poly_frontend_stateful_core stateful_core",
        "instr_req_valid_o = x86_fetch_req_valid || raw_mem_req_valid",
        "instr_req_conflict_o = x86_fetch_req_valid && raw_mem_req_valid",
        "instr_req_frontend_o =",
        "instr_req_addr_o =",
        "instr_req_bytes_o =",
        "x86_resp_valid =",
        "raw_resp_valid =",
        "instr_resp_frontend_i == POLY_FRONTEND_X86",
        "instr_resp_frontend_i == POLY_FRONTEND_AARCH64",
        "instr_resp_frontend_i == POLY_FRONTEND_RISCV",
        "input  logic        raw_branch_resolved_i",
        "input  logic        raw_branch_taken_i",
        "input  logic [63:0] raw_branch_target_i",
        "input  logic [63:0] raw_data_mem_addr_i",
        "input  logic        raw_memory_resolved_i",
        "input  logic        raw_memory_fault_i",
        "output logic        raw_data_mem_valid_o",
        "output logic [3:0]  raw_data_mem_access_bytes_o",
        "output logic        raw_data_mem_req_valid_o",
        "output logic [63:0] raw_data_mem_req_addr_o",
        "output logic [3:0]  raw_data_mem_req_bytes_o",
        "poly_raw_data_mem_request raw_data_mem_request",
        ".addr_i(raw_data_mem_addr_i)",
        ".request_valid_o(raw_data_mem_req_valid_o)",
        ".request_addr_o(raw_data_mem_req_addr_o)",
        ".request_bytes_o(raw_data_mem_req_bytes_o)",
        ".x86_fetch_valid_i(x86_resp_valid)",
        ".raw_mem_resp_valid_i(raw_resp_valid)",
        ".raw_branch_resolved_i(raw_branch_resolved_i)",
        ".raw_branch_taken_i(raw_branch_taken_i)",
        ".raw_branch_target_i(raw_branch_target_i)",
        ".raw_memory_resolved_i(raw_memory_resolved_i)",
        ".raw_memory_fault_i(raw_memory_fault_i)",
        ".raw_data_mem_valid_o(raw_data_mem_valid)",
        ".raw_data_mem_access_bytes_o(raw_data_mem_access_bytes)",
        "assign raw_data_mem_valid_o = raw_data_mem_valid",
        "assign raw_data_mem_access_bytes_o = raw_data_mem_access_bytes",
        ".x86_fetch_req_valid_o(x86_fetch_req_valid)",
        ".raw_mem_req_valid_o(raw_mem_req_valid)",
    ]
    for fragment in required_fragments:
        if fragment not in text:
            raise AssertionError(f"missing fpga-top wiring: {fragment}")

    forbidden_fragments = [
        "handle_poly_syscall",
        "libcall",
        "strlen",
        "memcpy",
        "#UD",
    ]
    lowered = text.lower()
    for fragment in forbidden_fragments:
        if fragment.lower() in lowered:
            raise AssertionError(f"fpga top should not contain policy fragment: {fragment}")

    assert "POLY_RTL_TOP ?= poly_frontend_fpga_top" in makefile
    assert "rtl/poly_frontend_fpga_top.sv" in makefile
    assert "rtl/poly_raw_data_mem_request.sv" in makefile
    assert "python3 rtl/test_poly_frontend_fpga_top.py" in makefile
    assert "python3 rtl/test_poly_raw_data_mem_request.py" in makefile

    print("POLY_RTL_FRONTEND_FPGA_TOP_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
