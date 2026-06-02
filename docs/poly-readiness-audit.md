# Poly Readiness Audit

Date: 2026-06-02

Objective: finish the ISA, Bochs emulator, and userspace runtime enough to have
an FPGA/silicon-ready Poly design for existing precompiled x86_64, AArch64, and
RISC-V64 code.

## Current Evidence

| Requirement | Evidence |
| --- | --- |
| Direct foreign fetch, not per-instruction traps | `docs/poly-isa.md`; Bochs raw AArch64/RISC-V decode in `bochs/cpu/proc_ctrl.cc`; `boot-poly-full-real-xsave-arch-traps` passed. |
| Dedicated Poly control operations | `docs/poly-isa.md`; CPUID geometry in `tools/include/polycpuid.h`; Bochs x86/AArch64/RISC-V control decode in `bochs/cpu/proc_ctrl.cc`. |
| Opcode discovery contract | `tools/include/polycpuid.h` defines CPUID escape subleafs 32 and 33 for active x86 opcode geometry and vendor/prototype opcode-contract flags; Bochs returns both leaves; runtime probes validate both leaves; `rtl/poly_frontend_core.sv` exposes the Poly CPUID ROM through a hardware discovery sideband. |
| FPGA-facing RTL bring-up artifacts | `rtl/poly_ctrl_decode.sv`; `rtl/poly_frontend_handoff.sv`; `rtl/poly_frontend_step.sv`; `rtl/poly_frontend_retire.sv`; `rtl/poly_frontend_memory_retire.sv`; `rtl/poly_frontend_core.sv`; `rtl/poly_interrupt_boundary.sv`; `rtl/poly_transition_stack.sv`; `rtl/poly_transition_cycle_budget.sv`; `rtl/poly_abi_signature_slots.sv`; `rtl/poly_cpuid_rom.sv`; `rtl/poly_memory_order.sv`; `rtl/poly_raw_fetch_request.sv`; `rtl/poly_raw_fetch_stage.sv`; `rtl/poly_raw_fetch_plan.sv`; `rtl/poly_return_cookie_recover.sv`; `rtl/poly_trap_packet_encode.sv`; `rtl/poly_trap_packet_stage.sv`; `make check-poly-rtl` passed. |
| Raw frontend memory path | `rtl/poly_raw_fetch_request.sv` validates raw instruction fetch addresses; `rtl/poly_raw_fetch_stage.sv` consumes memory responses and blocks instruction retirement on memory faults. |
| Frontend fetch-to-retire prototype | `rtl/poly_frontend_memory_retire.sv` connects external x86 fetch or raw AArch64/RISC-V memory fetch to `rtl/poly_frontend_retire.sv`, producing end-to-end retire/commit/fault outputs. |
| Frontend/transition-stack integration | `rtl/poly_frontend_core.sv` connects precise retired PCALL commits, TSO memory-order backpressure, raw interrupt save/restore, trap-packet delivery, ABI signature lookup, CPUID discovery, cycle-budget reporting, and native return-cookie recovery to `rtl/poly_transition_stack.sv`, blocks PCALL retirement when the transition stack is full or servicing a pop, and keeps return-cookie recovery from colliding with another same-cycle pop. |
| Poly control retirement ordering | `rtl/poly_frontend_retire.sv` prevents frontend/PC/transition-stack commits while fetch, execute, or async-boundary handling is not ready, or when older, fetch, execution, or control-validation faults are pending. |
| Hardware-shaped interrupt boundary | `rtl/poly_interrupt_boundary.sv` validates CPL3 raw interrupt entry and matching user-return restore; `rtl/poly_frontend_core.sv` stores the interrupted raw frontend/PC state and blocks same-cycle instruction retirement during interrupt entry or restore. |
| Hardware-shaped trap delivery | `rtl/poly_trap_packet_encode.sv` emits the 16-qword monitor packet and rejects disabled, non-canonical, unaligned, and boundary-crossing packet addresses before delivery. |
| Trap packet memory faults | `rtl/poly_trap_packet_stage.sv` emits the packet write request, waits for memory completion, and reports monitor-packet write/page faults without delivering the trap; `rtl/poly_frontend_core.sv` blocks same-cycle retirement while the packet write waits or delivers and maps packet errors to execute faults. |
| Hardware-shaped native return recovery | `rtl/poly_return_cookie_recover.sv` detects ordinary native returns to the Poly return cookie and requests transition-stack recovery. |
| Fast-path cycle budget | `rtl/poly_transition_cycle_budget.sv` models fixed-cycle `PSWITCH`, register-only `PCALL`, native return-cookie recovery, and trap-packet fixed work plus memory-response latency; `rtl/poly_frontend_core.sv` drives that budget model from committed transition, return-cookie, and trap-packet events. |
| OS-neutral syscall/libcall boundary | Trap-packet contract in `docs/poly-isa-design-directions.md`; Bochs `handle_poly_syscall_trap`; userspace monitor policy in `tools/runtime/polyexec.c`. |
| Explicit per-thread state | XSAVE-style state layout in `tools/include/polycpuid.h`; silicon-facing layout/check program in `tools/programs/polylayout.c`; guest XCR0 module in `tools/kernel/poly_xcr0.c`; real-XSAVE gates passed. |
| Native-ABI fast path | ABI signature slots and register maps in `tools/include/polycpuid.h`; `rtl/poly_abi_signature_slots.sv` caches register-only signature slots; `rtl/poly_frontend_core.sv` exposes committed PCALL signature kind/map/TLS metadata for RAT/register-alias application; `PCALL` implementation in Bochs; cross-ISA runtime stubs in `tools/runtime/polyexec.c`. |
| Complex ABI software path | Stack, aggregate, variadic, import, and syscall policy handled by loader/runtime thunks in `tools/runtime/polyexec.c`, not by hardware descriptors. |
| Native return semantics | Return-cookie transition stacks in Bochs and XSAVE state; native `ret`/`ret x30`/RISC-V `ret` coverage in `tools/programs/nativecheck.c`. |
| x86 TSO memory-order policy | `tools/include/polycpuid.h` advertises `POLY_MEMORY_MODEL_X86_TSO`; `rtl/poly_memory_order.sv` gates memory-op retirement so foreign frontends do not expose weak reordering and foreign barriers/fences retire as no-ops; `rtl/poly_frontend_core.sv` feeds memory-order waits into precise execute-stage retirement backpressure; `rtl/test_poly_memory_order_litmus.py` checks TSO message-passing and store-buffering outcomes; `rtl/poly_memory_order_formal.sv` captures the intended formal assertions. |
| Broad integration validation | `make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps` passed on 2026-06-02. |

## Not Yet Silicon-Complete

- Only the first RTL bring-up blocks exist. There is not yet a full RTL/FPGA
  CPU frontend-switch implementation.
- A directed cycle-budget model exists for the few-cycle `PSWITCH`/`PCALL`
  target, but no synthesized timing/timing-closure proof exists.
- Bochs proves functional behavior, not timing, area, power, or decode-stage
  feasibility.
- Foreign ISA support is broad enough for current fixtures, but not a complete
  architectural implementation of every AArch64/RISC-V extension.
- OS integration is modeled through an XSAVE-style component and a guest test
  module; no upstream Linux/Windows/macOS kernel support exists.
- Hardware transition-stack depth, exception ordering, and return-cookie
  behavior are specified and tested functionally, but not formally verified.

## Next Engineering Gates

1. Discharge `rtl/poly_memory_order_formal.sv` with a formal backend. The
   current repository has directed tests, litmus-style tests, and assertion
   properties, but no installed formal tool has run the proof.
2. Convert the vendor/prototype x86 opcode family into a production allocation
   or keep it as a CPUID-discovered vendor extension for FPGA bring-up.
