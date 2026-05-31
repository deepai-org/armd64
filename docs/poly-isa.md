# Poly ISA Quick Reference

Poly lets one x86_64 process execute real precompiled x86_64, AArch64, and
RISC-V64 code in one virtual address space.

## Run

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly` for a shorter smoke run.

## Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  virtual memory, atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode direct-fetch frontends, not `#UD`
  per-instruction envelopes.
- Cross-frontend control uses decoded Poly instructions: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`; frontend IDs are `0` x86_64,
  `1` AArch64, and `2` RISC-V64.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast calls use fixed register-only ABI signature slots. Stack arguments,
  large aggregates, variadics, relocation, syscalls, libcalls, and loader policy
  stay in software.
- Foreign register state is explicit per-thread XSAVE-style architectural state.

## Prototype Encodings

- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control space: reserved `HINT` encodings
- RISC-V64 control space: `custom-0` encodings

More detail: `../README.md` and `poly-isa-design-directions.md`.
