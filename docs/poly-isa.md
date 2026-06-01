# Poly ISA Quick Reference

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

Poly runs existing precompiled x86_64, AArch64, and RISC-V64 userspace code in
one x86_64 virtual address space. It is not a new compiler-only ABI and not a
per-instruction trap scheme.

## Difference From x86_64

- x86_64 stays the system ISA: boot, privilege, paging, faults, interrupts,
  VM control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends sharing the same address space,
  page tables, stack memory, and OS process model.
- Foreign code is fetched directly: AArch64 as aligned 32-bit instructions;
  RISC-V as 16-bit/32-bit instructions so RVC stays valid.
- Frontend changes use decoded Poly control instructions, not `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state.
- Fast cross-ISA calls may use register-only ABI signature slots.
- Stack arguments, aggregates, variadics, lazy binding, syscall translation, and
  policy are loader/runtime work, not hardware descriptor parsing.

## Controls

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a precise Poly trap. |
| `PLANDING` | Validate indirect cross-frontend targets when enabled. |

## Temporary Encodings

Bochs/test encodings only. These are not vendor opcode allocations.

| ID | Frontend | Fetch | Test encoding |
| --- | --- | --- | --- |
| `0` | x86_64 | variable length | `0f 3a fc <subop>` |
| `1` | AArch64 | 32-bit aligned | `0xd503201f | (subop << 5)` |
| `2` | RISC-V64 | 16-bit RVC plus 32-bit | `0x0000700b | (subop << 25)` |

Constants: `tools/include/polycpuid.h`.

Longer hardware/ABI rationale: `docs/poly-isa-design-directions.md`.
