# Poly ISA Quick Reference

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends. x86_64 stays
the system ISA for boot, privilege, paging, faults, interrupts, atomics, and the
memory model. Foreign code fetches native 32-bit instructions from the same
virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- State: foreign registers are XSAVE-style architectural state.
- Hardware: frontend switches, cross-frontend returns, register ABI signatures.
- Software: stacks, aggregates, variadics, linking, syscalls, libcalls.

## Controls

- `PENTER frontend`: enter a frontend.
- `PSWITCH frontend, target`: tail-branch across frontends.
- `PCALL frontend, target, sig`: call with register ABI signature `sig`.
- `PTRAPRET`: return from a Poly trap packet.
- `PLANDING`: validate an indirect cross-frontend landing pad.

Prototype encodings use x86 `0f 3a fc <subop>`, AArch64 reserved HINT, and RISC-V custom-0. Real hardware should allocate decoded opcodes, not `#UD`.

## Boundary

- Recoverable syscalls, breakpoints, imports, and unsupported instructions create
  OS-neutral Ring 3 trap packets.
- Native returns use hardware transition-stack cookies.
- Hardware does not parse descriptors, repack stacks, marshal structs, implement libcalls, or emulate OS syscalls.

See [poly-isa-design-directions.md](poly-isa-design-directions.md) for rationale.
