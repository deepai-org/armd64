# Poly ISA Quick Reference

Poly is a prototype CPU extension for running existing x86_64, AArch64, and
RISC-V64 user-mode code in one virtual address space.

This file is the compact operational reference. Design rationale lives in
`docs/poly-isa-design-directions.md`.

## Architectural Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  atomics, syscalls, and global TSO memory ordering.
- AArch64 and RISC-V64 are ring-3 decode frontends over the same x86_64 virtual
  address space.
- Mode switches and calls are decoded control instructions. The fast path does
  not use one `#UD` envelope per foreign instruction.
- AArch64 fetch is fixed 32-bit. RISC-V64 supports 16/32-bit fetch, including
  RVC.
- Foreign state is explicit per-thread XSAVE-style architectural state, not
  hidden CR3-scoped emulator state.
- Cross-ISA compatibility targets real native ABIs: x86_64 SysV, AArch64
  AAPCS64, and RISC-V psABI.

## Control Encodings

Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

x86_64 controls use the temporary prototype form:

```text
0f 3a fc <subop>
```

Implemented x86 subops:

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER mode` |
| `0x04` | `PSWITCH mode,target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL mode,target,signature` |
| `0x30..0x3c` | `PCALL` with immediate signature slot |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state key, ABI signature, monitor packet, landing policy |

AArch64 controls use reserved `HINT` encodings:

```c
0xd503201f | ((subop & 0x7f) << 5)
```

RISC-V controls use `custom-0` encodings:

```c
0x0000700b | ((subop & 0x7f) << 25)
```

Foreign subops cover the same operations: escape to x86, trap return, frontend
switch/call, signature-slot call, landing validation, state key, trap vector,
monitor packet, and landing policy.

## ABI Boundary

The hardware contract is register-only at the hot boundary.

- ABI signature slots describe fixed register remaps for fast `PCALL`.
- Integer, FP, and fixed 128-bit vector register paths are supported where the
  native ABI represents the value in registers.
- Stack arguments, variadics, by-value aggregates, lazy binding, libc policy,
  and syscall policy belong in loader/runtime thunks.
- Invalid signatures or non-canonical targets trap before frontend, PC, or
  transition-stack state changes.

Current signature slots are compact and register-only: exchange/null, x86_64
SysV register cases, native AArch64/RISC-V register cases, FP/vector cases, and
register-shaped structure returns.

## Returns

Cross-ISA calls return through ordinary native return instructions:

- x86_64: `ret`
- AArch64: `ret x30`
- RISC-V64: `ret` / `jalr x0, ra, 0`

`PCALL` installs a reserved return cookie and records caller state in a hardware
transition stack. Returning to the cookie restores the caller frontend and
continues at the saved caller PC. Same-ISA returns remain normal.

## Traps

Foreign `svc`/`ecall`, breakpoints, illegal or unsupported instructions,
unresolved imports, and recoverable frontend exits produce OS-neutral trap
packets. A registered ring-3 Poly monitor may translate syscalls, bind imports,
or resume with `PTRAPRET`; the kernel still owns real privilege transitions,
signals, scheduling, and hard faults.
