# Poly ISA

Poly is an x86_64 ISA extension for running user-mode AArch64 and RISC-V64
code in the same virtual address space as x86_64 code. The target workload is
existing native ABI objects and cross-ISA shared libraries, not a new compiler
ABI.

## Run

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 remains the system ISA: boot, privilege, paging, interrupts,
  exceptions, syscalls, atomics, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends fetched directly from `RIP`.
- AArch64 fetch is fixed 4-byte. RISC-V fetch is 16/32-bit so RVC is legal.
- There are no per-instruction `#UD` envelopes.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only ABI signature slots are the fast path. Stack arguments,
  variadics, aggregate reshaping, lazy binding, libc policy, and syscall policy
  stay in software.
- Extra foreign state is per-thread XSAVE-style architectural state, not a
  hidden CR3-scoped emulator table.
- Foreign traps produce OS-neutral trap packets for runtime or OS policy.

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

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings.
Detailed hardware direction lives in `docs/poly-isa-design-directions.md`.
