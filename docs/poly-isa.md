# Poly ISA

Poly runs existing x86_64, AArch64, and RISC-V64 code in one process and one
virtual address space.

## Run

```bash
make image
make boot-poly
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Oops|Kernel panic' out/serial.log
```

## What Changes

- x86_64 remains the system ISA: privilege, paging, faults, interrupts, VM
  control, atomics, and global TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are user frontends. AArch64 direct-fetches 32-bit
  instructions; RISC-V64 direct-fetches 16-bit compressed and 32-bit
  instructions from the same address space.
- Cross-ISA calls target the real native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast calls use fixed register signature slots suitable for hardware RAT
  remapping. Stack arguments, aggregates, variadics, dynamic linking, libcalls,
  and policy stay in software.
- Foreign state is explicit per-thread XSAVE-style state, not hidden
  CR3-scoped emulator state. The state import layout version is `9`.
- Syscalls and recoverable exits report precise, OS-neutral trap packets. Import
  and syscall packets preserve the first eight native foreign ABI argument
  registers so the runtime can translate policy without CPU-side Linux/libc
  knowledge.
- There are no single-instruction `#UD` envelopes.

## Control

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

- `PENTER`: enter a frontend from trusted runtime or system code.
- `PSWITCH`: switch frontend and branch without call semantics.
- `PCALL`: cross-ISA call using an ABI signature slot.
- `PTRAPRET`: return from a Poly monitor or trap path.
- `PLANDING`: mark or validate an indirect cross-ISA landing point.

Prototype encodings are x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`,
and RISC-V `custom-0`.

Detailed rationale lives in `docs/poly-isa-design-directions.md`.
