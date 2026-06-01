# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to x86_64 in one virtual
address space. The target is compatibility with existing compiled objects and
cross-ISA shared libraries, not a new source ABI.

## Run

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Architecture

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  exceptions, syscalls, atomics, and memory ordering.
- AArch64 and RISC-V64 are user-mode decode frontends fetched from the same
  `RIP` address space.
- AArch64 uses fixed 32-bit fetch. RISC-V64 supports 16/32-bit fetch, including
  RVC.
- Mode switches use explicit control opcodes, not per-instruction `#UD`
  envelopes.
- Cross-ISA calls preserve native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast register-only calls may use ABI signature slots. Stack arguments,
  aggregates, variadics, lazy binding, libc policy, and syscall policy stay in
  software.
- Foreign architectural state is per-thread XSAVE-style state. It is not a
  hidden CR3-scoped emulator table.
- Foreign traps produce OS-neutral trap packets for a runtime or OS handler.

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
Hardware and ABI rationale lives in `docs/poly-isa-design-directions.md`.
