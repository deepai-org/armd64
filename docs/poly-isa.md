# Poly ISA Quick Reference

Poly runs precompiled x86_64, AArch64, and RISC-V64 userspace code in one
x86_64 virtual address space. x86_64 remains the system ISA; foreign ISAs are
user-mode frontends.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful variants: `make boot-poly-binfmt-arch-traps` for loader/binfmt coverage,
and `make boot-poly-full-arch-traps` for the full userspace runtime path.

## Differences From x86_64

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Boot, privilege, paging, faults, interrupts, VM control, atomics, and memory
  ordering stay x86_64-owned.
- AArch64 fetches native 32-bit instructions from `RIP`, 4-byte aligned.
- RISC-V64 fetches native instructions from `RIP`, 2-byte aligned for RVC.
- ISA transitions are decoded Poly control opcodes, not `#UD` envelopes.
- Fast register-only calls use cached ABI signature slots. Stack arguments,
  aggregates, variadics, and unusual ABI cases use loader/runtime thunks.
- Cross-ISA returns use native return instructions, a hardware transition stack,
  and return cookies. Same-ISA returns are unchanged.
- Foreign syscalls, breakpoints, and libcalls emit OS-neutral trap packets.

## Temporary Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Numeric subops live in [polycpuid.h](../tools/include/polycpuid.h).

## References

- Public constants: [polycpuid.h](../tools/include/polycpuid.h)
- Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Bochs prototype: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
