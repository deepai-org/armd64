# Poly ISA

Poly is an x86_64 extension prototype for running existing AArch64 and RISC-V64
user-mode code in the same virtual address space.

## Run

```sh
make image
make boot-poly-focused-validation
make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  syscalls, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are ring-3 decode frontends fetched from x86 virtual
  memory. AArch64 fetches fixed 4-byte instructions; RISC-V fetches 16/32-bit
  RVC-capable instructions.
- ISA switches are decoded control instructions, not `#UD` trap envelopes.
- Foreign register state is per-thread XSAVE-style architectural state.
- Fast register-only cross-ISA calls use programmable ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, libc policy, and syscall
  policy stay in software thunks/runtime code.
- Recoverable foreign exits write OS-neutral trap packets.

## Control Summary

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86 controls use `0f 3a fc xx`: `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`,
  `PCALL_IMM`, and `PTRAPRET`.
- AArch64 controls use reserved HINT encodings.
- RISC-V controls use custom-0 encodings.

Detailed hardware and ABI rationale belongs in
`docs/poly-isa-design-directions.md`.
