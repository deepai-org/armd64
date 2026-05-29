# Poly ISA Quick Reference

Poly lets precompiled x86_64, AArch64, and RISC-V64 user-mode code run in one
x86_64 virtual address space under the Bochs prototype.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Frontends

- `0`: x86_64, normal byte-stream fetch.
- `1`: AArch64, raw instruction fetch from `RIP`.
- `2`: RISC-V64, raw RV64/RVC fetch from `RIP`.

## Differences From x86_64

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  faults, atomics, virtual memory, and TSO ordering.
- Foreign frontends direct-fetch real instructions. There are no per-instruction
  `#UD` envelopes in the fast path.
- Cross-ISA calls target existing ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Register-only `PCALL` uses cached ABI signature slots for fast integer, FP,
  and compatible fixed-vector calls.
- Stack arguments, aggregates, variadics, lazy binding, and loader policy stay
  in software thunks/runtime code.
- Foreign traps produce OS-neutral trap packets, not Linux/libc emulation.
- Foreign register state is explicit XSAVE-style architectural state.

## Prototype Controls

- Operations: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`, `PTRAPRET`.
- Encodings: x86_64 `0f 3a fc <subop>`, AArch64 `HINT`, RISC-V `custom-0`.
- XSAVE component: `20`.

Design rationale lives in `docs/poly-isa-design-directions.md`.
