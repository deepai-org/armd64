# Poly ISA

Poly extends x86_64 with fast user-mode frontends for existing AArch64 and
RISC-V64 code. The target is compatibility with precompiled objects in one
virtual address space, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 is still the system ISA: privilege, paging, interrupts, faults,
  atomics, VM control, and memory ordering remain x86_64-defined.
- AArch64 and RISC-V64 execute as alternate user-mode decode frontends over the
  same x86_64 virtual memory and thread context.
- Cross-ISA branches and calls are real decoded instructions, not `#UD`
  exception envelopes.
- Non-x86 frontend state is per-thread XSAVE-style architectural state.
- Register-only ABI calls can use hardware signature slots; stack arguments,
  aggregates, variadics, relocation, loading, and syscall/libcall policy remain
  software responsibilities.

## Control Instructions

- `PENTER frontend`: enter another frontend at the next instruction.
- `PSWITCH frontend,target`: branch to `target` in another frontend.
- `PCALL frontend,target,signature`: call `target` through an ABI signature slot.
- `PTRAPRET`: resume after a user-space Poly trap handler.
- `PLANDING`: mark a valid indirect cross-ISA landing target.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Prototype encodings: `0f 3a fc <subop>` on x86_64, reserved `HINT` space on
AArch64, and `custom-0` space on RISC-V64.

Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
