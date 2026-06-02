# Poly RTL Bring-Up

This directory contains minimal hardware-facing artifacts. The current module is
not a full CPU; it is the first fixed-latency decode block needed by an FPGA or
silicon prototype.

## Files

- `poly_ctrl_decode.sv`: synthesizable SystemVerilog decoder for x86_64,
  AArch64, and RISC-V64 Poly control instructions.
- `poly_frontend_handoff.sv`: fixed-latency frontend/PC handoff controller with
  pre-commit validation for targets, alignment, signature slots, and stack
  capacity.
- `poly_frontend_step.sv`: one-step planner that composes raw fetch geometry,
  control decode, and handoff validation for an already-fetched instruction
  word.
- `poly_transition_stack.sv`: fixed-depth hardware transition stack for
  `PCALL` and native return-cookie recovery.
- `poly_abi_signature_slots.sv`: cached register-only ABI signature slots for
  hardware register aliasing.
- `poly_cpuid_rom.sv`: Poly vendor CPUID discovery ROM for feature bits, XSAVE
  geometry, trap packets, transitions, and ABI signatures.
- `poly_raw_fetch_plan.sv`: raw AArch64/RISC-V fetch geometry for alignment,
  instruction width, instruction bits, and next-PC calculation.
- `test_poly_ctrl_decode.py`: static and behavioral consistency test against
  `tools/include/polycpuid.h`.
- `test_poly_frontend_handoff.py`: transition fault-ordering checks against
  `tools/include/polycpuid.h`.
- `test_poly_frontend_step.py`: integration checks for raw fetch, control
  decode, and handoff composition.
- `test_poly_transition_stack.py`: behavioral transition-stack checks against
  `tools/include/polycpuid.h`.
- `test_poly_abi_signature_slots.py`: signature-slot checks against
  `tools/include/polycpuid.h`.
- `test_poly_cpuid_rom.py`: CPUID ROM checks against
  `tools/include/polycpuid.h`.
- `test_poly_raw_fetch_plan.py`: raw foreign-fetch geometry checks against
  `tools/include/polycpuid.h`.

## Run

```bash
python3 rtl/test_poly_ctrl_decode.py
python3 rtl/test_poly_frontend_handoff.py
python3 rtl/test_poly_frontend_step.py
python3 rtl/test_poly_transition_stack.py
python3 rtl/test_poly_abi_signature_slots.py
python3 rtl/test_poly_cpuid_rom.py
python3 rtl/test_poly_raw_fetch_plan.py
```

Expected output:

```text
POLY_RTL_CTRL_DECODE_OK
POLY_RTL_FRONTEND_HANDOFF_OK
POLY_RTL_FRONTEND_STEP_OK
POLY_RTL_TRANSITION_STACK_OK
POLY_RTL_ABI_SIGNATURE_SLOTS_OK
POLY_RTL_CPUID_ROM_OK
POLY_RTL_RAW_FETCH_PLAN_OK
```
