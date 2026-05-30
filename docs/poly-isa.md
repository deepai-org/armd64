# Poly ISA

Poly runs x86_64, AArch64, and RISC-V64 user code in one x86_64 virtual
address space. x86_64 remains the system CPU and owns boot, privilege, paging,
interrupts, faults, atomics, and the effective TSO memory model.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Differences From x86_64

| Area | Poly contract |
| --- | --- |
| Fetch | `x86_64` uses normal byte fetch; foreign modes use native 32-bit instruction fetch from the same virtual memory. |
| Switching | Dedicated Poly control ops switch frontends directly. Per-instruction `#UD` envelopes are not part of the ISA. |
| Calls | `PCALL frontend,target,sig` applies a cached register ABI signature for fast register-only calls. |
| ABI | Compatibility targets real SysV x86_64, AAPCS64, and RISC-V psABI objects. Complex calls still use software thunks. |
| Traps | Foreign syscalls, breakpoints, and faults produce OS-neutral trap packets; policy stays in user/runtime or OS software. |
| State | Foreign-only registers are explicit XSAVE-style architectural state, not hidden emulator state. |

## Control Ops

- `PENTER frontend`
- `PSWITCH frontend,target`
- `PCALL frontend,target,sig`
- `PTRAPRET`

Current prototype encodings are temporary: x86_64 `0f 3a fc <subop>`,
AArch64 reserved `HINT`, RISC-V `custom-0`, XSAVE component `20`.

Design rationale lives in `docs/poly-isa-design-directions.md`.
