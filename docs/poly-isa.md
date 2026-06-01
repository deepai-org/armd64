# Poly ISA

Current Bochs prototype quick reference. See `README.md` for run commands and
`docs/poly-isa-design-directions.md` for rationale.

## Contract

- Goal: run precompiled x86_64, AArch64, and RISC-V64 user code in one VA.
- x86_64 is the system ISA for privilege, paging, faults, interrupts, syscalls,
  atomics, VM control, and TSO.
- AArch64/RISC-V64 are user decode frontends using their native ABIs.
- Foreign state is explicit per-thread XSAVE-style state.
- Hardware switches frontends and aliases register lanes; software owns stack
  layout, aggregates, variadics, lazy binding, libc, and syscall policy.

## Controls

Decoded controls replace `#UD` envelopes.

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Key subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, `PCALL=0x2d`,
`PCALL_SLOT=0x30..0x3c`, `PTRAPRET=0x62`, setup `0x65..0x6e`.
