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
- `poly_frontend_decode_dispatch.sv`: fetch/decode dispatch boundary that
  normalizes x86 byte fetch and raw AArch64/RISC-V instruction words before
  Poly control decode.
- `poly_frontend_fetch_decode_pipeline.sv`: integrated fetch/decode wrapper
  that composes request issue, x86/raw response stages, and Poly control
  decode dispatch.
- `poly_frontend_step.sv`: one-step planner that composes raw fetch geometry,
  decode dispatch, and handoff validation for an already-fetched instruction
  word.
- `poly_frontend_retire.sv`: retirement gate that blocks frontend, PC, and
  transition-stack mutations behind fetch/execute/async-boundary stalls or
  older, fetch, execution, and control faults.
- `poly_frontend_predecoded_retire.sv`: retirement gate for predecoded Poly
  control records from the fetch/decode pipeline.
- `poly_frontend_fetch_issue.sv`: dual frontend fetch-request issuer for x86
  byte fetch versus raw AArch64/RISC-V instruction fetch.
- `poly_frontend_memory_retire.sv`: fetch-to-retire prototype that feeds the
  integrated fetch/decode pipeline into frontend retirement commits.
- `poly_frontend_core.sv`: frontend core wrapper that connects fetch-to-retire
  commits, TSO memory-order backpressure, raw interrupt save/restore,
  trap-packet delivery, ABI signature lookup, CPUID discovery, and native
  return-cookie recovery to the hardware transition stack, with fast-path
  cycle-budget reporting for integrated transition events.
- `poly_frontend_state.sv`: architectural frontend/PC state register that
  validates and applies committed transitions, raw interrupt restores, and
  native return-cookie resumes, and emits same-cycle redirect metadata for
  frontend flush.
- `poly_frontend_stateful_core.sv`: stateful wrapper that feeds
  `poly_frontend_core.sv` from architectural frontend/PC state and applies
  retired commits, raw interrupt entry/restore, and native return-cookie resume
  updates back into that state while exposing redirect sidebands.
- `poly_interrupt_boundary.sv`: raw frontend interrupt entry and user-return
  restore planner for precise interrupted-PC handling.
- `poly_transition_stack.sv`: fixed-depth hardware transition stack for
  `PCALL` and native return-cookie recovery.
- `poly_transition_cycle_budget.sv`: cycle-budget model for fixed-latency
  `PSWITCH`, register-only `PCALL`, return-cookie recovery, and trap delivery.
- `poly_abi_signature_slots.sv`: cached register-only ABI signature slots for
  hardware register aliasing.
- `poly_cpuid_rom.sv`: Poly vendor CPUID discovery ROM for feature bits, XSAVE
  geometry, trap packets, transitions, and ABI signatures.
- `poly_memory_order.sv`: x86 TSO memory-order retirement policy for all Poly
  frontends, including foreign barrier/fence no-op handling.
- `poly_memory_order_formal.sv`: formal harness with assertions for the TSO
  retirement policy. It requires an external formal backend to discharge.
- `poly_x86_fetch_stage.sv`: x86 frontend response stage that waits for the
  byte frontend and reports response faults before retirement.
- `poly_raw_fetch_plan.sv`: raw AArch64/RISC-V fetch geometry for alignment,
  instruction width, instruction bits, and next-PC calculation.
- `poly_raw_fetch_request.sv`: raw AArch64/RISC-V instruction-memory request
  geometry with canonical-range and alignment validation.
- `poly_raw_fetch_response_stage.sv`: raw AArch64/RISC-V instruction-memory
  response stage that waits for memory, extracts instruction width, and reports
  response faults before retirement.
- `poly_raw_fetch_stage.sv`: raw fetch request/response stage that blocks
  instruction retirement on request errors or memory faults.
- `poly_return_cookie_recover.sv`: native return-cookie detector that requests
  transition-stack recovery for ordinary native returns to the Poly cookie.
- `poly_trap_packet_encode.sv`: OS-neutral trap-packet encoder for recoverable
  foreign exits and monitor-packet address validation.
