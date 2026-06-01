# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
virtual address space. The goal is compatibility with existing native ABI
objects and cross-ISA shared libraries, not a new source-level ABI.

## Run

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  exceptions, syscalls, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are alternate user-mode instruction frontends fetched
  from the same `RIP` address space.
- AArch64 fetches fixed 32-bit instructions. RISC-V fetches 16/32-bit
  instructions, so RVC is valid.
- Mode switches are explicit control instructions, not per-instruction `#UD`
  envelopes.
- Cross-ISA calls preserve real platform ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast calls use register-only ABI signature slots. Stack arguments,
  aggregates, variadics, lazy binding, libc policy, and syscall policy remain
  software responsibilities.
- Extra foreign register state is per-thread XSAVE-style architectural state,
  not a hidden CR3-scoped emulator table.
- Foreign traps write OS-neutral trap packets for user runtime or OS policy.

## Control Encodings

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

| Control | x86 encoding | Inputs |
| --- | --- | --- |
| `PENTER` | `0f 3a fc 03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `0f 3a fc 04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `0f 3a fc 05` | landing marker for indirect cross-ISA targets |
| `PCALL` | `0f 3a fc 2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `0f 3a fc 30..` | `R15=frontend`, `RBX=target`, `R11=return`; `sig=op - 0x30` |
| `PTRAPRET` | `0f 3a fc 62` | resume from a Poly trap packet |

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings.
Detailed hardware direction lives in `docs/poly-isa-design-directions.md`.
