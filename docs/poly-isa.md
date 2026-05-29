# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system CPU so
precompiled native ABI objects can share one virtual address space.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Model

| ID | Frontend | Fetch |
| --- | --- | --- |
| `0` | x86_64 | normal x86 byte stream |
| `1` | AArch64 | raw 32-bit words from `RIP` |
| `2` | RISC-V64 | raw RV64/RVC from `RIP` |

## Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, and TSO memory ordering.
- Foreign code is direct-fetched. There is no per-instruction `#UD` envelope.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Register-only ABI cases can use cached signature slots. Stack arguments,
  aggregates, variadics, lazy binding, and ABI reshaping stay in software.
- Foreign traps produce OS-neutral trap packets, not Linux/libc-specific CPU
  behavior.
- Foreign register state is explicit XSAVE-style state, not hidden emulator
  state.

## Controls

- Operations: `PENTER`, `PSWITCH`, `PCALL frontend,target,sig`, `PTRAPRET`.
- Prototype encodings: x86_64 `0f 3a fc <subop>`, AArch64 `HINT`, RISC-V
  `custom-0`.
- Prototype XSAVE component: `20`.

See `README.md` for current build/test status and
`docs/poly-isa-design-directions.md` for design rationale.
