# Poly ISA

Poly is a CPU extension for one process to run existing x86_64, AArch64, and
RISC-V64 code in one virtual address space.

## Run

```bash
make image
make boot-poly
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch normal 32-bit native
  instructions from the same virtual address space.
- Cross-ISA calls target the real native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast calls use fixed register-only ABI signature slots; stack and aggregate
  reshaping stays in software thunks.
- Syscalls, libcalls, relocation, lazy binding, debugging, and policy stay in
  the runtime or OS, not in the ISA.
- Foreign state is explicit per-thread XSAVE-style state, not hidden emulator
  state keyed by CR3 or process address space.
- The ISA has no single-instruction `#UD` envelopes.

## Control Instructions

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Instruction | Purpose |
| --- | --- |
| `PENTER` | Enter another frontend from trusted runtime/system code. |
| `PSWITCH` | Switch frontend and branch without call semantics. |
| `PCALL` | Cross-ISA call using an ABI signature slot. |
| `PTRAPRET` | Resume from a Poly monitor/trap path. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and
RISC-V `custom-0`.

Detailed design rationale lives in `docs/poly-isa-design-directions.md`.
