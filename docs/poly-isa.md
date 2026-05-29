# Poly ISA

Poly is an x86_64 CPU extension that adds user-mode AArch64 and RISC-V64
frontends in the same virtual address space. It targets existing native ABI
objects and libraries, not a new compiler-only ABI.

## Frontends

| ID | Frontend | Fetch |
| --- | --- | --- |
| `0` | x86_64 | normal variable-length x86 |
| `1` | AArch64 | raw 32-bit instructions from `RIP` |
| `2` | RISC-V64 | raw RV64/RVC instructions from `RIP` |

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, and the global TSO memory model.
- Foreign instructions are fetched directly. The fast path has no `#UD`
  envelope and no one-trap-per-instruction execution model.
- All frontends share virtual memory, page permissions, TLB behavior, fault
  delivery, and x86-style TSO ordering.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only ABI cases can use cached signature slots. Stack arguments,
  aggregates, variadics, lazy binding, and ABI reshaping stay in software
  thunks.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
  and recoverable faults produce OS-neutral trap packets with eight native ABI
  argument lanes.
- Foreign register state is explicit XSAVE-style architectural state. Hidden
  CR3/TLS-keyed emulator banks are not the hardware contract.

## Controls

- Generic operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`.
- Prototype encodings: x86_64 Poly control opcode page, AArch64 reserved
  `HINT` subspace, and RISC-V `custom-0` subspace.
- Prototype Poly XSAVE component: `20`.

See `README.md` for build/run status and `docs/poly-isa-design-directions.md`
for design rationale.
