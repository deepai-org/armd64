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
| OS-neutral syscall/libcall boundary | Trap-packet contract in `docs/poly-isa-design-directions.md`; Bochs `handle_poly_syscall_trap`; userspace monitor policy in `tools/runtime/polyexec.c`. |
| Explicit per-thread state | XSAVE-style state layout in `tools/include/polycpuid.h`; guest XCR0 module in `tools/kernel/poly_xcr0.c`; real-XSAVE gates passed. |
| Native-ABI fast path | ABI signature slots and register maps in `tools/include/polycpuid.h`; `PCALL` implementation in Bochs; cross-ISA runtime stubs in `tools/runtime/polyexec.c`. |
| Complex ABI software path | Stack, aggregate, variadic, import, and syscall policy handled by loader/runtime thunks in `tools/runtime/polyexec.c`, not by hardware descriptors. |
| Native return semantics | Return-cookie transition stacks in Bochs and XSAVE state; native `ret`/`ret x30`/RISC-V `ret` coverage in `tools/programs/nativecheck.c`. |
| Broad integration validation | `make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps` passed on 2026-06-02. |

## Not Yet Silicon-Complete

- No RTL or FPGA implementation exists in this repo.
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

1. Produce a minimal RTL/FPGA frontend-switch prototype that implements CPUID,
   Poly control decode, raw 32-bit foreign fetch, register alias slots, and the
   return-cookie stack.
2. Build a cycle-counting model for `PSWITCH`, register-only `PCALL`, native
   return-cookie recovery, and trap-packet delivery.
3. Generate a silicon-facing state-layout table from `tools/include/polycpuid.h`
   and validate it against Bochs CPUID leaves.
4. Add directed tests for every hardware exception-ordering rule: invalid
   targets, invalid slots, packet page faults, interrupted raw mode, and
   transition-stack overflow/underflow.
5. Decide the production x86 opcode allocation or define the vendor CPUID
   discovery contract that lets software consume non-Bochs encodings.
