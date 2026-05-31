# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code run in one
x86_64 virtual address space. It targets native ABI compatibility, not a new
compiler-only ABI.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 remains the system ISA for privilege, paging, faults, interrupts,
  atomics, VM control, and TSO ordering.
- AArch64 and RISC-V64 are user frontends in the same virtual address space.
  AArch64 fetches aligned 32-bit instructions; RISC-V64 fetches 16/32-bit
  instructions including RVC.
- Cross-ISA control is decoded hardware, not `#UD` envelopes. Fast calls use
  register-only ABI signature slots for real SysV, AAPCS64, and RISC-V psABI
  objects.
- Hardware switches frontends, branches, records trap packets, handles return
  cookies, and maps registers. Software handles loaders, syscalls, libcalls,
  stack arguments, variadics, aggregates, and incompatible vector layouts.
- Poly state is per-thread XSAVE-style architectural state.

## Controls

`PENTER f` enters a frontend, `PSWITCH f,target` branches across frontends,
`PCALL f,target,sig` calls through ABI signature slot `sig`, `PTRAPRET` resumes
from a trap packet, and `PLANDING` marks indirect landing targets.

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64. Prototype encodings
are `0f 3a fc <subop>` on x86_64, reserved HINT on AArch64, and custom-0 on
RISC-V64. Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
