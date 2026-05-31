# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to one x86_64 machine so
existing native ABI objects can share one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log

# focused cross-ISA loader test
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Architecture Contract

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  faults, atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user frontends fetched from `RIP`.
- AArch64 fetch is aligned 32-bit; RISC-V fetch is native 16/32-bit so RVC
  remains valid.
- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Cross-ISA control flow uses decoded Poly control instructions, not per-
  instruction `#UD` envelopes.
- Foreign register state is explicit per-thread XSAVE-style state. Current
  state import layout version: `9`.
- Fast calls use register-only ABI signature slots, implementable as register
  alias/RAT remaps.
- Software owns linking, syscall/libcall policy, stack arguments, aggregates,
  variadics, incompatible vectors, and thunks.

## Control

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: mark or validate indirect cross-frontend targets.
- Same-ISA returns use native returns; cross-ISA returns use native returns to
  reserved hardware return cookies.

## Encodings

- x86_64: temporary decoded `0f 3a fc <subop>` control page.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.
- Bochs encodings are prototype stand-ins for future dedicated silicon opcodes.

See [poly-isa-design-directions.md](poly-isa-design-directions.md) for the
hardware/software boundary rationale.
