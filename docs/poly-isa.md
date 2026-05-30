# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 code in one virtual address
space. x86_64 remains the system ISA; AArch64 and RISC-V64 are user-mode
execution frontends.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `boot-poly-arch-traps`, `boot-poly-call-arch-traps`,
`boot-poly-full-arch-traps`.

## Differences From x86_64

- x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics,
  and global TSO ordering.
- AArch64 and RISC-V64 fetch normal aligned 32-bit instructions from the shared
  virtual address space.
- Frontend changes are decoded Poly control instructions, not `#UD` traps.
- Foreign registers, trap state, ABI signature slots, and transition state are
  explicit XSAVE-style architectural state.
- Fast cross-ISA calls use register-only ABI signature slots; stack arguments,
  aggregates, variadics, lazy binding, libcalls, and syscall translation remain
  software/runtime responsibilities.

## Reference

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Control ops: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.
- Current prototype constants: `tools/include/polycpuid.h`.
- Design rationale: `docs/poly-isa-design-directions.md`.
