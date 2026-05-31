# Poly ISA Summary

Poly keeps x86_64 as the system ISA and adds user-mode decode frontends for
existing AArch64 and RISC-V64 code. The goal is linking and running precompiled
cross-ISA objects in one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- Privilege, paging, interrupts, faults, VM control, atomics, and memory
  ordering remain x86_64-defined.
- AArch64 and RISC-V64 are alternate user-mode fetch/decode modes over the same
  x86_64 address space and thread.
- Cross-ISA transfers use decoded control instructions, not `#UD` envelopes.
- Non-x86 architectural state is per-thread XSAVE-style state.
- Register-only ABI calls may use hardware signature slots. Stack arguments,
  aggregates, variadics, loading, relocation, syscalls, and libcalls are handled
  by software/runtime policy.

## Control Surface

- `PENTER frontend`: enter another frontend at the next instruction.
- `PSWITCH frontend,target`: branch to `target` in another frontend.
- `PCALL frontend,target,signature`: call `target` through an ABI signature slot.
- `PTRAPRET`: resume after a user-space Poly trap handler.
- `PLANDING`: mark a valid indirect cross-ISA landing target.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Prototype encodings are `0f 3a fc <subop>` on x86_64, reserved `HINT` space on
AArch64, and `custom-0` space on RISC-V64. Design rationale belongs in
[poly-isa-design-directions.md](poly-isa-design-directions.md), not this file.
