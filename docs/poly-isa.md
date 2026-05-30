# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU. All
modes share one virtual address space; x86_64 still owns boot, privilege,
paging, interrupts, faults, atomics, and the effective TSO memory model.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' \
  out/serial.log
```

## Contract

- Fetch is mode-specific: x86_64 uses x86 byte fetch; foreign modes fetch native 32-bit instructions from `RIP`.
- Mode switches use dedicated Poly control instructions, not `#UD` envelopes.
- `PCALL frontend,target,sig` switches ISA and applies a cached register ABI signature for fast register-only calls.
- Compatibility targets real SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Stack arguments, aggregates, and variadics stay in software thunks.
- Foreign-only registers are XSAVE-style architectural state.
- Foreign syscalls, breakpoints, and faults produce OS-neutral trap packets.

## Controls

- `PENTER frontend`
- `PSWITCH frontend,target`
- `PCALL frontend,target,sig`
- `PTRAPRET`

Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`, RISC-V `custom-0`, XSAVE component `20`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
