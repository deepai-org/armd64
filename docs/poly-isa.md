# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64 decode
frontends. x86_64 stays the system ISA: it owns boot, paging, interrupts,
privilege transitions, atomics, and the global TSO memory model.

| ID | Frontend | Fetch path |
| -- | -------- | ---------- |
| `0` | x86_64 | normal x86 variable-length decode |
| `1` | AArch64 | direct 32-bit fetch from `RIP` |
| `2` | RISC-V64 | direct RV64/RVC fetch from `RIP` |

## Difference From x86_64

- Foreign code is decoded directly. There are no per-instruction `#UD`
  envelopes.
- All frontends share the same x86_64 virtual address space, TLB, page faults,
  permissions, and TSO ordering.
- Foreign registers are architectural XSAVE-style state, not hidden emulator
  state keyed by CR3 or TLS.
- Cross-ISA calls preserve real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Register-only calls can use cached ABI signature slots for fast frontend
  boundary register aliasing.
- Stack arguments, aggregates, variadics, lazy binding, and incompatible vector
  layouts are handled by loader/runtime thunks.
- Traps are OS-neutral records. Hardware does not implement Linux syscall
  policy or libc helpers.

## Control Surface

- x86_64 prototype controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT` subspace
- RISC-V64 controls: `custom-0` subspace
- Poly XSAVE component: `20`
- `PENTER frontend`: enter a foreign frontend.
- `PSWITCH frontend, target`: switch frontend without installing a return.
- `PCALL frontend, target, sig`: cross-ISA call using a native return cookie
  and hardware transition stack.
- `PTRAPRET`: resume from a precise poly trap.

Full rationale: `docs/poly-isa-design-directions.md`.
Shared constants: `tools/include/polycpuid.h`.
