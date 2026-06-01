# Poly ISA

Short reference for the Bochs prototype. Longer hardware/ABI rationale is in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Goal

Run existing precompiled x86_64, AArch64, and RISC-V64 userspace code in one
x86_64 virtual address space. This is not a new compiler-only ABI and not a
per-instruction trap scheme.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, VM control, and global TSO memory ordering.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same
  address space and page tables.
- Foreign instructions are fetched directly: AArch64 at 4-byte alignment,
  RISC-V at 2-byte alignment so RVC remains valid.
- Frontend changes use decoded Poly control instructions, not `#UD` envelopes.
- Foreign architectural state is explicit per-thread XSAVE-style state.
- Fast cross-ISA calls use register-only ABI signature slots where possible.
- Stack arguments, aggregates, variadics, lazy binding, syscall translation,
  and policy stay in loader/runtime software.

## Controls

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a precise Poly trap. |
| `PLANDING` | Validate indirect cross-frontend targets when enabled. |

## Prototype Encodings

Temporary Bochs/test encodings; not vendor opcode allocations.

| ID | Frontend | Fetch | Test encoding |
| --- | --- | --- | --- |
| `0` | x86_64 | variable length | `0f 3a fc <subop>` |
| `1` | AArch64 | 32-bit aligned | `0xd503201f | (subop << 5)` |
| `2` | RISC-V64 | 16-bit RVC plus 32-bit | `0x0000700b | (subop << 25)` |

Constants are in `tools/include/polycpuid.h`.
