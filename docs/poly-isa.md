# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine.
x86_64 remains authoritative for privilege, paging, interrupts, faults,
syscalls, VM control, atomics, and memory ordering.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
```

## ISA Differences

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 decodes x86 bytes; foreign frontends fetch native 32-bit
  instructions from the shared PC.
- State: no per-instruction `#UD` envelopes. Non-aliased foreign registers are
  per-thread XSAVE-style state.
- Memory: foreign modes use the x86_64 address space and inherit x86 TSO.
- Controls: `PENTER frontend`, `PSWITCH frontend,target`,
  `PCALL frontend,target,sig`, `PTRAPRET`, and `PLANDING`.
- Calls: `PCALL` is a fixed-latency mode-switch branch. ABI signature slots may
  remap registers, but hardware does not translate memory or stack layouts.
- Traps: foreign traps produce runtime trap packets. The OS still handles real
  faults and interrupts.

| Window | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

Stack arguments, aggregates, variadics, libcalls, dynamic linking, and syscall
translation stay in software thunks/runtime code.

## Temporary Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `HINT`, encoded as `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, encoded as `0x0000700b | (subop << 25)`

Rationale and open design notes live in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
