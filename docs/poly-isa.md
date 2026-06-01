# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user frontends to an x86_64 machine. x86_64
remains the system ISA.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## What Changes From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 is variable length; AArch64 is 32-bit aligned; RISC-V64 is 2-byte aligned.
- System model: paging, interrupts, faults, VM control, atomics, and global ordering stay x86_64-owned.
- Memory model: one x86_64 virtual address space with x86 TSO.
- State model: foreign registers and frontend TLS are per-thread XSAVE-style state.
- Transitions: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are
  decoded control instructions, not `#UD` envelopes.
- Traps: foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce OS-neutral records.

## ABI Boundary

The target is precompiled x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI code.
Fast paths use register-only ABI signature slots; software thunks handle stack
arguments, aggregates, variadics, dynamic linking, libcalls, and syscall policy.

| Window | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0..P7` | `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` | `x0..x7` | `a0..a7` |
| `F0..F7` | `XMM0..XMM7` | `v0..v7` | `fa0..fa7` |

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.

Detailed rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
