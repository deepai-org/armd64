# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. x86_64
remains authoritative for privilege, paging, interrupts, faults, syscalls, VM
control, atomics, and TSO memory ordering.

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 uses native x86 bytes; foreign frontends fetch native 32-bit
  instructions from `RIP`.
- State: no per-instruction `#UD` envelopes; non-aliased foreign registers are
  per-thread XSAVE-style architectural state.
- Controls: `PENTER frontend`, `PSWITCH frontend,target`,
  `PCALL frontend,target,sig`, `PTRAPRET`, and `PLANDING`.
- Calls: `PCALL` is fixed-latency and register-only. Software handles stack
  arguments, aggregate layout, variadics, dynamic linking, libcalls, and syscall
  translation.

## ABI Window

- Integer: `P0..P7` maps to x86_64 `RAX,RDX,RCX,RDI,RSI,R8,R9,R10`, AArch64
  `x0..x7`, and RISC-V64 `a0..a7`.
- Floating point: `F0..F7` maps to x86_64 `XMM0..XMM7`, AArch64 `v0..v7`, and
  RISC-V64 `fa0..fa7`.
- ABI signature slots may remap this window at call boundaries. Hardware does
  not translate memory or stack layouts.

## Temporary Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `HINT`, encoded as `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, encoded as `0x0000700b | (subop << 25)`

Rationale and open design notes: [poly-isa-design-directions.md](poly-isa-design-directions.md).
