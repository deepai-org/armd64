# Poly ISA Reference

Poly adds peer user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA.
It targets existing SysV x86_64, AAPCS64, and RISC-V psABI objects. Design
rationale lives in `docs/poly-isa-design-directions.md`.

## Frontends

`0` is x86_64 variable-length system fetch. `1` is AArch64 32-bit aligned user
fetch. `2` is RISC-V64 16/32-bit user fetch.

x86_64 owns privilege, paging, faults, interrupts, syscalls, atomics, and the
global TSO memory model.

## Control Encodings

Temporary decoded control instructions, not `#UD` envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

```text
PENTER=0x03  PSWITCH=0x04  PLANDING=0x05  PCALL=0x2d
PCALL_SLOT=0x30..0x3c      PTRAPRET=0x62  STATE=0x65..0x6e
```

## Architectural Rules

- Foreign state is per-thread XSAVE-style architectural state.
- `PENTER` enters a frontend from trusted runtime/system code.
- `PSWITCH` switches frontend and branches without creating a return edge.
- `PCALL` saves caller mode/PC/SP/flags to the hardware transition stack,
  installs a return cookie, applies a register-only ABI signature, and jumps.
- Native returns to a return cookie restore the caller frontend; same-ISA
  returns remain ordinary native returns.
- ABI signatures may only rename/alias registers. Hardware does not parse user
  descriptors, marshal stack arguments, translate aggregates, bind libcalls, or
  implement OS syscall policy.
- Recoverable foreign exits write OS-neutral trap packets for a Ring 3 monitor.
  Kernels still own scheduling, privilege transitions, hard faults, signals,
  interrupts, and real syscalls issued by the monitor.