- `poly_trap_packet_stage.sv`: trap-packet write/delivery stage that waits for
  memory completion and reports monitor-packet page/write faults.
- `test_poly_ctrl_decode.py`: static and behavioral consistency test against
  `tools/include/polycpuid.h`.
- `test_poly_frontend_handoff.py`: transition fault-ordering checks against
  `tools/include/polycpuid.h`.
- `test_poly_frontend_decode_dispatch.py`: fetch/decode dispatch checks for
  x86 control words, raw AArch64/RISC-V instruction width, fallthrough PC, and
  raw alignment suppression.
- `test_poly_frontend_fetch_decode_pipeline.py`: integrated fetch/decode
  checks for x86/raw request issue, wait/fault handling, and Poly control
  decode dispatch.
- `test_poly_frontend_step.py`: integration checks for raw fetch, control
  decode, and handoff composition.
- `test_poly_frontend_retire.py`: retirement-ordering checks that
  fetch/execute stalls block retirement and faults suppress Poly control commits
  and transition-stack pushes.
- `test_poly_frontend_predecoded_retire.py`: predecoded retirement checks for
  wait/fault ordering and handoff validation without re-running decode.
- `test_poly_frontend_fetch_issue.py`: dual frontend fetch-request checks for
  x86 canonical-range validation and raw AArch64/RISC-V request validation.
- `test_poly_frontend_memory_retire.py`: fetch-to-retire integration checks for
  x86 external fetch and raw AArch64/RISC-V memory fetch.
- `test_poly_frontend_core.py`: frontend/transition-stack integration checks
  for PCALL push, stack-full blocking, TSO memory-order backpressure, raw
  interrupt save/restore, trap-packet wait/deliver/fault handling, ABI
  signature lookup, CPUID discovery, return-cookie recovery, transition
  cycle-budget reporting, and return-pop conflict avoidance.
- `test_poly_frontend_state.py`: architectural frontend/PC state checks for
  committed transitions, raw interrupt restores, native return-cookie resumes,
  stalls, faults, invalid targets, and update conflicts.
- `test_poly_frontend_stateful_core.py`: stateful core wiring checks for
  state-fed frontend/PC inputs and state updates from retire, interrupt, and
  return-cookie paths.
- `test_poly_interrupt_boundary.py`: interrupt-entry and user-return restore
  checks against `tools/include/polycpuid.h`.
- `test_poly_transition_stack.py`: behavioral transition-stack checks against
  `tools/include/polycpuid.h`.
- `test_poly_transition_cycle_budget.py`: cycle-budget checks for the
  hardware fast paths and trap-packet memory response accounting.
- `test_poly_abi_signature_slots.py`: signature-slot checks against
  `tools/include/polycpuid.h`.
- `test_poly_cpuid_rom.py`: CPUID ROM checks against
  `tools/include/polycpuid.h`.
- `test_poly_memory_order.py`: memory-order policy checks against the CPUID
  TSO contract.
- `test_poly_memory_order_formal.py`: static check that the formal harness
  contains the expected TSO assertions.
- `test_poly_memory_order_litmus.py`: litmus-style checks for x86 TSO message
  passing, store buffering, and coherence behavior.
- `test_poly_x86_fetch_stage.py`: x86 fetch response-stage checks for wait,
  instruction-valid, fallthrough, and response-fault behavior.
- `test_poly_raw_fetch_plan.py`: raw foreign-fetch geometry checks against
  `tools/include/polycpuid.h`.
- `test_poly_raw_fetch_request.py`: raw instruction-fetch request checks
  against `tools/include/polycpuid.h`.
- `test_poly_raw_fetch_response_stage.py`: raw instruction-fetch response
  stage checks for wait, instruction extraction, memory faults, and response
  alignment faults.
- `test_poly_raw_fetch_stage.py`: raw instruction-fetch request/response
  integration checks.
- `test_poly_return_cookie_recover.py`: native return-cookie recovery checks
  against `tools/include/polycpuid.h` and Bochs cookie constants.
