# Poly ISA

Poly is a CPU extension prototype for running existing x86_64, AArch64, and
RISC-V64 user code in one virtual address space. x86_64 remains the system ISA:
boot, privilege, paging, interrupts, faults, atomics, VM control, and TSO memory
ordering stay x86-owned.

## Running It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused gates: `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## How It Differs From x86_64

| Area | Difference |
| --- | --- |
| Frontends | `0` x86_64, `1` AArch64, `2` RISC-V64. |
| Fetch | Foreign modes fetch normal aligned 32-bit instructions from `RIP`. |
| Switching | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded control operations, not `#UD` envelopes. |
| State | Foreign registers, trap packets, ABI signatures, transition state, and landing policy are XSAVE-style architectural state. |
| Interop | Fast calls use register-only ABI signature slots; thunks handle stack args, aggregates, variadics, lazy binding, libcalls, and syscall translation. |
| Prototype encodings | x86 currently uses temporary `0f 3a fc <op>` controls; constants live in `tools/include/polycpuid.h`. |

Detailed hardware and ABI rationale lives in `docs/poly-isa-design-directions.md`.
