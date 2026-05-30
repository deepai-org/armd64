# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine.
x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts,
atomics, scheduling, syscalls, and TSO ordering stay x86-owned.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, and `boot-poly-neutral-arch-traps`.

## Architectural Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 keeps normal variable-length decode. AArch64 fetches aligned
  32-bit words from `RIP`. RISC-V fetches 16-bit or 32-bit native instructions
  from `RIP`, so compressed RVC code is legal.
- Memory: every frontend uses the same x86_64 virtual address space and page
  permissions.
- State: foreign GPR/FP/SIMD state is explicit XSAVE-style architectural state,
  not hidden emulator state.
- Calls: `PCALL` switches frontend and may apply a register-only ABI signature.
  Software thunks handle stack args, aggregates, variadics, lazy binding, and
  policy.
- Traps: foreign syscalls, breakpoints, unsupported instructions, and import
  misses create OS-neutral trap packets. Trap packets carry the first eight native foreign ABI argument registers.
  Hardware does not implement Linux, libc, libgcc, libatomic, or dynamic-linker
  behavior.

## Control Operations And Encodings

These are temporary Bochs prototype encodings, not final vendor allocations:
`PENTER` enters a frontend, `PSWITCH` branches across frontends, `PCALL` calls
across frontends, `PCALL_SIG_IMM` calls with an immediate signature slot,
`PTRAPRET` resumes from a Poly trap, and `PLANDING` marks or validates an
indirect cross-frontend target.

| Op | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| Base | `0f 3a fc <subop>` | `0xd503201f | (subop << 5)` | `0x0000700b | (subop << 25)` |
| `PENTER` | `0x03` | n/a | n/a |
| `PSWITCH` | `0x04` | `0x78` | `8` |
| `PCALL` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
