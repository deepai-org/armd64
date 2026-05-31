# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to one x86_64 machine. The
goal is compatibility with existing native ABI objects in one virtual address
space, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused cross-ISA loader path:

```sh
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Model

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user frontends. They fetch native 32-bit
  instructions directly from `RIP`.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Cross-ISA control flow uses decoded Poly control instructions. It does not
  wrap each foreign instruction in a `#UD` envelope.
- Foreign register state is explicit per-thread XSAVE-style architectural
  state. The current state import layout version is `9`.
- Fast calls use register-only ABI signature slots. Hardware can implement
  these as register alias/RAT remaps.
- Software owns dynamic linking, syscall/libcall policy, stack arguments,
  aggregates, variadics, incompatible vectors, and loader/runtime thunks.

## Control

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap packet.
- `PLANDING`: mark or validate indirect cross-frontend targets.

Same-ISA returns use native returns. Cross-ISA returns use native returns to
reserved hardware return cookies.

## Encodings

- x86_64: temporary decoded `0f 3a fc <subop>` control page.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.

The Bochs encodings are prototype stand-ins for future dedicated silicon
opcodes. See [poly-isa-design-directions.md](poly-isa-design-directions.md) for
the hardware/software boundary rationale.
