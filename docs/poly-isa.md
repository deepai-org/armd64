# Poly ISA Quick Reference

Poly lets precompiled x86_64, AArch64, and RISC-V64 user code run in one
process and one virtual address space. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Oops|Kernel panic' out/serial.log
```

## What Changes From x86_64

- x86_64 keeps privilege, paging, faults, interrupts, VM control, atomics, and
  global TSO ordering.
- AArch64 and RISC-V64 are user-mode fetch/decode frontends, not coprocessors
  and not `#UD` instruction envelopes.
- Cross-ISA calls target existing ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Fast calls use fixed ABI signature slots so hardware can remap register names
  in the rename stage. Stack arguments, aggregates, variadics, and other
  memory-shaped ABI details stay in software thunks.
- Extra foreign registers are explicit per-thread XSAVE-style state. They are
  not hidden emulator state and are not keyed only by CR3.
- Syscalls and recoverable traps produce OS-neutral trap packets for a user
  runtime monitor. The CPU does not implement Linux, libc, libgcc, libatomic, or
  dynamic-linker policy.
- There are no single-instruction `#UD` envelopes.

## Control Transfers

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER`: enter a frontend from trusted runtime or system code.
- `PSWITCH`: switch frontend and branch without call semantics.
- `PCALL`: cross-ISA call using an ABI signature slot.
- `PTRAPRET`: return from a Poly monitor or trap path.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Prototype encodings are intentionally non-final: x86_64 `0f 3a fc <subop>`,
AArch64 reserved `HINT`, and RISC-V `custom-0`.

## More Detail

- Architecture rationale: `docs/poly-isa-design-directions.md`
- Current implementation details: `README.md`
