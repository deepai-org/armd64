# Poly ISA

Poly extends an x86_64 Bochs VM with direct-fetch AArch64 and RISC-V64
user-mode execution for precompiled cross-ISA code.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Frontends

- `0`: x86_64 byte-stream fetch.
- `1`: AArch64 32-bit direct fetch from `RIP`.
- `2`: RISC-V64 direct RV64/RVC fetch from `RIP`.

## Difference From x86_64

- x86_64 remains the system ISA for boot, paging, privilege, interrupts,
  virtual memory, atomics, and TSO ordering.
- Foreign modes execute raw native instructions; there are no per-instruction
  `#UD` envelopes.
- Cross-ISA calls target existing ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Fast `PCALL` uses register-only ABI signature slots.
- Stack arguments, aggregates, variadics, and lazy binding are handled by
  software thunks/runtime code, not hardware memory repacking.
- Foreign traps produce OS-neutral trap packets; hardware does not emulate Linux
  or libc.
- Non-aliased foreign registers are XSAVE-style architectural state.

## Controls

- Ops: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`, `PTRAPRET`.
- Temporary encodings: x86_64 `0f 3a fc <subop>`, AArch64 `HINT`,
  RISC-V `custom-0`.
- Prototype XSAVE component: `20`.

See `docs/poly-isa-design-directions.md` for design rationale.
