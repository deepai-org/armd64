#!/usr/bin/env python3
"""Integration checks for rtl/poly_event_frame_stage.sv."""

from pathlib import Path

from test_poly_event_frame_encode import (
    EVENT_BYTES,
    EVENT_MAGIC,
    encode,
    parse_c_enum_constants,
)


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_event_frame_stage.sv"


def stage(
    valid: bool,
    enabled: bool,
    addr: int,
    event_kind: int,
    source_frontend: int,
    number: int,
    selector: int,
    insn_pc: int,
    resume_pc: int,
    args: list[int],
    mem_resp_valid: bool,
    mem_fault: bool,
    c: dict[str, int],
) -> dict[str, object]:
    frame = encode(
        valid, enabled, addr, event_kind, source_frontend, number, selector,
        insn_pc, resume_pc, args, c
    )
    frame_valid = bool(frame["frame_valid"])
    frame_mem_fault = frame_valid and mem_resp_valid and mem_fault
    return {
        "mem_write_valid": frame_valid,
        "mem_addr": frame["addr"],
        "mem_bytes": frame["bytes"],
        "qwords": frame["qwords"],
        "wait": frame_valid and not mem_resp_valid,
        "delivered": frame_valid and mem_resp_valid and not mem_fault,
        "fault": frame["error"] or frame_mem_fault,
        "encode_error": frame["error"],
        "frame_mem_fault": frame_mem_fault,
        "disabled": frame["disabled"],
        "noncanonical": frame["noncanonical"],
        "align": frame["align"],
        "range": frame["range"],
        "invalid_kind": frame["invalid_kind"],
        "invalid_source": frame["invalid_source"],
    }


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    text = RTL.read_text()
    assert "poly_event_frame_encode frame_encode" in text
    assert "frame_delivered_o =" in text
    assert "frame_mem_fault_o =" in text

    args = [0x100 + n for n in range(c["POLY_V2_EVENT_ARG_COUNT"])]

    idle = stage(
        False, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 0, 0x4000, 0x4004, args,
        False, False, c
    )
    assert not idle["mem_write_valid"]
    assert not idle["wait"] and not idle["fault"] and not idle["delivered"]

    waiting = stage(
        True, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_RAW_AARCH64"], 172, 0, 0x4000, 0x4004, args,
        False, False, c
    )
    assert waiting["mem_write_valid"] and waiting["wait"]
    assert not waiting["fault"] and not waiting["delivered"]

    delivered = stage(
        True, True, 0x457000, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_RISCV"], 5, 9, 0x8000, 0x8002, args,
        True, False, c
    )
    assert delivered["mem_write_valid"] and delivered["delivered"]
    assert not delivered["fault"] and not delivered["wait"]
    assert delivered["mem_bytes"] == EVENT_BYTES
    assert delivered["qwords"][0] == EVENT_MAGIC
    assert delivered["qwords"][3] == (c["POLY_TRAP_IMPORT"] << 32) | c["POLY_MODE_RAW_RISCV"]
    assert delivered["qwords"][6:9] == [0x8000, 0x8002, 0x8002]
    assert delivered["qwords"][14:23] == [9, *args]
    assert delivered["qwords"][24] == 5

    frame_fault = stage(
        True, True, 0x457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, True, c
    )
    assert frame_fault["mem_write_valid"]
    assert frame_fault["fault"] and frame_fault["frame_mem_fault"]
    assert not frame_fault["delivered"]

    disabled = stage(
        True, False, 0x457000, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, False, c
    )
    assert disabled["fault"] and disabled["encode_error"] and disabled["disabled"]
    assert not disabled["mem_write_valid"] and not disabled["delivered"]

    bad_align = stage(
        True, True, 0x457008, c["POLY_TRAP_BREAK"],
        c["POLY_MODE_RAW_RISCV"], 1, 0, 0x8000, 0x8002, args,
        True, False, c
    )
    assert bad_align["fault"] and bad_align["align"]
    assert not bad_align["mem_write_valid"]

    bad_source = stage(
        True, True, 0x457000, c["POLY_TRAP_SYSCALL"],
        c["POLY_MODE_X86"], 1, 0, 0x1000, 0x1004, args,
        True, False, c
    )
    assert bad_source["fault"] and bad_source["invalid_source"]
    assert not bad_source["mem_write_valid"]

    bad_range = stage(
        True, True, 0xFFFFFFFFFFFFFE40, c["POLY_TRAP_IMPORT"],
        c["POLY_MODE_RAW_AARCH64"], 1, 0, 0x4000, 0x4004, args,
        True, False, c
    )
    assert bad_range["fault"] and bad_range["range"]
    assert not bad_range["mem_write_valid"]

    print("POLY_RTL_EVENT_FRAME_STAGE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
