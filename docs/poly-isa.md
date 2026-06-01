# Poly ISA

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

Poly lets existing x86_64, AArch64, and RISC-V64 userspace code run in one
x86_64 virtual address space. The goal is precompiled-code compatibility, not a
new compiler-only ABI and not one trap per foreign instruction.

## Execution Model

- x86_64 stays the system ISA: boot, privilege, paging, faults, interrupts,
  VM control, atomics, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends in the same process address
  space, page tables, and stack memory.
- AArch64 fetch is direct aligned 32-bit fetch. RISC-V fetch supports 16-bit RVC
  and 32-bit instructions.
- ISA transitions use decoded Poly control instructions, not `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state.
- The runtime handles ABI shims, stack arguments, aggregates, variadics, lazy
  binding, syscall translation, and policy.

## Controls

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PLANDING` | Validate indirect cross-frontend targets when enabled. |
| `PTRAPRET` | Resume after a precise Poly trap. |

## Temporary Encodings

Current Bochs/test encodings. These are placeholders, not vendor opcode
allocations.

| ID | Frontend | Fetch | Test encoding |
| --- | --- | --- | --- |
| `0` | x86_64 | variable length | `0f 3a fc <subop>` |
| `1` | AArch64 | 32-bit aligned | `0xd503201f | (subop << 5)` |
| `2` | RISC-V64 | 16-bit RVC plus 32-bit | `0x0000700b | (subop << 25)` |

Constants live in `tools/include/polycpuid.h`.

Hardware and ABI rationale lives in `docs/poly-isa-design-directions.md`.
