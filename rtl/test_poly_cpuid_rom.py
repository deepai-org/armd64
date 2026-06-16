#!/usr/bin/env python3
"""Static and behavioral checks for rtl/poly_cpuid_rom.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "tools/include/polycpuid.h"
RTL = ROOT / "rtl/poly_cpuid_rom.sv"


def parse_c_enum_constants(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pending: list[tuple[str, str]] = []
    for name, expr in re.findall(r"\b(POLY_[A-Z0-9_]+)\s*=\s*([^,\n]+)", text):
        expr = re.sub(r"/\*.*?\*/", "", expr.strip().replace("(uint32_t) ", ""))
        or_shift_parts = [part.strip().strip("()") for part in expr.split("|")]
        if len(or_shift_parts) > 1 and all(
            re.fullmatch(r"\d+U?(?:LL)?\s*<<\s*\d+", part)
            for part in or_shift_parts
        ):
            value = 0
            for part in or_shift_parts:
                shift = re.fullmatch(r"(\d+)U?(?:LL)?\s*<<\s*(\d+)", part)
                assert shift is not None
                value |= int(shift.group(1), 0) << int(shift.group(2), 0)
            constants[name] = value
            continue
        shift = re.fullmatch(r"\(?(\d+)U?(?:LL)?\s*<<\s*(\d+)\)?", expr)
        if shift:
            constants[name] = int(shift.group(1), 0) << int(shift.group(2), 0)
            continue
        expr = expr.rstrip("UuLl")
        if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", expr):
            constants[name] = int(expr, 0)
            continue
        pending.append((name, expr.strip()))

    changed = True
    while changed:
        changed = False
        for name, expr in pending:
            if name in constants:
                continue
            if re.fullmatch(r"POLY_[A-Z0-9_]+", expr) and expr in constants:
                constants[name] = constants[expr]
                changed = True
                continue
            or_parts = [part.strip() for part in expr.split("|")]
            if len(or_parts) > 1 and all(part in constants for part in or_parts):
                value = 0
                for part in or_parts:
                    value |= constants[part]
                constants[name] = value
                changed = True
                continue
            add = re.fullmatch(r"(POLY_[A-Z0-9_]+)\s*\+\s*(0x[0-9a-fA-F]+|\d+)", expr)
            if add and add.group(1) in constants:
                constants[name] = constants[add.group(1)] + int(add.group(2), 0)
                changed = True
    return constants


def parse_sv_localparams(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pattern = re.compile(
        r"localparam\s+logic\s+(?:\[[^\]]+\]\s+)?([A-Z0-9_]+)\s*=\s*([^;]+);"
    )
    for name, expr in pattern.findall(text):
        expr = expr.split("//", 1)[0].strip()
        sv = re.fullmatch(r"(\d+)'([hdb])([0-9a-fA-F_]+)", expr)
        if sv:
            _, base, value = sv.groups()
            constants[name] = int(value.replace("_", ""), {"h": 16, "d": 10, "b": 2}[base])
        elif re.fullmatch(r"\d+", expr):
            constants[name] = int(expr, 0)
    return constants


def mask(c: dict[str, int], names: str) -> int:
    value = 0
    for name in names.split():
        value |= c[name]
    return value & 0xFFFFFFFF


def mode_mask(c: dict[str, int], names: str) -> int:
    value = 0
    for name in names.split():
        value |= 1 << c[name]
    return value


def vendor_regs() -> tuple[int, int, int]:
    vendor = b"PolyglotCPU!"
    ebx = int.from_bytes(vendor[0:4], "little")
    edx = int.from_bytes(vendor[4:8], "little")
    ecx = int.from_bytes(vendor[8:12], "little")
    return ebx, ecx, edx


def rom_model(leaf: int, subleaf: int, c: dict[str, int]) -> tuple[bool, int, int, int, int]:
    base = c["POLY_CPUID_BASE"]
    mode_all = mode_mask(c, "POLY_MODE_X86 POLY_MODE_RAW_AARCH64 POLY_MODE_RAW_RISCV")
    mode_raw = mode_mask(c, "POLY_MODE_RAW_AARCH64 POLY_MODE_RAW_RISCV")
    frontend_all = mode_mask(c, "POLY_FRONTEND_X86 POLY_FRONTEND_AARCH64 POLY_FRONTEND_RISCV")
    feature = mask(c, """
        POLY_CPUID_FEATURE_RAW_AARCH64
        POLY_CPUID_FEATURE_RAW_RISCV
        POLY_CPUID_FEATURE_NEUTRAL_SWITCH
        POLY_CPUID_FEATURE_NATIVE_RET
        POLY_CPUID_FEATURE_PCALL_SYSV
        POLY_CPUID_FEATURE_PCALL_SRET
        POLY_CPUID_FEATURE_FP_BRIDGE
        POLY_CPUID_FEATURE_TRAP_RECORDS
        POLY_CPUID_FEATURE_USER_RETURN_RESTORE
        POLY_CPUID_FEATURE_X86_TSO
        POLY_CPUID_FEATURE_PER_THREAD_STATE
        POLY_CPUID_FEATURE_GENERIC_FRONTEND_IDS
        POLY_CPUID_FEATURE_X86_POLY_OPCODES
        POLY_CPUID_FEATURE_FPAIR32_RET
        POLY_CPUID_FEATURE_FPAIR32_ARG
        POLY_CPUID_FEATURE_HETERO_U64_F64
        POLY_CPUID_FEATURE_HETERO_F64_U64
        POLY_CPUID_FEATURE_HETERO_U64_F32
        POLY_CPUID_FEATURE_HETERO_F32_U64
        POLY_CPUID_FEATURE_COMPACT_U32_F32
        POLY_CPUID_FEATURE_COMPACT_F32_U32
        POLY_CPUID_FEATURE_AARCH64_HFA32_ARGS
        POLY_CPUID_FEATURE_TRAP_VECTOR
        POLY_CPUID_FEATURE_STATE_KEY
        POLY_CPUID_FEATURE_VEC128_BRIDGE
        POLY_CPUID_FEATURE_AARCH64_HFA64_RET
        POLY_CPUID_FEATURE_AARCH64_HFA32_RET
        POLY_CPUID_FEATURE_FOREIGN_PCALL_SIG_IMM
    """)
    state = mask(c, """
        POLY_CPUID_STATE_OVERLAP_GPRS
        POLY_CPUID_STATE_USER_RETURN_RESTORE
        POLY_CPUID_STATE_X86_TSO
        POLY_CPUID_STATE_KEY_EXPLICIT
        POLY_CPUID_STATE_TRANSITION_FRAME_32
        POLY_CPUID_STATE_EXPLICIT_SAVE_RESTORE
        POLY_CPUID_STATE_XSAVE_ARCH_CONTRACT
        POLY_CPUID_STATE_IMPORT_RETURN_XSAVE
        POLY_CPUID_STATE_ABI_SIGNATURE_XSAVE
        POLY_CPUID_STATE_CROSS_RETURN_XSAVE
        POLY_CPUID_STATE_FRONTEND_TLS_XSAVE
        POLY_CPUID_STATE_LANDING_POLICY_XSAVE
        POLY_CPUID_STATE_STATE_KEY_XSAVE
        POLY_CPUID_STATE_TRAP_RESTORE_XSAVE
        POLY_CPUID_STATE_NATIVE_RETURN_XSAVE
        POLY_CPUID_STATE_USER_SPILL
        POLY_CPUID_STATE_MONITOR_TRAMPOLINE
        POLY_CPUID_STATE_OS_XSAVE_NOT_REQUIRED
    """)
    xsave_flags = mask(c, """
        POLY_STATE_XSAVE_FLAG_INTERRUPT_RESUME
        POLY_STATE_XSAVE_FLAG_TRAP_STATE
        POLY_STATE_XSAVE_FLAG_COMPLETE_BANK_EXPORT
        POLY_STATE_XSAVE_FLAG_IMPORT_RETURN
        POLY_STATE_XSAVE_FLAG_ABI_SIGNATURES
        POLY_STATE_XSAVE_FLAG_CROSS_RETURN
        POLY_STATE_XSAVE_FLAG_FRONTEND_TLS
        POLY_STATE_XSAVE_FLAG_LANDING_POLICY
        POLY_STATE_XSAVE_FLAG_STATE_KEY
        POLY_STATE_XSAVE_FLAG_TRAP_RESTORE
        POLY_STATE_XSAVE_FLAG_NATIVE_RETURN
        POLY_STATE_XSAVE_FLAG_USER_SPILL
        POLY_STATE_XSAVE_FLAG_MONITOR_TRAMPOLINE
        POLY_STATE_XSAVE_FLAG_OS_XSAVE_NOT_REQUIRED
    """)
    trap_flags = mask(c, """
        POLY_EVENT_RECORD_FLAG_VECTOR_DELIVERY
        POLY_EVENT_RECORD_FLAG_NO_VECTOR_X86_EXCEPTIONS
        POLY_EVENT_RECORD_FLAG_TRAP_RETURN_RESTORE
        POLY_EVENT_RECORD_FLAG_ALL_FRONTEND_HANDLERS
        POLY_EVENT_RECORD_FLAG_OPAQUE_SYSCALLS
        POLY_EVENT_RECORD_FLAG_OPAQUE_IMPORTS
    """)
    interrupt_flags = mask(c, """
        POLY_INTERRUPT_FLAG_RAW_CPL3_ONLY
        POLY_INTERRUPT_FLAG_STANDARD_X86_ENTRY
        POLY_INTERRUPT_FLAG_STATE_COMPONENT_SAVE
        POLY_INTERRUPT_FLAG_PRECISE_FOREIGN_PC
        POLY_INTERRUPT_FLAG_EVENT_CHECK_BETWEEN_INSNS
    """)
    interrupt_returns = mask(c, """
        POLY_INTERRUPT_RETURN_IRET64
        POLY_INTERRUPT_RETURN_SYSRET
        POLY_INTERRUPT_RETURN_SYSEXIT
        POLY_INTERRUPT_RETURN_SIGNAL
    """)
    memory_flags = mask(c, """
        POLY_MEMORY_FLAG_SHARED_X86_MEMORY
        POLY_MEMORY_FLAG_AARCH64_BARRIERS_NOOP
        POLY_MEMORY_FLAG_RISCV_FENCES_NOOP
        POLY_MEMORY_FLAG_ATOMICS_COHERENT
        POLY_MEMORY_FLAG_NO_WEAK_REORDERING
    """)
    transition_flags = mask(c, """
        POLY_TRANSITION_FLAG_DECODED_X86_OPCODES
        POLY_TRANSITION_FLAG_NATIVE_RAW_ESCAPES
        POLY_TRANSITION_FLAG_PIPELINE_FLUSH
        POLY_TRANSITION_FLAG_BLOCK_BOUNDARY
        POLY_TRANSITION_FLAG_PRECISE_NEXT_PC
        POLY_TRANSITION_FLAG_NATIVE_FRONTEND_WIDTHS
        POLY_TRANSITION_FLAG_NEUTRAL_FOREIGN
        POLY_TRANSITION_FLAG_NATIVE_RETURN_COOKIE
        POLY_TRANSITION_FLAG_TRAP_RETURN
        POLY_TRANSITION_FLAG_INTERRUPTED_RAW
        POLY_TRANSITION_FLAG_LANDING_PADS
        POLY_TRANSITION_FLAG_LANDING_POLICY
    """)
    abi_flags = mask(c, """
        POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_AAPCS64
        POLY_ABI_BRIDGE_FLAG_X86_SYSV_TO_RISCV
        POLY_ABI_BRIDGE_FLAG_SRET
        POLY_ABI_BRIDGE_FLAG_SCALAR_FP
        POLY_ABI_BRIDGE_FLAG_REGISTER_ONLY_AGGREGATES
        POLY_ABI_BRIDGE_FLAG_TLS_BASE
        POLY_ABI_BRIDGE_FLAG_NO_CPU_HELPER_FALLBACK
        POLY_ABI_BRIDGE_FLAG_ORDINARY_X86_RET
        POLY_ABI_BRIDGE_FLAG_VEC128
        POLY_ABI_BRIDGE_FLAG_REGISTER_SIGNATURES
        POLY_ABI_BRIDGE_FLAG_NATIVE_I128_SIGNATURES
        POLY_ABI_BRIDGE_FLAG_REGISTER_MAP_SIGNATURES
    """)
    v2_features = mask(c, """
        POLY_CPUID_V2_FEATURE_EVENT_FRAME
        POLY_CPUID_V2_FEATURE_SPILL_DESCRIPTOR
        POLY_CPUID_V2_FEATURE_DEBUG_NOTE_EXPORT
        POLY_CPUID_V2_FEATURE_MEMORY_PROBE
        POLY_CPUID_V2_FEATURE_STATE_DERIVE
        POLY_CPUID_V2_FEATURE_SHARED_MEMORY_FENCE
        POLY_CPUID_V2_FEATURE_POLICY_PREFLIGHT
        POLY_CPUID_V2_FEATURE_ABI_DESCRIPTORS
        POLY_CPUID_V2_FEATURE_DIAGNOSTIC_COUNTERS
    """)

    if leaf == base:
        ebx, ecx, edx = vendor_regs()
        return (True, c["POLY_CPUID_MAX"], ebx, ecx, edx)
    if leaf == base + 1:
        return (True, 1, mode_all, feature, c["POLY_STATE_XSAVE_COMPONENT_ARCH"])
    if leaf == base + 2:
        x86_opcode_geometry = (
            c["POLY_X86_CTRL_PREFIX_0"] |
            (c["POLY_X86_CTRL_PREFIX_1"] << 8) |
            (c["POLY_X86_CTRL_PREFIX_2"] << 16)
        )
        x86_opcode_flags = mask(c, """
            POLY_X86_OPCODE_FLAG_CPUID_DISCOVERED
            POLY_X86_OPCODE_FLAG_DEDICATED_DECODE
            POLY_X86_OPCODE_FLAG_FIXED_LENGTH
            POLY_X86_OPCODE_FLAG_NOT_UD_TRAP
            POLY_X86_OPCODE_FLAG_VENDOR_PROTOTYPE
            POLY_X86_OPCODE_FLAG_PRODUCTION_REASSIGNABLE
        """)
        escapes = {
            31: (
                c["POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS"],
                c["POLY_X86_CTRL_FOREIGN_IMPORT_COUNT_STATUS"],
                c["POLY_X86_CTRL_EVENT_PTR_SET"],
                c["POLY_X86_CTRL_SPILL_DESC_SET"],
            ),
            32: (
                x86_opcode_geometry,
                c["POLY_X86_CTRL_PREFIX_BYTES"],
                c["POLY_X86_CTRL_TOTAL_BYTES"],
                c["POLY_X86_CTRL_SUBOP_OFFSET"],
            ),
            33: (
                c["POLY_X86_OPCODE_CONTRACT_VERSION"],
                x86_opcode_flags,
                c["POLY_X86_OPCODE_FAMILY_VENDOR_PROTOTYPE"],
                0,
            ),
            34: (
                c["POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS"],
                c["POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS"],
                c["POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS"],
                0,
            ),
        }
        return (True, *escapes[subleaf]) if subleaf in escapes else (False, 0, 0, 0, 0)
    if leaf == base + 3:
        return (
            True, state, 0, c["POLY_STATE_XSAVE_COMPONENT_ARCH"],
            c["POLY_STATE_XSAVE_BYTES_ARCH"]
        )
    if leaf == base + 4:
        layout_ecx = c["POLY_STATE_XSAVE_LAYOUT_VERSION"] | (
            c["POLY_STATE_XSAVE_ALIGN_ARCH"] << 16
        )
        arch = {
            0: (
                c["POLY_STATE_XSAVE_COMPONENT_ARCH"],
                c["POLY_STATE_XSAVE_BYTES_ARCH"],
                layout_ecx,
                xsave_flags,
            ),
            1: (
                c["POLY_STATE_XSAVE_HEADER_OFFSET"],
                c["POLY_STATE_XSAVE_HEADER_BYTES"],
                c["POLY_STATE_XSAVE_MAGIC"],
                c["POLY_STATE_XSAVE_LAYOUT_VERSION"],
            ),
            2: (
                c["POLY_STATE_XSAVE_EVENT_RECORD_OFFSET"],
                c["POLY_STATE_XSAVE_EVENT_RECORD_BYTES"],
                c["POLY_STATE_XSAVE_EVENT_ARGS_OFFSET"],
                c["POLY_STATE_XSAVE_EVENT_ARGS_BYTES"],
            ),
            3: (
                c["POLY_STATE_XSAVE_AARCH64_GPR_OFFSET"],
                c["POLY_STATE_XSAVE_AARCH64_GPR_BYTES"],
                32,
                8,
            ),
            4: (
                c["POLY_STATE_XSAVE_AARCH64_FP_OFFSET"],
                c["POLY_STATE_XSAVE_AARCH64_FP_BYTES"],
                32,
                16,
            ),
            5: (
                c["POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET"],
                c["POLY_STATE_XSAVE_AARCH64_STATUS_BYTES"],
                c["POLY_STATE_XSAVE_AARCH64_STATUS_BYTES"],
                0,
            ),
            6: (
                c["POLY_STATE_XSAVE_RISCV_GPR_OFFSET"],
                c["POLY_STATE_XSAVE_RISCV_GPR_BYTES"],
                32,
                8,
            ),
            7: (
                c["POLY_STATE_XSAVE_RISCV_FP_OFFSET"],
                c["POLY_STATE_XSAVE_RISCV_FP_BYTES"],
                32,
                16,
            ),
            8: (
                c["POLY_STATE_XSAVE_RISCV_STATUS_OFFSET"],
                c["POLY_STATE_XSAVE_RISCV_STATUS_BYTES"],
                c["POLY_STATE_XSAVE_RISCV_STATUS_BYTES"],
                0,
            ),
            9: (
                c["POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET"],
                c["POLY_STATE_XSAVE_ABI_SIGNATURE_BYTES"],
                c["POLY_ABI_SIGNATURE_SLOT_COUNT"],
                8,
            ),
            10: (
                c["POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET"],
                c["POLY_STATE_XSAVE_FRONTEND_TLS_BYTES"],
                c["POLY_STATE_XSAVE_FRONTEND_TLS_BYTES"],
                0,
            ),
            11: (
                c["POLY_STATE_XSAVE_LANDING_POLICY_OFFSET"],
                c["POLY_STATE_XSAVE_LANDING_POLICY_BYTES"],
                c["POLY_LANDING_POLICY_REQUIRE_SWITCH"] |
                c["POLY_LANDING_POLICY_REQUIRE_CALL"],
                0,
            ),
            12: (
                c["POLY_STATE_XSAVE_STATE_KEY_OFFSET"],
                c["POLY_STATE_XSAVE_STATE_KEY_BYTES"],
                c["POLY_STATE_KEY_FLAG_EXPLICIT"],
                0,
            ),
            13: (
                c["POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET"],
                c["POLY_STATE_XSAVE_TRAP_RESTORE_BYTES"],
                c["POLY_TRAP_RESTORE_FLAG_VALID"] |
                c["POLY_TRAP_RESTORE_FLAG_AARCH64_STATE_VALID"] |
                c["POLY_TRAP_RESTORE_FLAG_RISCV_STATE_VALID"],
                0,
            ),
            14: (
                c["POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_BYTES"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES"],
            ),
            15: (
                c["POLY_STATE_XSAVE_RESERVED_OFFSET"],
                c["POLY_STATE_XSAVE_RESERVED_BYTES"],
                0,
                0,
            ),
        }
        return (True, *arch[subleaf]) if subleaf in arch else (False, 0, 0, 0, 0)
    if leaf == base + 5:
        return (
            True, c["POLY_EVENT_RECORD_LAYOUT_VERSION"],
            c["POLY_EVENT_RECORD_HEADER_BYTES"],
            c["POLY_V2_EVENT_ARG_COUNT"],
            trap_flags,
        )
    if leaf == base + 6:
        return (True, c["POLY_INTERRUPT_ABI_VERSION"], interrupt_flags, interrupt_returns, mode_raw)
    if leaf == base + 7:
        return (
            True, c["POLY_MEMORY_ABI_VERSION"],
            c["POLY_MEMORY_MODEL_X86_TSO"],
            memory_flags,
            mode_raw,
        )
    if leaf == base + 8:
        transition_align = c["POLY_TRANSITION_AARCH64_ALIGN"] | (
            c["POLY_TRANSITION_RISCV_ALIGN"] << 16
        )
        transition = {
            0: (
                c["POLY_TRANSITION_ABI_VERSION"],
                transition_flags,
                transition_align,
                mode_all,
            ),
            1: (
                c["POLY_FRONTEND_X86"],
                c["POLY_FRONTEND_AARCH64"],
                c["POLY_FRONTEND_RISCV"],
                frontend_all,
            ),
            2: (
                c["POLY_STATE_XSAVE_TRANSITION_OFFSET"],
                c["POLY_STATE_XSAVE_TRANSITION_BYTES"],
                32,
                0,
            ),
            3: (
                c["POLY_STATE_XSAVE_CROSS_RETURN_OFFSET"],
                c["POLY_STATE_XSAVE_CROSS_RETURN_BYTES"],
                c["POLY_STATE_XSAVE_CROSS_RETURN_DEPTH"],
                c["POLY_STATE_XSAVE_CROSS_RETURN_FRAME_BYTES"],
            ),
            4: (
                c["POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET"],
                c["POLY_STATE_XSAVE_IMPORT_RETURN_BYTES"],
                c["POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH"],
                c["POLY_STATE_XSAVE_IMPORT_RETURN_FRAME_BYTES"],
            ),
            5: (
                c["POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_BYTES"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH"],
                c["POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES"],
            ),
        }
        return (True, *transition[subleaf]) if subleaf in transition else (False, 0, 0, 0, 0)
    if leaf == base + 9:
        counts_align = (
            c["POLY_ABI_BRIDGE_GPR_ARG_COUNT"] |
            (c["POLY_ABI_BRIDGE_FP_ARG_COUNT"] << 8) |
            (c["POLY_ABI_BRIDGE_STACK_ALIGN"] << 16)
        )
        return (True, c["POLY_ABI_BRIDGE_ABI_VERSION"], abi_flags, counts_align, 0)
    if leaf == base + 10:
        size_edx = c["POLY_V2_EVENT_BYTES"] | (
            c["POLY_V2_SPILL_DESC_BYTES"] << 16
        )
        return (
            True,
            c["POLY_CPUID_V2_ABI_VERSION"],
            v2_features,
            c["POLY_CPUID_V2_REQUIRED_FEATURES"],
            size_edx,
        )
    return (False, 0, 0, 0, 0)


def main() -> int:
    c = parse_c_enum_constants(HEADER)
    sv = parse_sv_localparams(RTL)

    expected_sv = {
        "POLY_CPUID_BASE": c["POLY_CPUID_BASE"],
        "POLY_CPUID_MAX": c["POLY_CPUID_MAX"],
        "POLY_FEATURE_MASK": rom_model(c["POLY_CPUID_BASE"] + 1, 0, c)[3],
        "POLY_STATE_MASK": rom_model(c["POLY_CPUID_BASE"] + 3, 0, c)[1],
        "POLY_MODE_MASK": rom_model(c["POLY_CPUID_BASE"] + 1, 0, c)[2],
        "POLY_RAW_MODE_MASK": rom_model(c["POLY_CPUID_BASE"] + 6, 0, c)[4],
        "POLY_FRONTEND_MASK": rom_model(c["POLY_CPUID_BASE"] + 8, 1, c)[4],
        "POLY_STATE_XSAVE_COMPONENT_ARCH": c["POLY_STATE_XSAVE_COMPONENT_ARCH"],
        "POLY_STATE_XSAVE_BYTES_ARCH": c["POLY_STATE_XSAVE_BYTES_ARCH"],
        "POLY_STATE_XSAVE_LAYOUT_VERSION": c["POLY_STATE_XSAVE_LAYOUT_VERSION"],
        "POLY_STATE_XSAVE_ALIGN_ARCH": c["POLY_STATE_XSAVE_ALIGN_ARCH"],
        "POLY_STATE_XSAVE_LAYOUT_ECX": rom_model(c["POLY_CPUID_BASE"] + 4, 0, c)[3],
        "POLY_STATE_XSAVE_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 4, 0, c)[4],
        "POLY_STATE_XSAVE_HEADER_OFFSET": c["POLY_STATE_XSAVE_HEADER_OFFSET"],
        "POLY_STATE_XSAVE_EVENT_RECORD_OFFSET": c["POLY_STATE_XSAVE_EVENT_RECORD_OFFSET"],
        "POLY_STATE_XSAVE_TRANSITION_OFFSET": c["POLY_STATE_XSAVE_TRANSITION_OFFSET"],
        "POLY_STATE_XSAVE_AARCH64_GPR_OFFSET": c["POLY_STATE_XSAVE_AARCH64_GPR_OFFSET"],
        "POLY_STATE_XSAVE_AARCH64_FP_OFFSET": c["POLY_STATE_XSAVE_AARCH64_FP_OFFSET"],
        "POLY_STATE_XSAVE_RISCV_GPR_OFFSET": c["POLY_STATE_XSAVE_RISCV_GPR_OFFSET"],
        "POLY_STATE_XSAVE_RISCV_FP_OFFSET": c["POLY_STATE_XSAVE_RISCV_FP_OFFSET"],
        "POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET": c["POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET"],
        "POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET": c["POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET"],
        "POLY_STATE_XSAVE_CROSS_RETURN_OFFSET": c["POLY_STATE_XSAVE_CROSS_RETURN_OFFSET"],
        "POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET": c["POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET"],
        "POLY_STATE_XSAVE_LANDING_POLICY_OFFSET": c["POLY_STATE_XSAVE_LANDING_POLICY_OFFSET"],
        "POLY_STATE_XSAVE_STATE_KEY_OFFSET": c["POLY_STATE_XSAVE_STATE_KEY_OFFSET"],
        "POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET": c["POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET"],
        "POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET": c["POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET"],
        "POLY_STATE_XSAVE_RESERVED_OFFSET": c["POLY_STATE_XSAVE_RESERVED_OFFSET"],
        "POLY_ABI_SIGNATURE_SLOT_COUNT": c["POLY_ABI_SIGNATURE_SLOT_COUNT"],
        "POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES":
            c["POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES"],
        "POLY_EVENT_RECORD_LAYOUT_VERSION": c["POLY_EVENT_RECORD_LAYOUT_VERSION"],
        "POLY_EVENT_RECORD_ARG_COUNT": c["POLY_V2_EVENT_ARG_COUNT"],
        "POLY_EVENT_RECORD_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 5, 0, c)[4],
        "POLY_INTERRUPT_ABI_VERSION": c["POLY_INTERRUPT_ABI_VERSION"],
        "POLY_INTERRUPT_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 6, 0, c)[2],
        "POLY_INTERRUPT_RETURN_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 6, 0, c)[3],
        "POLY_MEMORY_ABI_VERSION": c["POLY_MEMORY_ABI_VERSION"],
        "POLY_MEMORY_MODEL_X86_TSO": c["POLY_MEMORY_MODEL_X86_TSO"],
        "POLY_MEMORY_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 7, 0, c)[3],
        "POLY_TRANSITION_ABI_VERSION": c["POLY_TRANSITION_ABI_VERSION"],
        "POLY_TRANSITION_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 8, 0, c)[2],
        "POLY_TRANSITION_ALIGN_ECX": rom_model(c["POLY_CPUID_BASE"] + 8, 0, c)[3],
        "POLY_TRANSITION_FRAME_BYTES": rom_model(c["POLY_CPUID_BASE"] + 8, 2, c)[3],
        "POLY_FRONTEND_X86": c["POLY_FRONTEND_X86"],
        "POLY_FRONTEND_AARCH64": c["POLY_FRONTEND_AARCH64"],
        "POLY_FRONTEND_RISCV": c["POLY_FRONTEND_RISCV"],
        "POLY_ABI_BRIDGE_ABI_VERSION": c["POLY_ABI_BRIDGE_ABI_VERSION"],
        "POLY_ABI_BRIDGE_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 9, 0, c)[2],
        "POLY_ABI_BRIDGE_COUNTS_ALIGN": rom_model(c["POLY_CPUID_BASE"] + 9, 0, c)[3],
        "POLY_CPUID_V2_ABI_VERSION": c["POLY_CPUID_V2_ABI_VERSION"],
        "POLY_CPUID_V2_FEATURES": rom_model(c["POLY_CPUID_BASE"] + 10, 0, c)[2],
        "POLY_CPUID_V2_REQUIRED_FEATURES":
            c["POLY_CPUID_V2_REQUIRED_FEATURES"],
        "POLY_V2_EVENT_BYTES": c["POLY_V2_EVENT_BYTES"],
        "POLY_V2_SPILL_DESC_BYTES": c["POLY_V2_SPILL_DESC_BYTES"],
        "POLY_V2_SIZE_EDX": rom_model(c["POLY_CPUID_BASE"] + 10, 0, c)[4],
        "POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS":
            c["POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS"],
        "POLY_X86_CTRL_FOREIGN_IMPORT_COUNT_STATUS":
            c["POLY_X86_CTRL_FOREIGN_IMPORT_COUNT_STATUS"],
        "POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS":
            c["POLY_X86_CTRL_AUTO_SPILL_COUNT_STATUS"],
        "POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS":
            c["POLY_X86_CTRL_AUTO_SPILL_BYTES_STATUS"],
        "POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS":
            c["POLY_X86_CTRL_AUTO_SPILL_CYCLES_STATUS"],
        "POLY_X86_CTRL_SPILL_DESC_SET": c["POLY_X86_CTRL_SPILL_DESC_SET"],
        "POLY_X86_CTRL_PRESTORE": c["POLY_X86_CTRL_PRESTORE"],
        "POLY_X86_OPCODE_GEOMETRY_EAX": rom_model(c["POLY_CPUID_BASE"] + 2, 32, c)[1],
        "POLY_X86_CTRL_PREFIX_BYTES": c["POLY_X86_CTRL_PREFIX_BYTES"],
        "POLY_X86_CTRL_TOTAL_BYTES": c["POLY_X86_CTRL_TOTAL_BYTES"],
        "POLY_X86_CTRL_SUBOP_OFFSET": c["POLY_X86_CTRL_SUBOP_OFFSET"],
        "POLY_X86_OPCODE_CONTRACT_VERSION": c["POLY_X86_OPCODE_CONTRACT_VERSION"],
        "POLY_X86_OPCODE_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 2, 33, c)[2],
        "POLY_X86_OPCODE_FAMILY_VENDOR_PROTOTYPE":
            c["POLY_X86_OPCODE_FAMILY_VENDOR_PROTOTYPE"],
        "POLY_LANDING_POLICY_SUPPORTED": rom_model(c["POLY_CPUID_BASE"] + 4, 11, c)[3],
        "POLY_STATE_KEY_FLAG_EXPLICIT": c["POLY_STATE_KEY_FLAG_EXPLICIT"],
        "POLY_TRAP_RESTORE_FLAGS": rom_model(c["POLY_CPUID_BASE"] + 4, 13, c)[3],
    }
    for name, expected in expected_sv.items():
        assert sv[name] == expected, f"{name}: rtl={sv[name]:#x} c={expected:#x}"

    assert sv["POLY_VENDOR_EBX"] == vendor_regs()[0]
    assert sv["POLY_VENDOR_ECX"] == vendor_regs()[1]
    assert sv["POLY_VENDOR_EDX"] == vendor_regs()[2]

    cases = [
        (c["POLY_CPUID_BASE"], 0),
        (c["POLY_CPUID_BASE"] + 1, 0),
        (c["POLY_CPUID_BASE"] + 2, 31),
        (c["POLY_CPUID_BASE"] + 2, 32),
        (c["POLY_CPUID_BASE"] + 2, 33),
        (c["POLY_CPUID_BASE"] + 2, 34),
        (c["POLY_CPUID_BASE"] + 3, 0),
        *[(c["POLY_CPUID_BASE"] + 4, subleaf) for subleaf in range(16)],
        (c["POLY_CPUID_BASE"] + 5, 0),
        (c["POLY_CPUID_BASE"] + 6, 0),
        (c["POLY_CPUID_BASE"] + 7, 0),
        *[(c["POLY_CPUID_BASE"] + 8, subleaf) for subleaf in range(6)],
        (c["POLY_CPUID_BASE"] + 9, 0),
        (c["POLY_CPUID_BASE"] + 10, 0),
    ]
    for leaf, subleaf in cases:
        assert rom_model(leaf, subleaf, c)[0], f"expected hit for {leaf:#x}.{subleaf}"

    forbidden_features = mask(c, """
        POLY_CPUID_FEATURE_RESERVED_IMPORT_SELECTOR_TRAPS
        POLY_CPUID_FEATURE_HARDWARE_FP64_STACK_ARGS
        POLY_CPUID_FEATURE_HARDWARE_NEUTRAL_FP64_STACK
        POLY_CPUID_FEATURE_LEGACY_AARCH64_HFA_ARGS
    """)
    forbidden_abi = mask(c, """
        POLY_ABI_BRIDGE_FLAG_HARDWARE_IMPORT_DESCRIPTORS
        POLY_ABI_BRIDGE_FLAG_HARDWARE_STACK_ARGS
        POLY_ABI_BRIDGE_FLAG_HARDWARE_USER_DESCRIPTORS
    """)
    assert (rom_model(c["POLY_CPUID_BASE"] + 1, 0, c)[3] & forbidden_features) == 0
    assert (rom_model(c["POLY_CPUID_BASE"] + 9, 0, c)[2] & forbidden_abi) == 0

    negative_cases = [
        (c["POLY_CPUID_BASE"] + 2, 0),
        (c["POLY_CPUID_BASE"] + 4, 16),
        (c["POLY_CPUID_BASE"] + 8, 6),
        (c["POLY_CPUID_BASE"] + 11, 0),
    ]
    for leaf, subleaf in negative_cases:
        assert rom_model(leaf, subleaf, c) == (False, 0, 0, 0, 0)

    print("POLY_RTL_CPUID_ROM_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
