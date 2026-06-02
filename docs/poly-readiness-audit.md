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
| FPGA-facing RTL bring-up artifacts | `rtl/poly_ctrl_decode.sv`; `rtl/poly_frontend_handoff.sv`; `rtl/poly_frontend_step.sv`; `rtl/poly_transition_stack.sv`; `rtl/poly_abi_signature_slots.sv`; `rtl/poly_cpuid_rom.sv`; `rtl/poly_raw_fetch_plan.sv`; `rtl/poly_trap_packet_encode.sv`; `make check-poly-rtl` passed. |
| Hardware-shaped trap delivery | `rtl/poly_trap_packet_encode.sv` emits the 16-qword monitor packet and rejects disabled, non-canonical, unaligned, and boundary-crossing packet addresses before delivery. |
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
   packet page faults, interrupted raw mode, and instruction-retirement faults.
5. Decide the production x86 opcode allocation or define the vendor CPUID
   discovery contract that lets software consume non-Bochs encodings.
