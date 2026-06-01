# Poly ISA

Poly is a multi-frontend extension for running existing x86_64, AArch64, and
RISC-V64 user code in one x86_64 system address space.

For rationale and hardware direction, see `docs/poly-isa-design-directions.md`.

## System Model

- `0`: x86_64, variable-length fetch, system ISA.
- `1`: AArch64, 32-bit fixed fetch, user frontend.
- `2`: RISC-V64, 16/32-bit fetch, user frontend.

x86_64 owns boot, privilege, paging, faults, interrupts, atomics, syscalls, and
the global TSO memory model. Foreign frontends execute ordinary native user
instructions and target ordinary SysV x86_64, AAPCS64, and RISC-V psABI code.

## Temporary Encodings

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

These are decoded control instructions. They are not `#UD` envelopes.

Current x86 subops are `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`,
`PCALL=0x2d`, compact slot `PCALL=0x30..0x3c`, `PTRAPRET=0x62`, and state
controls `0x65..0x6e`.

## Boundary Rules

- Foreign architectural state is per-thread XSAVE-style state.
- `PENTER` enters a frontend from trusted runtime/system code.
- `PSWITCH` branches to another frontend without return.
- `PCALL` is register-only in hardware: it saves caller frontend/PC/SP/flags to
  the hardware transition stack, installs a return cookie, applies an ABI
  signature by register aliasing/renaming, and jumps.
- Native returns to the cookie restore the caller frontend. Same-ISA returns
  stay native.
- ABI signatures never read descriptors or touch memory. Stack arguments,
  variadics, aggregates, lazy binding, libc policy, and syscall translation stay
  in software.
- Recoverable foreign exits produce OS-neutral trap packets for a user monitor.
  The kernel still owns scheduling, privilege transitions, hard faults, signals,
  interrupts, and real syscalls issued by the monitor.
