# Poly ISA

Poly is an x86_64 CPU extension for running existing precompiled x86_64,
AArch64, and RISC-V64 userspace code in one address space.

This file is the short reference. Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## How To Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## What Differs From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode frontends over the same process memory.
- Fetch is frontend-specific: x86_64 variable-length, AArch64 32-bit aligned,
  and RISC-V64 16-bit RVC plus 32-bit instructions.
- Cross-frontend control uses decoded Poly instructions, not `#UD` envelopes.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.
- Fast calls may use hardware register-alias signature slots. Stack arguments,
  aggregates, variadics, syscall translation, and loader policy stay in
  software.

## Frontends

| ID | Frontend | Fetch |
| --- | --- | --- |
| `0` | x86_64 | variable-length |
| `1` | AArch64 | 32-bit aligned |
| `2` | RISC-V64 | 16-bit aligned RVC plus 32-bit |

## Prototype Control Encodings

Prototype opcode pages for Bochs and tests. They are not vendor allocations.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` reserved HINT subspace |
| RISC-V64 | `0x0000700b | (subop << 25)` custom-0 family |

Constants are in `tools/include/polycpuid.h`.
