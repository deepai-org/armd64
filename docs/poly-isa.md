# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
process and virtual address space. The target is compatibility with existing
native ABI objects, not a new compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused tests:

```sh
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
make boot-poly-binfmt-arch-traps
```

## ISA Delta From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  exceptions, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are direct-fetch user frontends sharing x86_64 virtual
  memory. They are not decoded through per-instruction `#UD` envelopes.
- Poly adds decoded control instructions for entering, switching, calling,
  landing-pad validation, and trap return.
- Cross-ISA calls preserve native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast register-only calls use ABI signature slots suitable for hardware
  register renaming. Stack arguments, variadics, aggregate repacking, lazy
  binding, syscall policy, and libc policy stay in software.
- Extra foreign registers are explicit per-thread XSAVE-style architectural
  state, not hidden CR3-scoped emulator state.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER frontend`: enter another frontend.
- `PSWITCH frontend, target`: cross-ISA tail branch.
- `PCALL frontend, target, sig`: cross-ISA call using ABI signature slot `sig`.
- `PLANDING`: validate an indirect cross-ISA landing target.
- `PTRAPRET`: resume from a Poly trap packet.

Design rationale: `docs/poly-isa-design-directions.md`.
