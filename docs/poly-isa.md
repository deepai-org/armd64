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

## Difference From x86_64

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  virtual memory, atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends that direct-fetch 32-bit
  instructions from the same virtual address space.
- There are no per-instruction x86 `#UD` envelopes in the ISA model.
- Cross-ISA calls target existing native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast calls use fixed register-only ABI signature slots. Stack arguments,
  large aggregates, variadics, relocation, syscalls, libcalls, and loader policy
  stay in software.
- Foreign architectural state is explicit per-thread XSAVE-style state, not
  hidden CR3-scoped emulator state.

## Control

- `PENTER`: enter a foreign frontend
- `PSWITCH`: switch frontend without call semantics
- `PCALL`: cross-ISA call
- `PTRAPRET`: return from a runtime trap/monitor path
- `PLANDING`: validated cross-ISA landing point

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

Prototype encodings are `0f 3a fc <subop>` on x86_64, reserved `HINT`
encodings on AArch64, and `custom-0` encodings on RISC-V64.

Detailed architecture notes: `poly-isa-design-directions.md`.
