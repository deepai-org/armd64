# Poly ISA Reference

Short contract for the Poly prototype. Design rationale lives in
`docs/poly-isa-design-directions.md`.

- x86_64 owns boot, privilege, paging, interrupts, atomics, syscalls, and TSO.
- AArch64 and RISC-V64 are ring-3 frontends over the same virtual memory.
- Foreign code is fetched directly: no per-instruction `#UD` envelopes.
- Foreign register state is per-thread, XSAVE-style architectural state.
- Compatibility targets existing SysV x86_64, AAPCS64, and RISC-V psABI code.

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

## Encodings

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

## Operations

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER mode` |
| `0x04` | `PSWITCH mode,target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL mode,target,signature` |
| `0x30..0x3c` | `PCALL` with immediate signature slot |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state key, ABI signature, monitor packet, landing policy |

Foreign frontends expose equivalent control instructions.

## Boundary Rules

`PCALL` records caller state in a hardware transition stack, installs a reserved
return cookie, applies a register-only ABI signature, and jumps to the target
frontend. Native returns to that cookie restore the caller frontend and PC.

ABI signatures never touch memory. Stack arguments, variadics, by-value
aggregates, lazy binding, libc policy, and syscall policy stay in
loader/runtime thunks or the ring-3 Poly monitor.

Recoverable foreign exits produce OS-neutral trap packets. The kernel still
owns scheduling, privilege transitions, signals, hard faults, and real syscalls.
