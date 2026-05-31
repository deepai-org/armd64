# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system ISA so
existing precompiled libraries can run in one process and one virtual address
space.

## Run

```bash
make image
make boot-poly
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Oops|Kernel panic' out/serial.log
```

## Delta From x86_64

- x86_64 still owns privilege, paging, faults, interrupts, VM control, atomics,
  and global TSO memory ordering.
- AArch64 and RISC-V64 are direct-fetch user frontends. AArch64 fetches 32-bit
  instructions; RISC-V64 fetches 16-bit compressed and 32-bit instructions.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast `PCALL` paths use fixed register signature slots suitable for hardware
  RAT remapping. Memory-shaped ABI work stays in software thunks.
- Non-aliased foreign registers are explicit per-thread XSAVE-style state, not
  hidden CR3-scoped emulator state. Current import layout version: `9`.
- Syscalls and recoverable exits produce OS-neutral trap packets for a runtime
  monitor. The CPU does not know Linux, libc, or dynamic-linker policy.
- There are no single-instruction `#UD` envelopes.

## Control Transfers

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER`: enter a frontend from trusted runtime or system code.
- `PSWITCH`: switch frontend and branch without call semantics.
- `PCALL`: cross-ISA call using an ABI signature slot.
- `PTRAPRET`: return from a Poly monitor or trap path.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`,
RISC-V `custom-0`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
