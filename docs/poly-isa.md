# Poly ISA

Poly adds AArch64 and RISC-V64 user frontends to an x86_64 system ISA for
existing SysV x86_64, AAPCS64, and RISC-V psABI objects. It is not a new
source-level ABI. Design rationale: `docs/poly-isa-design-directions.md`.

## Frontends

```text
0 x86_64    system frontend: privilege, paging, faults, syscalls, atomics
1 AArch64   user frontend: 32-bit aligned fetch
2 RISC-V64  user frontend: 16/32-bit fetch
```

All frontends share x86_64 virtual memory and TSO ordering. Foreign registers
are per-thread XSAVE-style architectural state.

## Encodings

Temporary decoded controls, not `#UD` envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  // custom-0

PENTER=0x03  PSWITCH=0x04  PLANDING=0x05  PCALL=0x2d
PCALL_SLOT=0x30..0x3c      PTRAPRET=0x62  STATE=0x65..0x6e
```

## Contract

- `PENTER` enters a frontend from trusted runtime code.
- `PSWITCH` switches frontend and branches without a return edge.
- `PCALL` saves mode/PC/SP/flags to the hardware transition stack, installs a
  return cookie, applies a register-only ABI signature, and jumps.
- Native returns to a return cookie restore the caller frontend; same-ISA
  returns stay native.
- Hardware only renames registers. Software handles stack args, aggregates,
  variadics, libcalls, lazy binding, and OS syscall policy.
- Recoverable foreign exits write OS-neutral trap packets for a Ring 3 monitor;
  kernels still own scheduling, privilege, hard faults, signals, interrupts, and
  real syscalls.
