# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system. The
system frontend remains x86_64: it owns boot, privilege transitions, paging,
interrupt delivery, virtual memory, atomics, and the global TSO memory model.

| ID | Frontend | Fetch |
| -- | -------- | ----- |
| `0` | x86_64 | normal variable-length x86 decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## What Changes From x86_64

- Foreign instructions are real frontend decode, not per-instruction `#UD`
  envelopes.
- All frontends share one x86_64 virtual address space, TLB, permission model,
  precise fault model, and TSO ordering.
- Foreign register state is explicit XSAVE-style architectural state, not a
  hidden CR3/TLS-keyed emulator bank.
- Cross-ISA calls target existing native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast register-only calls may use cached ABI signature slots to alias register
  lanes at the frontend boundary.
- Stack arguments, aggregates, variadics, lazy binding, and incompatible vector
  layouts stay in loader/runtime thunks.
- Foreign traps produce OS-neutral trap records. Hardware must not emulate libc
  or Linux syscall policy.

## Prototype Encodings

- x86_64 control page: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT` subspace
- RISC-V64 controls: `custom-0` subspace
- Poly XSAVE component: `20`

## Core Operations

- `PENTER frontend`: enter a non-x86 frontend from system/runtime code.
- `PSWITCH frontend, target`: switch frontends without installing a return.
- `PCALL frontend, target, sig`: call another frontend and install a native
  return cookie through the hardware transition stack.
- `PTRAPRET`: resume from a precise poly trap.

## References

- Design rationale: `docs/poly-isa-design-directions.md`
- Shared constants: `tools/include/polycpuid.h`
