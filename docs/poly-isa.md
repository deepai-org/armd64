# Poly ISA

Poly is an x86_64 CPU extension for running existing precompiled x86_64,
AArch64, and RISC-V64 userspace code in one virtual address space.

This file is the short reference. Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## How To Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same
  process address space.
- Fetch follows the active frontend: x86_64 variable-length, AArch64 32-bit
  aligned, RISC-V64 16-bit RVC plus 32-bit instructions.
- Cross-frontend control uses decoded Poly instructions: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`; no `#UD` instruction envelopes.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.
- Fast calls use hardware register-alias signature slots. Stack arguments,
  aggregates, variadics, and loader policy stay in software thunks.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and recoverable
  exits produce OS-neutral trap packets for a user-mode Poly monitor.

## Frontends

| ID | Frontend | Fetch |
| --- | --- | --- |
| `0` | x86_64 | variable-length |
| `1` | AArch64 | 32-bit aligned |
| `2` | RISC-V64 | 16-bit aligned RVC plus 32-bit |

## Prototype Control Encodings

These are prototype opcode pages for Bochs and tests, not an x86 vendor
allocation claim.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` reserved HINT subspace |
| RISC-V64 | `0x0000700b | (subop << 25)` custom-0 family |

Constants are in `tools/include/polycpuid.h`.

## Hardware Boundary

- No Linux, libc, libgcc, libatomic, or dynamic-linker policy in hardware.
- No user-memory call descriptor parsing in control instructions.
- No stack repacking, aggregate marshalling, or variadic argument handling in
  hardware.
