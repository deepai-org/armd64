# Poly ISA

Poly adds raw user-mode AArch64 and RISC-V64 frontends to an x86_64 system.
x86_64 remains responsible for boot, privilege, paging, interrupts, virtual
memory, atomics, syscalls, and the TSO memory contract.

| ID | Frontend | Fetch |
| -- | -------- | ----- |
| `0` | x86_64 | normal variable-length x86 decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## Differences From x86_64

- Foreign instructions are real frontend decode, not per-instruction `#UD`
  envelopes.
- All frontends share one x86_64 virtual address space, TLB, permission model,
  precise fault model, and TSO ordering.
- Foreign register state is explicit XSAVE-style architectural state, not a
  hidden CR3/TLS-keyed emulator bank.
- Cross-ISA calls target native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Fast register-only calls may use cached ABI signature slots for register
  aliasing; stack arguments, aggregates, variadics, lazy binding, and complex
  vectors stay in loader/runtime thunks.
- Foreign traps produce OS-neutral trap records. Hardware must not emulate libc
  or Linux syscall policy.

## Prototype Encodings

- x86_64 control page: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT` subspace
- RISC-V64 controls: `custom-0` subspace
- Poly XSAVE component: `20`

Design rationale: `docs/poly-isa-design-directions.md`
Shared constants: `tools/include/polycpuid.h`
