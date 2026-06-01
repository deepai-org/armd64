# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 execution to an x86_64 system for precompiled-code interop.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Differences From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 is variable-width; AArch64 is 32-bit aligned; RISC-V64 is 16-bit aligned with RVC.
- System model: x86_64 owns privilege, paging, interrupts, faults, atomics, VM, and TSO.
- State: all frontends share one x86_64 virtual address space; non-x86 registers are XSAVE-style per-thread state.
- Controls: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded opcodes, not `#UD` envelopes.
- Traps: foreign traps produce OS-neutral packets for a user-mode monitor.
- Calls: register-only calls use ABI signature slots; stack args, aggregates, variadics, lazy binding, libcalls, and syscall policy stay in software thunks.

Detailed rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
