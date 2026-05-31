# Poly ISA Quick Reference

Poly extends one x86_64 machine with user-mode AArch64 and RISC-V64
frontends. The goal is compatibility with existing native ABI code in one
virtual address space, not a new source-language target.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused cross-ISA process-loader test:

```sh
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are additional user-mode instruction frontends that
  fetch native 32-bit instructions directly from `RIP`.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Cross-ISA control flow uses decoded Poly control instructions, not
  per-instruction `#UD` envelopes.
- Foreign register state is explicit XSAVE-style per-thread architectural
  state. The current state import layout version is `9`.
- Fast calls use register-only ABI signature slots. Hardware may implement
  these as register alias/RAT remaps; it must not parse user-memory call
  descriptors.
- Software still owns dynamic linking, syscall/libcall policy, stack
  arguments, aggregates, variadics, incompatible vectors, and loader/runtime
  thunks.

## Control Operations

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: mark or validate indirect cross-frontend targets.

Same-ISA returns use normal native returns. Cross-ISA returns use normal native
returns to reserved hardware return cookies.

## Prototype Encodings

- x86_64: temporary decoded `0f 3a fc <subop>` control page.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.

The Bochs prototype uses these encodings as stand-ins for future dedicated
silicon opcodes.

## Design Rationale

Detailed hardware/software boundary notes are in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
