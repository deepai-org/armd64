# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system CPU.
x86_64 remains the system ISA: boot, privilege, paging, interrupts, atomics,
syscalls, and global TSO ordering.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Differences

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign fetch reads native instructions from `RIP` in the same virtual address
  space as x86_64 code.
- Foreign registers are XSAVE-style architectural state, not hidden emulator
  state.
- `PCALL` is fixed-latency and uses register-only ABI signature slots.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding,
  and policy.
- Foreign syscalls, traps, illegal instructions, and import misses produce
  OS-neutral trap packets.

## Control Ops

Prototype encodings are in `tools/include/polycpuid.h`; silicon should use
vendor-allocated decoded opcodes, not `#UD` envelopes.

- `PENTER`: enter a frontend from trusted runtime code.
- `PSWITCH`: tail-branch to another frontend.
- `PCALL`: cross-frontend call with an ABI signature slot.
- `PCALL_SIG_IMM`: compact same-frontend call using a signature slot.
- `PTRAPRET`: resume after a precise Poly trap packet.
- `PLANDING`: mark/validate indirect cross-frontend targets.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
