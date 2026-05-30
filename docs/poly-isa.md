# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. The
goal is to run existing precompiled libraries from all three ISAs in one
process, with x86_64 still owning the OS-visible machine model.

## Run

```bash
make image
make boot-poly
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Full validation path: `make boot-poly-full-real-xsave-arch-traps`.

## How It Differs From x86_64

- x86_64 remains the privileged/system ISA: paging, interrupts, faults, VM
  control, atomics, syscalls, and TSO memory ordering stay x86-defined.
- AArch64 and RISC-V64 are alternate user-mode decode frontends over the same
  virtual address space, not separate machines or high-level emulators.
- Foreign instructions are fetched directly as native 32-bit instructions after
  an ISA switch. There are no per-instruction `#UD` envelopes.
- Cross-ISA calls target native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Fast calls use hardware ABI signature slots for register-only argument/return
  mapping. Stack arguments, aggregates, and variadics use software thunks.
- Non-shared foreign architectural state is explicit XSAVE-style state, not
  hidden CR3-scoped emulator state.
- Native returns cross ISA boundaries through transition cookies. Recoverable
  foreign exits are reported as OS-neutral trap packets.

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

## Control Encodings

Temporary decoded control instructions:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

These are real frontend control opcodes in the prototype, not exception-based
`#UD` envelopes.

## References

- [README](../README.md)
- [Design directions](poly-isa-design-directions.md)
- [CPUID ABI](../tools/include/polycpuid.h)
- [Bochs implementation](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
