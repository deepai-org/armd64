# Poly ISA Quick Reference

Poly is an x86_64 system ISA extension that lets user-mode AArch64 and
RISC-V64 code execute in the same virtual address space. It is aimed at
existing native ABI objects and cross-ISA shared libraries.

## Run

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- x86_64 still owns boot, privilege, paging, interrupts, exceptions, syscalls,
  atomics, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends fetched directly from `RIP`;
  there is no per-instruction `#UD` envelope.
- AArch64 fetch is fixed 4-byte. RISC-V fetch is 16/32-bit so RVC is legal.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, RISC-V psABI.
- Fast calls use register-only ABI signature slots. Stack/variadic/aggregate
  reshaping, lazy binding, libc policy, and syscall policy stay in software.
- Extra foreign state is per-thread XSAVE-style architectural state.
- Foreign traps produce OS-neutral trap packets for the runtime or OS policy.

## Control Encodings

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Control | x86 encoding | Inputs |
| --- | --- | --- |
| `PENTER` | `0f 3a fc 03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `0f 3a fc 04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `0f 3a fc 05` | landing marker for indirect cross-ISA targets |
| `PCALL` | `0f 3a fc 2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `0f 3a fc 30..` | same as `PCALL`; signature slot is `op - 0x30` |
| `PTRAPRET` | `0f 3a fc 62` | resume from a Poly trap packet |

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings. See
`docs/poly-isa-design-directions.md` for rationale and hardware direction.
