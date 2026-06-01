# Poly ISA

Poly lets existing AArch64 and RISC-V64 user-mode code run inside an x86_64
process address space.

## Run

```sh
make image
make boot-poly-focused-validation
make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 stays responsible for boot, paging, privilege, interrupts, syscalls,
  atomics, and memory ordering.
- AArch64/RISC-V64 are ring-3 decode frontends over the same x86_64 address
  space.
- Mode switches are decoded controls, not per-instruction `#UD` envelopes.
- Foreign registers are per-thread XSAVE-style state.
- Fast calls use ABI signature slots for register-only remapping.
- Stack args, aggregates, variadics, libc policy, syscall policy, and lazy
  binding stay in software runtimes or loader thunks.
- Recoverable foreign exits produce OS-neutral trap packets.

## Encodings

- Modes: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86 controls: `0f 3a fc xx` for `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`,
  `PCALL_IMM`, `PTRAPRET`.
- AArch64 controls: reserved `HINT` encodings.
- RISC-V controls: `custom-0` encodings.

Design rationale: `docs/poly-isa-design-directions.md`.
