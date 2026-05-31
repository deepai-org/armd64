# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so existing
native ABI objects can link and run in one virtual address space. It is not a
new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log

# Focused cross-ISA execution test:
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
  atomics, VM control, and TSO ordering.
- AArch64 and RISC-V64 are user frontends over the same address space and
  `RIP`/PC stream.
- AArch64 fetch is aligned 32-bit; RISC-V fetch supports 16/32-bit instructions
  including RVC.
- Cross-ISA control uses decoded Poly instructions, not `#UD` envelopes.
- Foreign state is explicit per-thread XSAVE-style state. Import layout: `9`.
- Hardware only switches frontends and applies register-only ABI signature
  slots. Software handles loading, relocations, syscalls, libcalls, stack
  arguments, aggregates, variadics, incompatible vectors, and thunks.

## Control Instructions

- `PENTER`: enter a foreign frontend from trusted runtime code.
- `PSWITCH`: switch frontend and branch without a return.
- `PCALL`: cross-ISA call using an ABI signature slot.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: validate indirect cross-frontend landing targets.
- Native returns remain native; cross-ISA returns use hardware return cookies.

## Encodings

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64: temporary decoded `0f 3a fc <subop>`.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.

See [poly-isa-design-directions.md](poly-isa-design-directions.md) for design
rationale.
