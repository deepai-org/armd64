# Poly ISA

Poly is an x86_64 CPU extension prototype that runs existing AArch64 and
RISC-V64 user-mode code in the same virtual address space as x86_64 code.

## Running

```sh
make image
make boot-poly-focused-validation
make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- x86_64 remains the system ISA: boot, paging, privilege, interrupts, syscalls,
  atomics, and global TSO memory ordering are x86-owned.
- AArch64 and RISC-V64 are ring-3 frontends that fetch native 32-bit
  instructions from x86_64 virtual memory.
- Frontend changes use decoded control opcodes, not `#UD` envelopes.
- Foreign registers are per-thread XSAVE-style state.
- Register-only cross-ISA calls use ABI signature slots.
- Runtime thunks handle stack arguments, aggregates, variadics, lazy binding,
  libc, and syscall policy.
- Recoverable foreign exits produce OS-neutral trap packets.

## Controls

| Control | Encoding | Inputs |
| --- | --- | --- |
| `PENTER` | `0f 3a fc 03` | frontend in `R15`, TLS/state key in `R13` |
| `PSWITCH` | `0f 3a fc 04` | target in `RBX`, frontend in `R15` |
| `PLANDING` | `0f 3a fc 05` | indirect-call landing marker |
| `PCALL` | `0f 3a fc 2d` | target `RBX`, return `R11`, signature `R12` |
| `PCALL_IMM` | `0f 3a fc 30..` | immediate signature slot |
| `PTRAPRET` | `0f 3a fc 62` | resume from a trap packet |

AArch64 controls use reserved HINT encodings. RISC-V controls use custom-0
encodings. Deeper hardware and ABI notes live in
`docs/poly-isa-design-directions.md`.
