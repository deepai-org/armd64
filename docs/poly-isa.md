# Poly ISA

Poly is an x86_64 CPU extension for running user-mode x86_64, AArch64, and
RISC-V64 code in one virtual address space.

Longer design notes live in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Architecture

- x86_64 is the system ISA: boot, privilege, paging, interrupts, syscalls,
  atomics, and TSO stay x86-owned.
- AArch64 and RISC-V64 are additional user-mode frontends.
- All frontends share the x86 virtual address space and page tables.
- Foreign frontends fetch native 32-bit instructions directly from `RIP`.
- Foreign modes inherit x86 TSO; AArch64/RISC-V barriers are compatibility
  instructions, not a weaker memory model.
- Foreign register state is architectural XSAVE-style state, not hidden
  CR3-keyed emulator state.

## Frontend IDs

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

## Control

Prototype encodings are in `tools/include/polycpuid.h`; silicon needs real
vendor-allocated decoded opcodes, not `#UD` envelopes.

| Instruction | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Tail-branch to another frontend. |
| `PCALL frontend, target, sig` | Cross-frontend call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a precise Poly trap. |
| `PLANDING` | Validate an indirect cross-frontend target when enabled. |

## ABI Boundary

Poly targets existing precompiled code, not a new source ABI.

- Register-only calls use `PCALL` plus cached ABI signature slots. Hardware may
  remap argument and return registers through the rename/alias machinery.
- Stack arguments, by-value aggregates, variadics, vector mismatches, lazy
  binding, and libc policy are software-runtime work.
- Native return instructions remain valid. Cross-ISA returns use a hardware
  transition stack plus reserved return cookies.

## Traps

Foreign syscalls, breakpoints, illegal instructions, unsupported instructions,
unresolved imports, and recoverable exits produce OS-neutral trap packets. A
Ring 3 Poly monitor may handle them. The kernel still owns hard page faults,
interrupts, scheduling, signals, and real syscalls issued by the monitor.

## Non-Goals

- No legacy single-instruction `#UD` envelopes.
- No hidden synthetic register banks keyed only by `CR3`.
- No hardware parsing of user-memory call descriptors.
- No hardware stack repacking, struct marshalling, libcalls, or Linux-specific
  syscall emulation.
