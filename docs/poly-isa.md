# Poly ISA

Poly is an x86_64 extension for running existing x86_64, AArch64, and RISC-V64
user code in one virtual address space. The goal is native ABI compatibility
with precompiled objects, not a new compiler-only ABI.

## Running It

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 remains the system ISA for privilege, paging, faults, interrupts,
  atomics, VM control, and memory ordering.
- AArch64 and RISC-V64 are user frontends over the same x86_64 virtual memory.
- Cross-ISA control uses decoded instructions, not `#UD` envelopes.
- Poly state is per-thread XSAVE-style architectural state.
- Hardware switches frontends, applies register signature slots, captures trap
  packets, and handles return cookies.
- Software handles loading, relocation, syscall/libcall policy, stack arguments,
  variadics, aggregates, and incompatible vectors.

## Control Instructions

- `PENTER frontend`: enter another frontend at the next instruction.
- `PSWITCH frontend,target`: branch to `target` in another frontend.
- `PCALL frontend,target,signature`: call `target` through an ABI signature slot.
- `PTRAPRET`: resume after a user-space Poly trap handler.
- `PLANDING`: mark a valid indirect cross-ISA landing target.

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64. Prototype encodings
are `0f 3a fc <subop>` on x86_64, reserved `HINT` space on AArch64, and
`custom-0` space on RISC-V64.

Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).
