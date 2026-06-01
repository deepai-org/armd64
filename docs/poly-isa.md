# Poly ISA

Poly is an x86_64 CPU extension for running existing x86_64, AArch64, and
RISC-V64 native ABI code in one process.

## Run It

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode decode frontends over the same virtual
  address space, page permissions, stack memory, and process model.
- Poly transitions are real decoded control operations, not `#UD` envelopes.
- Foreign registers and Poly control state are per-thread XSAVE-style
  architectural state.
- Hardware owns frontend switching, register-only ABI signatures, precise trap
  packets, native return-cookie recovery, and XSAVE state.
- Runtime software owns stack arguments, aggregates, variadics, lazy binding,
  syscall policy, helper calls, and other memory-shaped ABI work.

## ISA Operations

| Operation | Role |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PLANDING` | Validate indirect cross-ISA targets when enabled. |
| `PTRAPRET` | Resume after a precise Poly trap. |

Current Bochs/test encodings are temporary; constants live in
`tools/include/polycpuid.h`. Design rationale lives in
`docs/poly-isa-design-directions.md`.
