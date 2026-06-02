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
| FPGA-facing RTL bring-up artifacts | `rtl/poly_ctrl_decode.sv`; `rtl/poly_frontend_handoff.sv`; `rtl/poly_frontend_step.sv`; `rtl/poly_frontend_retire.sv`; `rtl/poly_interrupt_boundary.sv`; `rtl/poly_transition_stack.sv`; `rtl/poly_abi_signature_slots.sv`; `rtl/poly_cpuid_rom.sv`; `rtl/poly_raw_fetch_request.sv`; `rtl/poly_raw_fetch_stage.sv`; `rtl/poly_raw_fetch_plan.sv`; `rtl/poly_return_cookie_recover.sv`; `rtl/poly_trap_packet_encode.sv`; `rtl/poly_trap_packet_stage.sv`; `make check-poly-rtl` passed. |
| Raw frontend memory path | `rtl/poly_raw_fetch_request.sv` validates raw instruction fetch addresses; `rtl/poly_raw_fetch_stage.sv` consumes memory responses and blocks instruction retirement on memory faults. |
| Poly control retirement ordering | `rtl/poly_frontend_retire.sv` prevents frontend/PC/transition-stack commits when older, fetch, execution, or control-validation faults are pending. |
| Hardware-shaped interrupt boundary | `rtl/poly_interrupt_boundary.sv` records precise raw frontend PC on CPL3 interrupt entry and resumes raw mode only on matching user-return PC. |
| Hardware-shaped trap delivery | `rtl/poly_trap_packet_encode.sv` emits the 16-qword monitor packet and rejects disabled, non-canonical, unaligned, and boundary-crossing packet addresses before delivery. |
| Trap packet memory faults | `rtl/poly_trap_packet_stage.sv` emits the packet write request, waits for memory completion, and reports monitor-packet write/page faults without delivering the trap. |
| Hardware-shaped native return recovery | `rtl/poly_return_cookie_recover.sv` detects ordinary native returns to the Poly return cookie and requests transition-stack recovery. |
| OS-neutral syscall/libcall boundary | Trap-packet contract in `docs/poly-isa-design-directions.md`; Bochs `handle_poly_syscall_trap`; userspace monitor policy in `tools/runtime/polyexec.c`. |
| Explicit per-thread state | XSAVE-style state layout in `tools/include/polycpuid.h`; guest XCR0 module in `tools/kernel/poly_xcr0.c`; real-XSAVE gates passed. |
| Native-ABI fast path | ABI signature slots and register maps in `tools/include/polycpuid.h`; `PCALL` implementation in Bochs; cross-ISA runtime stubs in `tools/runtime/polyexec.c`. |
| Complex ABI software path | Stack, aggregate, variadic, import, and syscall policy handled by loader/runtime thunks in `tools/runtime/polyexec.c`, not by hardware descriptors. |
| Native return semantics | Return-cookie transition stacks in Bochs and XSAVE state; native `ret`/`ret x30`/RISC-V `ret` coverage in `tools/programs/nativecheck.c`. |
| Broad integration validation | `make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps` passed on 2026-06-02. |

## Not Yet Silicon-Complete

- Only the first RTL bring-up blocks exist. There is not yet a full RTL/FPGA
  CPU frontend-switch implementation.
- No cycle-level proof exists for the few-cycle `PSWITCH`/`PCALL` target.
- Bochs proves functional behavior, not timing, area, power, or decode-stage
  feasibility.
- Foreign ISA support is broad enough for current fixtures, but not a complete
  architectural implementation of every AArch64/RISC-V extension.
- OS integration is modeled through an XSAVE-style component and a guest test
  module; no upstream Linux/Windows/macOS kernel support exists.
- Hardware transition-stack depth, exception ordering, and return-cookie
  behavior are specified and tested functionally, but not formally verified.

## Next Engineering Gates

1. Extend the one-step RTL planner into a frontend/memory prototype that
   performs real instruction fetch and end-to-end instruction retirement.
2. Build a cycle-counting model for `PSWITCH`, register-only `PCALL`, native
   return-cookie recovery, and trap-packet delivery.
3. Generate a silicon-facing state-layout table from `tools/include/polycpuid.h`
   and validate it against Bochs CPUID leaves.
4. Add directed tests for remaining hardware exception-ordering rules:
   full memory ordering.
5. Decide the production x86 opcode allocation or define the vendor CPUID
   discovery contract that lets software consume non-Bochs encodings.
