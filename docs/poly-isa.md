# Poly ISA Reference

Poly is an x86_64 user-mode extension for running existing x86_64, AArch64, and
RISC-V64 code in one process. It is not a new source ABI and it does not emulate
an operating system inside the CPU.

## Frontends

| Mode | Frontend | Contract |
| --- | --- | --- |
| `0` | x86_64 | Privilege, paging, interrupts, faults, atomics, syscalls |
| `1` | AArch64 | Raw user-mode 32-bit fetch from `RIP`, 4-byte aligned |
| `2` | RISC-V64 | Raw user-mode 16/32-bit fetch from `RIP`, 2-byte aligned |

All modes share the same virtual address space and use the x86 TSO memory model.
Foreign-only architectural registers are per-thread state, intended to be saved
by an XSAVE-style OS mechanism or an equivalent runtime contract.

## Control Encodings

Temporary prototype encodings are decoded controls, not `#UD` traps:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   reserved HINT space
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  custom-0 space
```

Important subops:

| Subop | Name | Purpose |
| --- | --- | --- |
| `0x03` | `PENTER` | Enter a raw foreign frontend at the next instruction |
| `0x04` | `PSWITCH` | Branch to another frontend without call state |
| `0x05` | `PLANDING` | Mark a valid cross-frontend landing target |
| `0x2d` | `PCALL` | Cross-frontend call using the selected ABI signature |
| `0x30..0x3c` | `PCALL_SLOT` | Cross-frontend call using cached signature slot |
| `0x62` | `PTRAPRET` | Return from the Ring 3 trap monitor |
| `0x65..0x6e` | `STATE` | Configure/query Poly runtime state |

## Call And Return

`PCALL` records caller mode, PC, SP, and flags, installs a return cookie in the
callee's native return register, applies register-only ABI mapping, and branches
to the callee frontend. A native return to the cookie restores the caller mode.

Fast ABI signatures may rename argument/result registers in hardware. They must
not read user memory, repack stacks, repack aggregates, translate variadics, or
implement libcalls. Those cases belong in loader/runtime thunks.

## OS Boundary

The OS remains responsible for privilege, scheduling, hard faults, signals, and
real syscalls. Poly hardware only switches decode frontends, manages architectural
Poly state, and reports traps to a user-space monitor or normal OS exception
path. Syscall translation and library interposition are software policy.
