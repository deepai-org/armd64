# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends so
precompiled native objects can share one x86_64 virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## x86_64 Delta

- System semantics stay x86_64: privilege, paging, faults, interrupts, VM
  control, atomics, virtual memory, and TSO ordering.
- Foreign frontends fetch native instructions directly in user mode. No
  per-instruction `#UD` envelopes.
- Cross-frontend control uses decoded `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`,
  and `PLANDING` instructions.
- Foreign register state is per-thread XSAVE-style architectural state.
- Register-only ABI calls may use fixed signature slots. Stack arguments,
  aggregates, variadics, relocation, syscalls, and libcalls stay in software.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

Prototype encodings: x86_64 `0f 3a fc <subop>`, reserved AArch64 `HINT` space,
and RISC-V `custom-0` space. Longer rationale lives in
[`poly-isa-design-directions.md`](poly-isa-design-directions.md).
