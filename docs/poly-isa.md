# Poly ISA

Poly adds direct-fetch AArch64 and RISC-V64 user frontends to an x86_64 system
ISA. x86_64 still owns boot, privilege, paging, interrupts, atomics, syscalls,
and the global TSO memory model. Compatibility targets ordinary SysV x86_64,
AAPCS64, and RISC-V psABI objects.

| ID | Frontend | Fetch |
| --- | --- | --- |
| `0` | x86_64 | variable length |
| `1` | AArch64 | 32-bit fixed |
| `2` | RISC-V64 | 16/32-bit |

## Encodings

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

These are decoded control instructions, not `#UD`/trap envelopes.

## Controls

- `PENTER frontend`: enter a frontend from trusted code.
- `PSWITCH frontend,target`: branch to another frontend.
- `PCALL frontend,target,sig`: cross-frontend call.
- `PCALL frontend,target,#slot`: compact call using an ABI signature slot.
- `PLANDING`: validate indirect cross-frontend targets.
- `PTRAPRET`: return from a precise trap packet.
- State ops: state key, ABI signature, monitor packet, landing policy.

Current x86 subops are `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`,
`PCALL=0x2d`, slot `PCALL=0x30..0x3c`, `PTRAPRET=0x62`, and state/control
`0x65..0x6e`.

## Boundary Contract

- Foreign architectural state is per-thread XSAVE-style state.
- `PCALL` is register-only in hardware: save caller frontend/PC/SP/flags to the
  hardware transition stack, install a return cookie, apply the ABI signature by
  register aliasing/renaming, and jump.
- Native returns to the cookie restore the caller; same-ISA returns stay native.
- ABI signatures never read descriptors or touch memory. Stack arguments,
  variadics, aggregates, lazy binding, libc policy, and syscall translation stay
  in software.
- Recoverable foreign exits produce OS-neutral trap packets for a user monitor.
  The kernel still owns scheduling, privilege transitions, hard faults, signals,
  interrupts, and real syscalls issued by the monitor.
