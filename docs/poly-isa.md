# Poly ISA Quick Reference

Poly extends x86_64 with AArch64 and RISC-V64 user-mode frontends in the same
virtual address space. x86_64 remains the system ISA for boot, privilege,
paging, interrupts, faults, atomics, VM control, and memory ordering.

## Running It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused gates: `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`,
`make boot-poly-full-arch-traps`.

## How It Differs From x86_64

| Area | Poly behavior |
| --- | --- |
| Frontends | `0` x86_64, `1` AArch64, `2` RISC-V64 |
| Fetch | Foreign modes fetch aligned 32-bit instructions from `RIP` |
| Controls | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING` are decoded instructions, not traps |
| State | Foreign registers, trap packets, ABI signatures, transition state, and landing policy are XSAVE-style state |
| Calls | Register-only ABI signature slots are the fast path; thunks handle stack args, aggregates, variadics, lazy binding, libcalls, and syscall translation |
| Encodings | Prototype x86 controls use temporary `0f 3a fc <op>` bytes; constants live in `tools/include/polycpuid.h` |

See `docs/poly-isa-design-directions.md` for hardware and ABI rationale.
