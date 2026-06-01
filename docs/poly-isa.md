# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user code share one x86_64
process. It is a hardware/Bochs ISA extension, not a new source-level ABI.

## Frontends

- `0`: x86_64. Owns privilege, paging, interrupts, faults, syscalls, atomics.
- `1`: AArch64. User-mode raw 32-bit fetch from `RIP`.
- `2`: RISC-V64. User-mode raw 16/32-bit fetch from `RIP`.
- All frontends use the same virtual address space and x86 TSO memory model.
- Non-aliased foreign registers are architectural per-thread state, suitable for
  XSAVE-style OS save/restore.

## Controls

Temporary encodings use decoded control opcodes, not `#UD` traps:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)   reserved HINT
RISC-V   0x0000700b | ((subop & 0x7f) << 25)  custom-0
```

Key subops: `PENTER 0x03`, `PSWITCH 0x04`, `PLANDING 0x05`, `PCALL 0x2d`,
`PCALL_SLOT 0x30..0x3c`, `PTRAPRET 0x62`, `STATE 0x65..0x6e`.

## Difference From x86_64

- Fetch/decode can switch to raw AArch64 or RISC-V; no per-instruction envelope.
- `PCALL` records caller frontend/PC/SP/flags, installs a return cookie, applies
  register-only ABI mapping, and branches to the callee frontend.
- Returning to the cookie restores the caller frontend.
- Hardware may rename registers for ABI slots; it must not parse user memory,
  repack stacks, repack aggregates, translate variadics, or implement libcalls.
- Software thunks or a Ring 3 monitor handle complex ABI cases and syscall
  policy. The OS still owns scheduling, privilege, interrupts, hard faults,
  signals, and real syscalls.
