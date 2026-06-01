# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 user objects in one x86_64
process. It is not a new compiler ABI.

## Model

- `0 x86_64`: system frontend for privilege, paging, faults, syscalls, atomics.
- `1 AArch64`: user frontend with 32-bit aligned fetch.
- `2 RISC-V64`: user frontend with 16/32-bit fetch.
- Shared state: x86_64 address space, x86 TSO, per-thread XSAVE-style foreign
  registers.

## Controls

Temporary decoded controls, not `#UD` envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   reserved HINT
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  custom-0

0x03 PENTER     0x04 PSWITCH    0x05 PLANDING
0x2d PCALL      0x30..0x3c PCALL_SLOT
0x62 PTRAPRET   0x65..0x6e STATE
```

## Rules

- Fetch/decode is native to the active frontend; no per-instruction wrapping.
- `PCALL` records caller mode/PC/SP/flags, installs a return cookie, applies
  register-only ABI remapping, and jumps.
- Returning to a hardware cookie restores the caller frontend.
- Hardware may rename registers, but must not parse user ABI descriptors, repack
  stacks, repack aggregates, or handle variadics.
- Software thunks or a Ring 3 monitor handle stack args, lazy binding, libcalls,
  and syscall policy.
- The OS still owns scheduling, privilege, interrupts, hard faults, signals, and
  real syscalls.
