# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine so
existing native ABI objects can link and run in one virtual address space. The
goal is compatibility with precompiled code, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log

# Focused cross-ISA smoke test:
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, VM control, atomics, and memory ordering.
- AArch64 and RISC-V64 are user frontends over the same address space and
  shared `RIP`/PC stream.
- AArch64 fetches aligned 32-bit instructions. RISC-V64 fetches 16/32-bit
  instructions, including RVC.
- Cross-ISA control uses real decoded Poly instructions, not `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state. Current
  import layout version: `9`.
- Hardware switches frontends, branches, records precise trap packets, and
  applies register-only ABI signature slots. Software handles loading,
  relocations, syscalls, libcalls, stack arguments, variadics, aggregates, and
  incompatible vector layouts.

## Control Instructions

- `PENTER`: enter a foreign frontend from trusted runtime code.
- `PSWITCH`: switch frontend and branch without a return.
- `PCALL`: cross-ISA call using an ABI signature slot.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: validate indirect cross-frontend landing targets.

Native returns remain native. Cross-ISA returns use hardware return cookies.

## IDs And Encodings

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64: decoded `0f 3a fc <subop>`.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.

Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
