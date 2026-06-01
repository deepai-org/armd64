# Poly ISA Reference

Compact software contract for the Poly prototype. Design rationale lives in
`docs/poly-isa-design-directions.md`.

## Model

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86_64 owns boot, privilege, paging, interrupts, atomics, syscalls, and TSO.
- AArch64/RISC-V64 are user-mode frontends in the same virtual address space.
- Foreign code is direct-fetched: no per-instruction `#UD` envelopes.
- Fetch rules are native: x86 variable length, AArch64 32-bit, RISC-V 16/32-bit.
- Foreign state is per-thread XSAVE-style architectural state.
- Compatibility targets ordinary SysV x86_64, AAPCS64, and RISC-V psABI code.

## Control Encoding

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

Prototype allocations for a decoded control family, not an exception path.

## Control Ops

| Control | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted code. |
| `PSWITCH frontend,target` | Branch to another frontend. |
| `PCALL frontend,target,sig` | Cross-frontend call. |
| `PCALL frontend,target,#slot` | Compact call using immediate signature slot. |
| `PLANDING` | Validate indirect cross-frontend targets. |
| `PTRAPRET` | Return from a precise trap packet. |
| state ops | State key, ABI signature, monitor packet, landing policy. |

Current x86 subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`, generic
`PCALL=0x2d`, immediate-slot `PCALL=0x30..0x3c`, `PTRAPRET=0x62`, state/control
`0x65..0x6e`. Foreign frontends expose equivalent controls.

## Boundary Rules

- `PCALL` is register-only in hardware: push caller frontend/PC/SP/flags to the
  hardware transition stack, install a return cookie, apply the ABI signature by
  register aliasing/renaming, then jump.
- Native returns to the cookie restore the caller. Same-ISA returns stay native.
- ABI signatures never parse descriptors or touch memory. Stack arguments,
  variadics, by-value aggregates, lazy binding, libc policy, syscall
  translation, and other memory-shaped ABI work stay in software.
- Recoverable foreign exits produce OS-neutral trap packets. The monitor may
  handle syscall translation, import binding, helper calls, and debugger policy.
  The kernel still owns scheduling, privilege transitions, hard faults, signals,
  interrupts, and real syscalls issued by the monitor.
