# Poly ISA

Poly is a Bochs prototype for running precompiled x86_64, AArch64, and
RISC-V64 user-mode code in one x86_64 virtual address space.

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

## How It Differs From x86_64

- x86_64 remains the system ISA: boot, paging, privilege, faults, interrupts,
  atomics, virtual memory, and TSO ordering.
- Foreign modes execute native instructions directly, not `#UD` envelopes.
- Cross-ISA calls target existing ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Fast `PCALL` is register-only and uses cached ABI signature slots.
- Stack arguments, aggregates, variadics, and lazy binding are handled by
  software thunks/runtime code.
- Foreign traps create OS-neutral trap packets; they do not emulate Linux or
  libc in hardware.
- Non-aliased foreign registers are explicit XSAVE-style architectural state.

## Controls

- Ops: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`, `PTRAPRET`.
- Temporary encodings: x86_64 `0f 3a fc <subop>`, AArch64 `HINT`,
  RISC-V `custom-0`.
- Prototype XSAVE component: `20`.

See `docs/poly-isa-design-directions.md` for design rationale.
