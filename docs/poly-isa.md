# Poly ISA Quick Reference

Poly keeps x86_64 as the system ISA and adds user-mode AArch64 and RISC-V64
frontends in the same virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign fetch decodes native 32-bit instructions from `RIP`.
- x86_64 still owns boot, paging, privilege, interrupts, atomics, syscalls, and
  the global TSO memory model.
- Foreign registers are XSAVE-style architectural state.
- `PCALL` is fixed-latency and applies register-only ABI signature slots.
- Software thunks handle memory-shaped ABI work: stack args, aggregates,
  variadics, lazy binding, and policy.
- Foreign syscalls, traps, illegal instructions, and import misses produce
  OS-neutral trap packets.

## Control Ops

Prototype encodings are in `tools/include/polycpuid.h`; silicon should use
vendor-allocated decoded opcodes, not `#UD` envelopes.

- `PENTER`: enter a frontend from trusted runtime code
- `PSWITCH`: tail-branch to another frontend
- `PCALL`: cross-frontend call with an ABI signature slot
- `PCALL_SIG_IMM`: compact call using an immediate signature slot
- `PTRAPRET`: resume after a precise Poly trap packet
- `PLANDING`: mark/validate indirect cross-frontend targets

Full rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
