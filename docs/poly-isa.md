# Poly ISA Reference

Poly is a prototype CPU extension for running existing x86_64, AArch64, and
RISC-V64 user-mode code in one virtual address space. Rationale and open design
notes live in `docs/poly-isa-design-directions.md`.

## Contract

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, atomics,
  syscalls, and global TSO memory ordering are x86-owned.
- AArch64 and RISC-V64 are ring-3 decode frontends over the same x86_64 virtual
  address space.
- Foreign code is fetched directly: AArch64 as fixed 32-bit instructions;
  RISC-V64 as 16/32-bit instructions with RVC.
- Mode changes use decoded control instructions, not one `#UD` envelope per
  foreign instruction.
- Foreign register state is explicit per-thread XSAVE-style architectural
  state, not hidden CR3-scoped emulator state.
- Compatibility targets real native ABIs: x86_64 SysV, AAPCS64, and RISC-V
  psABI.

## Controls

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

Temporary prototype encodings:

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)   // custom-0
```

Implemented x86_64 subops:

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER mode` |
| `0x04` | `PSWITCH mode,target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL mode,target,signature` |
| `0x30..0x3c` | `PCALL` with immediate signature slot |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state key, ABI signature, monitor packet, landing policy |

Foreign frontend subops mirror the same control surface: escape, switch/call,
signature-slot call, landing validation, state key, trap vector, monitor packet,
landing policy, and trap return.

## ABI Boundary

The fast hardware boundary is register-only. ABI signature slots remap integer,
FP, and fixed 128-bit vector registers when the native ABI already carries the
value in registers. Stack arguments, variadics, by-value aggregates, lazy
binding, libc policy, and syscall policy stay in loader/runtime thunks.

Invalid signatures or non-canonical targets trap before frontend, PC, or
transition-stack state changes.

## Returns And Traps

`PCALL` records caller state in a hardware transition stack and installs a
reserved return cookie. Ordinary native returns to that cookie restore the caller
frontend and saved PC:

- x86_64: `ret`
- AArch64: `ret x30`
- RISC-V64: `ret` / `jalr x0, ra, 0`

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable exits produce OS-neutral trap packets for a ring-3 Poly monitor.
The kernel still owns privilege transitions, scheduling, signals, and hard
faults.
