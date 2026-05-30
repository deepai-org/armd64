# Poly ISA

Poly lets one x86_64 process run precompiled AArch64 and RISC-V64 userspace
code in the same virtual address space. x86_64 remains the system ISA for
privilege, paging, faults, interrupts, atomics, VM control, and TSO memory
ordering.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused gates: `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, and `make boot-poly-binfmt-arch-traps`.

## ISA Delta From x86_64

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign frontends fetch native aligned 32-bit instructions from `RIP`.
- Poly controls are decoded instructions: `PENTER`, `PSWITCH`, `PCALL`,
  `PTRAPRET`, and `PLANDING`.
- Cross-ISA calls use ABI signature slots for register-only native ABI cases.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding,
  libcalls, and syscall translation.
- Extra state is XSAVE-style: foreign registers, trap packets, ABI signatures,
  transitions, monitor addresses, and landing policy.
- Prototype x86 encodings use temporary `0f 3a fc <op>` forms. Constants live
  in `tools/include/polycpuid.h`.

Architecture rationale lives in `docs/poly-isa-design-directions.md`.
