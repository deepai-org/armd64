# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to x86_64 in one virtual
address space. The goal is existing native ABI compatibility, including
cross-ISA dynamic libraries.

## Running

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
make boot-poly-binfmt-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Model

- x86_64 remains the system ISA for boot, privilege, paging, exceptions,
  interrupts, atomics, and TSO memory ordering.
- AArch64/RISC-V64 are direct-fetch user frontends. They fetch native 32-bit
  instructions from `RIP`; there is no per-instruction `#UD` envelope.
- Native ABIs remain native: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Fast cross-ISA calls use register-only ABI signature slots. Stack arguments,
  variadics, aggregate repacking, syscalls, libc policy, and lazy binding stay
  in software.
- Extra foreign registers are per-thread XSAVE-style architectural state, not
  CR3-scoped hidden emulator state.

## Prototype Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64. Bochs prototype controls
use decoded `0f 3a fc <op>` instructions.

| Op | Opcode | Inputs |
| --- | --- | --- |
| `PENTER` | `03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `05` | indirect cross-frontend landing marker |
| `PCALL` | `2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `30..` | `PCALL` with signature slot `op - 0x30` |
| `PTRAPRET` | `62` | resume from a Poly trap packet |

Design rationale: `docs/poly-isa-design-directions.md`.
