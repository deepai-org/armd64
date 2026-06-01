# Poly ISA Quick Reference

Poly is an x86_64 extension prototype for running existing AArch64 and RISC-V64
user-mode code in the same virtual address space as x86_64 code.

## Run

```sh
make image
make boot-poly-focused-validation
make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Contract

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  syscalls, atomics, and TSO memory ordering.
- Frontends are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.
- AArch64 and RISC-V64 are ring-3 frontends that fetch native 32-bit
  instructions from x86 virtual memory.
- Mode switches are decoded control instructions, not `#UD` trap envelopes.
- Foreign register state is per-thread XSAVE-style architectural state.
- Register-only cross-ISA calls use programmable ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, libc policy, and syscall
  policy stay in software thunks/runtime code.
- Recoverable foreign exits produce OS-neutral trap packets.

## x86 Controls

| Name | Encoding | Operands |
| --- | --- | --- |
| `PENTER` | `0f 3a fc 03` | `R15=frontend`, `R13=TLS/state key` |
| `PSWITCH` | `0f 3a fc 04` | `RBX=target`, `R15=frontend` |
| `PLANDING` | `0f 3a fc 05` | indirect-call landing marker |
| `PCALL` | `0f 3a fc 2d` | `RBX=target`, `R11=return`, `R12=signature` |
| `PCALL_IMM` | `0f 3a fc 30..` | immediate signature slot |
| `PTRAPRET` | `0f 3a fc 62` | resume from trap packet |

AArch64 controls use reserved HINT encodings. RISC-V controls use custom-0
encodings. Hardware and ABI rationale belongs in
`docs/poly-isa-design-directions.md`.
