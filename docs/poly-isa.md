# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA so
existing native objects can run in one x86_64 virtual address space.

## Run The Prototype

```sh
make image
make boot-poly
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly-full-real-xsave-arch-traps` for the broad XSAVE-backed
regression run.

## What Changes From x86_64

- x86_64 remains the system ISA for privilege, paging, interrupts, faults,
  atomics, VM control, virtual memory, and TSO memory ordering.
- AArch64 and RISC-V64 execute as direct-fetch user-mode frontends. There are
  no per-instruction `#UD` envelopes in the fast path.
- Cross-frontend control uses decoded Poly instructions: `PENTER`, `PSWITCH`,
  `PCALL`, `PTRAPRET`, and `PLANDING`.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign register state is explicit per-thread XSAVE-style architectural
  state, not hidden CR3-scoped emulator state.
- Fast native-ABI calls use fixed register signature slots. Stack arguments,
  aggregates, variadics, relocation, syscalls, libcalls, and loader policy stay
  in software/runtime code.

## Prototype Encodings

- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control space: reserved `HINT` encodings
- RISC-V64 control space: `custom-0` encodings

Long-form rationale: [`poly-isa-design-directions.md`](poly-isa-design-directions.md).
