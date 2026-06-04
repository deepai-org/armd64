#!/usr/bin/env python3
"""Instruction-class checks for rtl/poly_raw_insn_decode.sv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_raw_insn_decode.sv"

POLY_FRONTEND_X86 = 0
POLY_FRONTEND_AARCH64 = 1
POLY_FRONTEND_RISCV = 2


def sext(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def decode(valid: bool, frontend: int, insn: int, pc: int = 0x1000) -> dict[str, bool | int]:
    a64 = valid and frontend == POLY_FRONTEND_AARCH64
    rv = valid and frontend == POLY_FRONTEND_RISCV

    a64_load_store = ((insn >> 25) & 0x7) == 0b100
    a64_atomic = a64_load_store and ((insn >> 24) & 0x3F) == 0b001000
    a64_barrier = (insn & 0xFFFFF01F) == 0xD503301F
    a64_bl = (insn & 0xFC000000) == 0x94000000
    a64_b = (insn & 0x7C000000) == 0x14000000
    a64_cond_b = (insn & 0xFF000010) == 0x54000000
    a64_br = (insn & 0xFFFFFC1F) == 0xD61F0000
    a64_blr = (insn & 0xFFFFFC1F) == 0xD63F0000
    a64_ret = (insn & 0xFFFFFC1F) == 0xD65F0000
    a64_trap = (insn & 0xFF000000) == 0xD4000000

    rv32 = (insn & 3) == 3
    opcode = insn & 0x7F
    rd = (insn >> 7) & 0x1F
    rv16 = insn & 0xFFFF
    rv16_funct3 = (rv16 >> 13) & 0x7
    rv16_quad = rv16 & 0x3

    rv_load = (
        (rv32 and opcode in {0x03, 0x07}) or
        (not rv32 and rv16_quad in {0b00, 0b10} and rv16_funct3 in {0b001, 0b010, 0b011})
    )
    rv_store = (
        (rv32 and opcode in {0x23, 0x27}) or
        (not rv32 and rv16_quad in {0b00, 0b10} and rv16_funct3 in {0b101, 0b110, 0b111})
    )
    rv_atomic = rv32 and opcode == 0x2F
    rv_barrier = rv32 and opcode == 0x0F
    rv_branch = (
        (rv32 and opcode in {0x63, 0x67, 0x6F}) or
        (not rv32 and (
            (rv16_quad == 0b01 and rv16_funct3 in {0b001, 0b101, 0b110, 0b111}) or
            (rv16_quad == 0b10 and ((rv16 >> 12) & 0xF) == 0b1000 and ((rv16 >> 2) & 0x1F) == 0)
        ))
    )
    rv_call = (
        (rv32 and (
            (opcode == 0x6F and rd in {1, 5}) or
            (opcode == 0x67 and rd in {1, 5})
        )) or
        (not rv32 and rv16_quad == 0b10 and ((rv16 >> 12) & 0xF) == 0b1001 and
         ((rv16 >> 7) & 0x1F) != 0 and ((rv16 >> 2) & 0x1F) == 0)
    )
    rv_return = rv32 and insn == 0x00008067
    rv_trap = (rv32 and opcode == 0x73) or (not rv32 and rv16 == 0x9002)
    rv32_direct = rv32 and opcode == 0x6F
    rv16_direct = (
        not rv32 and rv16_quad == 0b01 and
        rv16_funct3 in {0b001, 0b101}
    )

    a64_b_target = pc + (sext(insn & 0x03FF_FFFF, 26) << 2)
    rv_jal_imm = (
        ((insn >> 31) & 0x1) << 20 |
        ((insn >> 12) & 0xFF) << 12 |
        ((insn >> 20) & 0x1) << 11 |
        ((insn >> 21) & 0x3FF) << 1
    )
    rv16_j_imm = (
        ((rv16 >> 12) & 0x1) << 11 |
        ((rv16 >> 11) & 0x1) << 4 |
        ((rv16 >> 9) & 0x3) << 8 |
        ((rv16 >> 8) & 0x1) << 10 |
        ((rv16 >> 7) & 0x1) << 6 |
        ((rv16 >> 6) & 0x1) << 7 |
        ((rv16 >> 3) & 0x7) << 1 |
        ((rv16 >> 2) & 0x1) << 5
    )

    load = (a64 and a64_load_store and not a64_atomic and bool((insn >> 22) & 1)) or (rv and rv_load)
    store = (a64 and a64_load_store and not a64_atomic and not bool((insn >> 22) & 1)) or (rv and rv_store)
    atomic = (a64 and a64_atomic) or (rv and rv_atomic)
    barrier = (a64 and a64_barrier) or (rv and rv_barrier)
    a64_bytes = 1 << ((insn >> 30) & 0x3)
    if rv32:
        rv_bytes = {
            0b000: 1,
            0b001: 2,
            0b010: 4,
            0b011: 8,
            0b100: 1,
            0b101: 2,
            0b110: 4,
        }.get((insn >> 12) & 0x7, 0)
    else:
        rv_bytes = {
            0b001: 8,
            0b010: 4,
            0b011: 8,
            0b101: 8,
            0b110: 4,
            0b111: 8,
        }.get(rv16_funct3, 0)
    access_bytes = 0
    if load or store or atomic:
        access_bytes = a64_bytes if a64 else rv_bytes
    raw = a64 or rv
    target_valid = (
        (a64 and (a64_b or a64_bl)) or
        (rv and (rv32_direct or rv16_direct))
    )
    target = 0
    if target_valid:
        if a64 and (a64_b or a64_bl):
            target = a64_b_target
        elif rv and rv32 and opcode == 0x6F:
            target = pc + sext(rv_jal_imm, 21)
        else:
            target = pc + sext(rv16_j_imm, 12)

    return {
        "raw": raw,
        "memory_order": raw and (load or store or atomic or barrier),
        "load": load,
        "store": store,
        "atomic": atomic,
        "barrier": barrier,
        "bytes": access_bytes,
        "branch": (a64 and (a64_b or a64_cond_b or a64_br or a64_bl or a64_blr or a64_ret)) or (rv and rv_branch),
        "call": (a64 and (a64_bl or a64_blr)) or (rv and rv_call),
        "return": (a64 and a64_ret) or (rv and rv_return),
        "trap": (a64 and a64_trap) or (rv and rv_trap),
        "target_valid": target_valid,
        "target": target & 0xFFFF_FFFF_FFFF_FFFF,
    }


def require_structural_wiring() -> None:
    text = RTL.read_text()
    for needle in [
        "module poly_raw_insn_decode",
        "input  logic [63:0] pc_i",
        "output logic [3:0]  memory_access_bytes_o",
        "a64_load_store_group = insn_i[27:25] == 3'b100",
        "a64_memory_access_bytes = 4'd1 << insn_i[31:30]",
        "rv_opcode = insn_i[6:0]",
        "rv_memory_access_bytes = 4'd0",
        "memory_access_bytes_o = 4'd0",
        "branch_target_valid_o =",
        "branch_target_o = pc_i +",
        "rv32_uncond_direct =",
        "rv16_uncond_direct =",
        "memory_order_valid_o =",
        "call_o =",
        "return_o =",
        "trap_o =",
    ]:
        if needle not in text:
            raise AssertionError(f"missing raw decoder structure: {needle}")


def assert_class(frontend: int, insn: int, pc: int = 0x1000, **expected: bool | int) -> None:
    got = decode(True, frontend, insn, pc)
    for name, value in expected.items():
        if isinstance(value, bool):
            assert got[name] is value, (hex(insn), name, got)
        else:
            assert got[name] == value, (hex(insn), name, got)


def main() -> int:
    require_structural_wiring()

    assert_class(POLY_FRONTEND_X86, 0x0FFC3A0F, raw=False, memory_order=False)

    assert_class(POLY_FRONTEND_AARCH64, 0xF9400000, raw=True, memory_order=True, load=True, store=False, bytes=8)
    assert_class(POLY_FRONTEND_AARCH64, 0x39400000, memory_order=True, load=True, bytes=1)
    assert_class(POLY_FRONTEND_AARCH64, 0xF9000000, memory_order=True, load=False, store=True, bytes=8)
    assert_class(POLY_FRONTEND_AARCH64, 0x79000000, memory_order=True, store=True, bytes=2)
    assert_class(POLY_FRONTEND_AARCH64, 0xC85F7C00, memory_order=True, atomic=True, load=False, store=False, bytes=8)
    assert_class(POLY_FRONTEND_AARCH64, 0x08DFFC00, memory_order=True, atomic=True, bytes=1)
    assert_class(POLY_FRONTEND_AARCH64, 0xD5033FBF, memory_order=True, barrier=True, bytes=0)
    assert_class(POLY_FRONTEND_AARCH64, 0x9E710381, raw=True, memory_order=False)
    assert_class(POLY_FRONTEND_AARCH64, 0x2F03D7F7, raw=True, memory_order=False)
    assert_class(POLY_FRONTEND_AARCH64, 0x2EBF13DE, raw=True, memory_order=False)
    assert_class(POLY_FRONTEND_AARCH64, 0x94000002, pc=0x6000, branch=True, call=True, target_valid=True, target=0x6008)
    assert_class(POLY_FRONTEND_AARCH64, 0x17FFFFFE, pc=0x6000, branch=True, target_valid=True, target=0x5FF8)
    assert_class(POLY_FRONTEND_AARCH64, 0x54000040, pc=0x6000, branch=True, target_valid=False, target=0)
    assert_class(POLY_FRONTEND_AARCH64, 0xD65F03C0, branch=True, call=False, target_valid=False, **{"return": True})
    assert_class(POLY_FRONTEND_AARCH64, 0xD4200000, trap=True)

    assert_class(POLY_FRONTEND_RISCV, 0x00013083, memory_order=True, load=True, store=False, bytes=8)
    assert_class(POLY_FRONTEND_RISCV, 0x00004083, memory_order=True, load=True, bytes=1)
    assert_class(POLY_FRONTEND_RISCV, 0x00113023, memory_order=True, load=False, store=True, bytes=8)
    assert_class(POLY_FRONTEND_RISCV, 0x00112023, memory_order=True, store=True, bytes=4)
    assert_class(POLY_FRONTEND_RISCV, 0x08B5302F, memory_order=True, atomic=True, bytes=8)
    assert_class(POLY_FRONTEND_RISCV, 0x0000000F, memory_order=True, barrier=True, bytes=0)
    assert_class(POLY_FRONTEND_RISCV, 0x008000EF, pc=0xA000, branch=True, call=True, target_valid=True, target=0xA008)
    assert_class(POLY_FRONTEND_RISCV, 0xFE000CE3, pc=0xA000, branch=True, target_valid=False, target=0)
    assert_class(POLY_FRONTEND_RISCV, 0x00008067, branch=True, target_valid=False, **{"return": True})
    assert_class(POLY_FRONTEND_RISCV, 0x00000073, trap=True)
    assert_class(POLY_FRONTEND_RISCV, 0x0000E002, memory_order=True, store=True, bytes=8)
    assert_class(POLY_FRONTEND_RISCV, 0x00009002, trap=True)

    disabled = decode(False, POLY_FRONTEND_AARCH64, 0xF9400000)
    assert not disabled["raw"] and not disabled["memory_order"]

    print("POLY_RTL_RAW_INSN_DECODE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
