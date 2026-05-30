# Poly ISA

Run precompiled AArch64 and RISC-V64 userspace code inside one x86_64 process. x86_64 remains the system ISA for privilege, paging, faults, interrupts, atomics, VM control, and TSO ordering.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates: `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## x86_64 Delta

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign fetch: native aligned 32-bit instructions from `RIP`.
- `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded controls, not `#UD` envelopes.
- State is XSAVE-style: foreign registers, traps, ABI signatures, transitions, and landing policy.
- Fast calls use ABI signature slots for register-only native ABI cases; thunks handle stack args, aggregates, variadics, lazy binding, libcalls, and syscall translation.
- Prototype x86 controls use temporary `0f 3a fc <op>` encodings; constants live in `tools/include/polycpuid.h`.

Design rationale: `docs/poly-isa-design-directions.md`.
