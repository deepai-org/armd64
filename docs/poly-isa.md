# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64
frontends in the same process and virtual address space.

## Architecture Contract

| Area | Contract |
| --- | --- |
| System ISA | x86_64 owns privilege, paging, faults, interrupts, VM control, atomics, and TSO ordering. |
| User frontends | AArch64 and RISC-V64 fetch native instructions from x86_64 virtual memory. |
| ABI target | Real precompiled ABI compatibility: SysV x86_64, AAPCS64, and RISC-V psABI. |
| Fast calls | Register-only calls may use ABI signature slots to rename or alias registers. |
| Slow calls | Software thunks handle stack args, aggregates, variadics, lazy binding, and incompatible vectors. |
| State | Non-x86 architectural state is explicit per-thread XSAVE-style state. |
| Traps | Recoverable foreign exits produce OS-neutral trap packets for a user runtime. |

Hardware must not implement OS, libc, linker, libgcc, or libatomic policy.

## Differences From x86_64

| Topic | Poly behavior |
| --- | --- |
| Decode | The frontend can switch among x86_64, AArch64, and RISC-V64. |
| Fetch width | x86_64 remains variable length; AArch64 is 4-byte aligned; RISC-V is 2-byte aligned when RVC is enabled. |
| Returns | Cross-ISA calls use native return instructions plus a hardware transition stack and reserved return cookies. |
| Syscalls/traps | Foreign syscalls and recoverable traps exit through trap packets, not hidden OS or libc emulation. |

## Control Ops

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Purpose |
| --- | --- |
| `PENTER` | Enter a frontend at the next instruction. |
| `PSWITCH` | Branch to another frontend. |
| `PCALL` | Cross-ISA call using an ABI signature slot. |
| `PTRAPRET` | Resume from the user trap monitor. |
| `PLANDING` | Validate indirect cross-ISA landing points when enabled. |

Prototype encodings are decoded control instructions, not `#UD` envelopes:
x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.

See `README.md` for build/test commands and
`docs/poly-isa-design-directions.md` for design rationale.
