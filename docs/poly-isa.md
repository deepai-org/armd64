# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
virtual address space. The goal is compatibility with existing native ABI
objects, not a new compiler-only ABI.

## Frontends

| ID | ISA | Fetch |
| --- | --- | --- |
| `0` | x86_64 | normal x86 byte stream |
| `1` | AArch64 | raw 32-bit words from `RIP` |
| `2` | RISC-V64 | raw RV64/RVC from `RIP` |

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, and the global TSO memory model.
- Foreign code is direct-fetched. There is no `#UD` envelope and no trap per
  instruction on the fast path.
- All frontends share virtual memory, page permissions, TLB behavior, fault
  delivery, and x86-style TSO ordering.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only ABI cases may use cached signature slots. Stack arguments,
  aggregates, variadics, lazy binding, and ABI reshaping remain software thunk
  work.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
  and recoverable faults produce OS-neutral trap packets.
- Foreign register state is explicit XSAVE-style architectural state, not
  hidden CR3/TLS-keyed emulator state.

## Controls

- Operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`.
- Temporary encodings: x86_64 Poly control page, AArch64 reserved `HINT`
  subspace, and RISC-V `custom-0`.
- Prototype XSAVE component: `20`.

See `README.md` for build/run status and `docs/poly-isa-design-directions.md`
for design rationale.
