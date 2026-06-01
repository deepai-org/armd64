# Poly ISA

Quick reference for the x86_64 extension that runs existing x86_64, AArch64,
and RISC-V64 code in one userspace address space. Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Difference From x86_64

- x86_64 still owns boot, privilege, paging, faults, interrupts, atomics, VM
  control, and global TSO memory ordering.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same virtual
  memory, not separate machines.
- Frontend switching uses decoded Poly control instructions, not exception
  envelopes.
- Non-x86 registers are explicit per-thread XSAVE-style state.
- Fast cross-ISA calls may use register-alias signature slots.
- Stack arguments, aggregates, variadics, syscalls, lazy binding, and policy
  remain software/runtime work.

## Frontends And Test Encodings

Temporary Bochs/test encodings; not vendor allocations.

| ID | Frontend | Fetch | Test encoding |
| --- | --- | --- | --- |
| `0` | x86_64 | variable length | `0f 3a fc <subop>` |
| `1` | AArch64 | 32-bit aligned | `0xd503201f | (subop << 5)` |
| `2` | RISC-V64 | 16-bit RVC plus 32-bit | `0x0000700b | (subop << 25)` |

Constants are in `tools/include/polycpuid.h`.
