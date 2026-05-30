# Poly ISA Quick Reference

Poly is a Bochs prototype of an x86_64 system CPU with peer AArch64 and
RISC-V64 user-mode frontends for running existing precompiled code.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Architectural Delta From x86_64

- x86_64 remains the system frontend for boot, privilege, paging, faults,
  interrupts, atomics, virtual memory, and TSO ordering.
- Frontend `0` is x86_64 byte fetch, `1` is AArch64 32-bit direct fetch, and
  `2` is RISC-V64 RV64/RVC direct fetch.
- Foreign instructions are fetched natively from the shared x86_64 virtual
  address space. There are no per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, and RISC-V
  psABI.
- Fast `PCALL` is register-only: it selects an ABI signature slot. Stack
  arguments, aggregates, variadics, lazy binding, and memory layout conversion
  stay in runtime thunks.
- Traps are OS-neutral packets. The ISA does not emulate Linux syscalls, libc,
  or dynamic-linker policy.
- Non-aliased foreign registers are explicit XSAVE-style architectural state.

## Control Surface

- Architectural ops: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`,
  `PTRAPRET`.
- Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`,
  RISC-V `custom-0`.
- Prototype XSAVE component: `20`.

Detailed rationale and future hardware direction are in
`docs/poly-isa-design-directions.md`.
