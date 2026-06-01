# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to x86_64 in one virtual
address space. The target is existing native ABI objects, not a new
compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots:

```sh
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
make boot-poly-binfmt-arch-traps
```

## Delta From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  exceptions, atomics, and global TSO memory ordering.
- Foreign frontends directly fetch native 32-bit instructions from `RIP`; no
  per-instruction `#UD` envelopes.
- Native ABIs stay native: x86_64 SysV, AArch64 AAPCS64, RISC-V psABI.
- Fast `PCALL`s use register-only ABI signature slots; software handles stack
  args, variadics, aggregate repacking, lazy binding, syscalls, and libc policy.
- Extra foreign registers are per-thread XSAVE-style state, not hidden
  CR3-scoped emulator state.

## x86 Control Encoding

Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64. Prototype controls use
`0f 3a fc <op>`; they are decoded controls, not fault-driven `UD2` envelopes.

| Operation | Opcode | Inputs |
| --- | --- | --- |
| `PENTER` | `03` | `R15=frontend`, `R13=TLS` |
| `PSWITCH` | `04` | `R15=frontend`, `RBX=target`, `R13=TLS` |
| `PLANDING` | `05` | marks an indirect cross-frontend target |
| `PCALL` | `2d` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig` |
| `PCALL_IMM` | `30..` | same as `PCALL`; signature slot is `op - 0x30` |
| `PTRAPRET` | `62` | resume from a Poly trap packet |

Longer rationale and hardware direction: `docs/poly-isa-design-directions.md`.
