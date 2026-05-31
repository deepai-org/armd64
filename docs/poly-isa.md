# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64
frontends. The goal is to run existing native ABI objects in one virtual
address space, not to define a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log

make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 is still the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user frontends fetched from the same `RIP` address
  space.
- AArch64 fetch is aligned 32-bit. RISC-V fetch supports native 16/32-bit
  instruction lengths, including RVC.
- Cross-ISA execution uses decoded Poly control instructions. There are no
  per-instruction `#UD` envelopes.
- Foreign architectural state is explicit per-thread XSAVE-style state. Current
  state import layout version: `9`.
- Hardware handles frontend switching and register-only ABI signature slots.
  Software handles loading, relocations, syscalls, libcalls, stack arguments,
  aggregates, variadics, incompatible vectors, and thunks.

## Control Instructions

- `PENTER frontend`: enter AArch64 or RISC-V64 from trusted runtime code.
- `PSWITCH frontend, target`: switch frontend and branch without a return.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: validate indirect cross-frontend landing targets.
- Native returns stay native. Cross-ISA returns use reserved hardware return
  cookies.

## Prototype Encodings

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 control page: temporary decoded `0f 3a fc <subop>`.
- AArch64 control page: reserved HINT subspace.
- RISC-V64 control page: custom-0 opcode family.

See [poly-isa-design-directions.md](poly-isa-design-directions.md) for design
rationale.
