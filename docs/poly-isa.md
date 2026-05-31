# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine.
x86_64 stays the system ISA.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
```

## What Differs From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 uses variable-length x86 bytes; foreign modes fetch native
  32-bit instructions from the same virtual address space.
- Privilege: paging, interrupts, faults, VM control, atomics, and global memory
  ordering remain x86_64-owned.
- State: foreign registers are per-thread XSAVE-style architectural state, not
  hidden emulator state.
- Memory model: all frontends inherit x86 TSO.
- Transitions: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are real
  decoded control instructions, not `#UD` envelopes.
- ABI work: hardware may rename registers through ABI signature slots. Stack
  layout, aggregates, variadics, dynamic linking, libcalls, and syscall
  translation stay in software.

| Window | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

## Temporary Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `HINT`, encoded as `0xd503201f | (subop << 5)`
- RISC-V64: `custom-0`, encoded as `0x0000700b | (subop << 25)`

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
