# Poly ISA

Poly lets one x86_64 process run existing x86_64, AArch64, and RISC-V64 user
objects. It is not a new compiler ABI. See
`docs/poly-isa-design-directions.md` for rationale.

## Frontends
```text
0 x86_64    system frontend: privilege, paging, faults, syscalls, atomics
1 AArch64   user frontend: 32-bit aligned fetch
2 RISC-V64  user frontend: 16/32-bit fetch
```

All frontends share x86_64 virtual memory and x86 TSO ordering. Non-x86 state
is per-thread XSAVE-style architectural state.

## Controls
```text
Temporary decoded controls, not #UD envelopes:

x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   reserved HINT
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  custom-0

0x03 PENTER     0x04 PSWITCH    0x05 PLANDING
0x2d PCALL      0x30..0x3c PCALL_SLOT
0x62 PTRAPRET   0x65..0x6e STATE
```

## Contract
- Native fetch/decode only; no per-instruction envelopes.
- `PENTER` enters a frontend. `PSWITCH` switches and branches without return.
- `PCALL` saves caller mode/PC/SP/flags, installs a return cookie, applies
  register-only ABI remapping, and jumps.
- Native returns to a return cookie restore the caller frontend.
- Hardware may rename registers, but must not parse user ABI descriptors or
  repack stack layouts.
- Software handles stack args, aggregates, variadics, lazy binding, libcalls,
  and syscall policy through thunks or a Ring 3 monitor.
- The kernel still owns scheduling, privilege, interrupts, hard faults, signals,
  and real syscalls.
