# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to x86_64. All frontends
share x86 virtual memory; x86_64 still owns boot, privilege, paging, faults,
interrupts, atomics, and the effective memory model. See
[poly-isa-design-directions.md](poly-isa-design-directions.md) for rationale.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign code fetches native 32-bit instructions directly from x86 memory.
- Foreign registers are XSAVE-style architectural state, not hidden state.
- Hardware handles frontend switches and register-only ABI signatures.
- Software handles stacks, aggregates, variadics, linking, syscalls, and libcalls.

## Controls

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: call another frontend with ABI signature `sig`.
- `PTRAPRET`: return from a precise Poly trap packet.
- `PLANDING`: validate an indirect cross-frontend landing pad.

Prototype encodings use x86 `0f 3a fc <subop>`, AArch64 reserved HINT, and
RISC-V custom-0. Real hardware should allocate decoded opcodes, not `#UD`.

## Traps And Returns

- Native returns can cross frontends via hardware transition stack and cookies.
- Recoverable foreign syscalls, breakpoints, unresolved imports, and unsupported
  instructions create OS-neutral Ring 3 trap packets.
- Hardware must not parse descriptors, repack stacks, marshal structs, implement
  libcalls, or emulate OS syscalls.
