# Poly ISA Quick Reference

Poly runs x86_64, AArch64, and RISC-V64 user code in one x86_64 virtual address
space. The goal is compatibility with existing compiled objects and cross-ISA
shared libraries.

## Run It

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- x86_64 still owns boot, privilege, paging, interrupts, exceptions, syscalls,
  atomics, and memory ordering.
- AArch64 and RISC-V64 are extra user-mode fetch/decode frontends over the same
  address space.
- Mode changes use explicit control opcodes. There are no per-instruction `#UD`
  envelopes.
- Cross-ISA calls keep native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Register-only calls can use ABI signature slots. Stack arguments, aggregates,
  variadics, lazy binding, libc policy, and syscall policy are software/runtime
  work.
- Foreign register state is per-thread XSAVE-style architectural state.
- Foreign traps create OS-neutral trap packets for a runtime or OS handler.

## Active x86 Controls

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

| Control | Encoding | Inputs |
| --- | --- | --- |
| `PENTER` | `0f 3a fc 03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `0f 3a fc 04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `0f 3a fc 05` | indirect-call landing marker |
| `PCALL` | `0f 3a fc 2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `0f 3a fc 30..` | `sig=op - 0x30` |
| `PTRAPRET` | `0f 3a fc 62` | resume from trap packet |

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings. Design
rationale is in `docs/poly-isa-design-directions.md`.
