# Poly ISA Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU. It
targets existing native ABI code, not a new source ABI.

## Contract

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64. x86_64 is the
system ISA: it owns privilege, paging, interrupts, faults, syscalls, atomics,
virtual memory, and global TSO ordering. Foreign frontends are user-mode fetch
frontends in the same virtual address space: AArch64 fetches aligned 32-bit
instructions, and RISC-V64 fetches aligned 16/32-bit instructions.

## Encodings

These are decoded control instructions, not `#UD` exception envelopes:

```text
x86_64   0f 3a fc <subop>
AArch64  0xd503201f | ((subop & 0x7f) << 5)
RISC-V   0x0000700b | ((subop & 0x7f) << 25)
```

Core subops: `0x03 PENTER`, `0x04 PSWITCH`, `0x05 PLANDING`, `0x2d PCALL`,
`0x30..0x3c PCALL_SLOT`, `0x62 PTRAPRET`, and `0x65..0x6e STATE`.

## Calls

`PCALL` records caller frontend, PC, SP, and flags; applies a register-only ABI
signature; installs a reserved native return cookie; and branches to the callee.
Ordinary native returns go through the cookie and restore the caller frontend.

Hardware may rename register arguments/results through cached ABI slots. It
must not parse user-memory descriptors, repack stacks or aggregates, translate
variadics, implement libcalls, or implement OS policy. Those cases use
loader/runtime thunks.

## State And Traps

Foreign register state is explicit per-thread XSAVE-style state. Recoverable
foreign syscalls, breakpoints, unresolved imports, and unsupported instructions
produce precise trap records for a runtime or OS handler.

Bochs implements this as a functional ISA prototype. Few-cycle switching is a
hardware/FPGA design target, not a Bochs timing claim. See
`docs/poly-isa-design-directions.md` for the hardware boundary rationale.