- `test_poly_trap_packet_encode.py`: trap-packet layout and monitor-address
  validation checks against `tools/include/polycpuid.h`.
- `test_poly_trap_packet_stage.py`: trap-packet delivery ordering checks for
  write completion and monitor-packet memory faults.

## Run

```bash
python3 rtl/test_poly_ctrl_decode.py
python3 rtl/test_poly_frontend_handoff.py
python3 rtl/test_poly_frontend_decode_dispatch.py
python3 rtl/test_poly_frontend_fetch_decode_pipeline.py
python3 rtl/test_poly_frontend_step.py
python3 rtl/test_poly_frontend_retire.py
python3 rtl/test_poly_frontend_predecoded_retire.py
python3 rtl/test_poly_frontend_fetch_issue.py
python3 rtl/test_poly_frontend_memory_retire.py
python3 rtl/test_poly_frontend_core.py
python3 rtl/test_poly_frontend_state.py
python3 rtl/test_poly_frontend_stateful_core.py
python3 rtl/test_poly_interrupt_boundary.py
python3 rtl/test_poly_transition_stack.py
python3 rtl/test_poly_abi_signature_slots.py
python3 rtl/test_poly_cpuid_rom.py
python3 rtl/test_poly_memory_order.py
python3 rtl/test_poly_memory_order_formal.py
python3 rtl/test_poly_memory_order_litmus.py
python3 rtl/test_poly_x86_fetch_stage.py
python3 rtl/test_poly_raw_fetch_request.py
python3 rtl/test_poly_raw_fetch_response_stage.py
python3 rtl/test_poly_raw_fetch_stage.py
python3 rtl/test_poly_raw_fetch_plan.py
python3 rtl/test_poly_return_cookie_recover.py
python3 rtl/test_poly_transition_cycle_budget.py
python3 rtl/test_poly_trap_packet_encode.py
python3 rtl/test_poly_trap_packet_stage.py
```

Run HDL frontend lint, Yosys process/check, and generic synthesis for the
integrated stateful frontend core:

```bash
make check-poly-rtl-hdl
```

Expected output:

```text
POLY_RTL_CTRL_DECODE_OK
POLY_RTL_FRONTEND_HANDOFF_OK
POLY_RTL_FRONTEND_DECODE_DISPATCH_OK
POLY_RTL_FRONTEND_FETCH_DECODE_PIPELINE_OK
POLY_RTL_FRONTEND_STEP_OK
POLY_RTL_FRONTEND_RETIRE_OK
POLY_RTL_FRONTEND_PREDECODED_RETIRE_OK
POLY_RTL_FRONTEND_FETCH_ISSUE_OK
POLY_RTL_FRONTEND_MEMORY_RETIRE_OK
POLY_RTL_FRONTEND_CORE_OK
POLY_RTL_FRONTEND_STATE_OK
POLY_RTL_FRONTEND_STATEFUL_CORE_OK
POLY_RTL_INTERRUPT_BOUNDARY_OK
POLY_RTL_TRANSITION_STACK_OK
POLY_RTL_ABI_SIGNATURE_SLOTS_OK
POLY_RTL_CPUID_ROM_OK
POLY_RTL_MEMORY_ORDER_OK
POLY_RTL_MEMORY_ORDER_FORMAL_OK
POLY_RTL_MEMORY_ORDER_LITMUS_OK
POLY_RTL_X86_FETCH_STAGE_OK
POLY_RTL_RAW_FETCH_REQUEST_OK
POLY_RTL_RAW_FETCH_RESPONSE_STAGE_OK
POLY_RTL_RAW_FETCH_STAGE_OK
POLY_RTL_RAW_FETCH_PLAN_OK
POLY_RTL_RETURN_COOKIE_RECOVER_OK
POLY_RTL_TRANSITION_CYCLE_BUDGET_OK
POLY_RTL_TRAP_PACKET_ENCODE_OK
POLY_RTL_TRAP_PACKET_STAGE_OK
POLY_RTL_FRONTEND_STATE_SIM_OK
POLY_RTL_TRANSITION_STACK_SIM_OK
```
