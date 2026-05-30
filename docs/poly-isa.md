# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU so
existing precompiled libraries can share one virtual address space.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

| Area | Contract |
| --- | --- |
| System CPU | x86_64 still owns boot, privilege, paging, faults, interrupts, atomics, and TSO. |
| Frontends | `0` = x86_64 byte fetch, `1` = AArch64 32-bit fetch, `2` = RISC-V64 RV64/RVC fetch. |
| Code bytes | Foreign code is fetched from ordinary x86_64 virtual memory. No per-instruction `#UD` envelopes. |
| ABI target | Cross-ISA calls target SysV x86_64, AAPCS64, and RISC-V psABI objects. |
| Fast calls | `PCALL frontend,target,sig` selects a cached register-only ABI mapping. |
| Complex calls | Stack args, aggregates, variadics, lazy binding, and layout conversion are runtime thunk work. |
| Traps | Hardware emits OS-neutral trap packets, not Linux syscall/libc/dynamic-linker policy. |
| State | Non-aliased foreign registers are explicit XSAVE-style architectural state. |

## Control Ops

- `PENTER frontend`
- `PSWITCH frontend,target`
- `PCALL frontend,target,sig`
- `PTRAPRET`

Prototype encodings are temporary: x86_64 `0f 3a fc <subop>`, AArch64 reserved
`HINT`, RISC-V `custom-0`, XSAVE component `20`.

Detailed rationale: `docs/poly-isa-design-directions.md`.
