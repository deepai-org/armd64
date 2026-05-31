# Poly ISA

Poly is an x86_64-compatible CPU extension that can execute user-mode AArch64
and RISC-V64 code in the same virtual address space.

This file is the short reference. Longer rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- x86_64 remains the system ISA.
- AArch64 and RISC-V64 are added as user-mode instruction frontends.
- Foreign frontends fetch native 32-bit instructions directly from `RIP`.
- Cross-ISA transfers are explicit decoded control instructions, not `#UD`
  exception envelopes.
- Foreign architectural state is explicit XSAVE-style state so the OS can save
  and restore it without understanding AArch64 or RISC-V semantics.

## What Stays x86_64

- x86_64 owns boot, paging, privilege, interrupts, atomics, syscalls, and TSO.
- Foreign memory accesses use the same virtual address space and page tables.
- Foreign modes inherit x86_64 TSO. AArch64/RISC-V barriers are compatibility
  points, not permission to weaken memory ordering.

## Frontends

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

## Control Instructions

Prototype encodings are in `tools/include/polycpuid.h`. Real silicon should use
vendor-allocated decoded opcodes.

| Instruction | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a precise Poly trap. |
| `PLANDING` | Validate an indirect cross-frontend target when enabled. |

## Calls And ABI

The ISA aims to run existing precompiled code, not define a new source-level
ABI. Hardware provides fast register-only transitions; software handles cases
that require memory layout changes.

- Fast path: `PCALL` applies a cached register alias signature and switches
  frontend. This is suitable for ordinary functions whose arguments and return
  values fit in registers.
- Slow path: loader/runtime thunks handle stack arguments, by-value aggregates,
  variadics, incompatible vectors, lazy binding, and libc policy.
- Returns use ordinary native return instructions. Cross-ISA returns are
  recovered through a hardware transition stack and reserved return cookies.

## Traps

Foreign syscalls, breakpoints, illegal or unsupported instructions, unresolved
imports, and recoverable frontend exits produce OS-neutral trap packets. A
Ring 3 Poly monitor may handle recoverable events. The kernel still owns hard
page faults, interrupts, scheduling, signals, and real syscalls issued by the
monitor.

## Non-Goals

- No hidden CR3-keyed synthetic register banks.
- No hardware parsing of user-memory call descriptors.
- No hardware stack repacking, struct marshalling, libcalls, or Linux-specific
  syscall emulation.
- No legacy single-instruction `#UD` envelopes.
