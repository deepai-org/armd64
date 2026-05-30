# Poly ISA

Poly extends an x86_64 system CPU with peer AArch64 and RISC-V64 user-mode
frontends for running existing precompiled code in one virtual address space.

## Run The Prototype

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From Plain x86_64

- x86_64 still owns boot, privilege, paging, faults, interrupts, atomics,
  virtual memory, and the global TSO memory model.
- Frontend `0` is x86_64 byte fetch, `1` is AArch64 32-bit direct fetch, and
  `2` is RISC-V64 RV64/RVC direct fetch.
- Foreign instructions come from normal x86_64 virtual memory. There are no
  per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, and RISC-V
  psABI.
- Fast calls use `PCALL frontend,target,sig`, where `sig` selects a cached
  register-only ABI mapping.
- Stack arguments, aggregates, variadics, lazy binding, and layout conversion
  stay in runtime thunks, not hardware.
- Traps are OS-neutral packets, not Linux syscall/libc/dynamic-linker policy.
- Non-aliased foreign registers are explicit XSAVE-style architectural state.

## Control Ops

- `PENTER frontend`
- `PSWITCH frontend,target`
- `PCALL frontend,target,sig`
- `PTRAPRET`

Prototype encodings are temporary: x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT`, RISC-V `custom-0`, XSAVE component `20`.

Detailed rationale lives in `docs/poly-isa-design-directions.md`.
