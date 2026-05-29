# Poly ISA Quick Reference

Poly is a prototype extension that lets precompiled x86_64, AArch64, and
RISC-V64 user-mode code run in one x86_64 virtual address space.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Frontends

- `0`: x86_64, normal byte-stream fetch.
- `1`: AArch64, raw 32-bit instruction fetch from `RIP`.
- `2`: RISC-V64, raw RV64/RVC fetch from `RIP`.

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, paging, privilege, faults,
  interrupts, atomics, and TSO ordering.
- Foreign code is direct-fetched. There are no per-instruction `#UD`
  envelopes.
- Cross-ISA calls target real ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- `PCALL` signature slots handle register-only ABI mappings. Stack arguments,
  aggregates, variadics, and lazy binding remain software/runtime work.
- Foreign traps use OS-neutral trap packets. The CPU does not emulate Linux,
  libc, or loader policy.
- Foreign register state is explicit XSAVE-style architectural state.

## Prototype Controls

- Operations: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`, `PTRAPRET`.
- Encodings: x86_64 `0f 3a fc <subop>`, AArch64 `HINT`, RISC-V `custom-0`.
- XSAVE component: `20`.

Deeper rationale lives in `docs/poly-isa-design-directions.md`.
