# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA in
one virtual address space. The goal is compatibility with existing native ABI
code and cross-ISA dynamic libraries, not a new compiler-only ABI.

## Running

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-focused-validation
make boot-poly-binfmt-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract And x86_64 Differences

- x86_64 remains the system ISA: boot, privilege, paging, exceptions,
  interrupts, atomics, syscalls, and TSO memory ordering.
- AArch64/RISC-V64 are user-mode direct-fetch frontends. AArch64 fetches
  4-byte instructions; RISC-V fetches native 16/32-bit instructions from `RIP`.
  There is no per-instruction `#UD` envelope.
- New decoded controls switch frontends and perform cross-ISA calls; they are
  not traps and are not encoded as `ud2`.
- Native ABIs remain native: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Fast cross-ISA calls use register-only ABI signature slots. Stack arguments,
  variadics, aggregate repacking, lazy binding, syscall policy, and libc policy
  stay in software/runtime code.
- Extra foreign registers are per-thread XSAVE-style architectural state, not
  CR3-scoped hidden emulator state.
- Foreign traps are reported through Poly trap packets; OS-specific behavior is
  runtime policy, not ISA behavior.

## Bochs Control Opcodes

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64. x86_64 controls use
decoded `0f 3a fc <op>` instructions. Foreign controls use reserved AArch64
HINT and RISC-V custom-0 encodings.

| Op | Opcode | Inputs |
| --- | --- | --- |
| `PENTER` | `03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `05` | indirect cross-frontend landing marker |
| `PCALL` | `2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `30..` | `R15=frontend`, `RBX=target`, `R11=return`, slot `op - 0x30` |
| `PTRAPRET` | `62` | resume from a Poly trap packet |

Full rationale: `docs/poly-isa-design-directions.md`.
