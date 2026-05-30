# Poly ISA

Poly keeps x86_64 as the system ISA and adds user-mode AArch64 and RISC-V64
frontends that share the same virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Foreign modes fetch native 32-bit instructions from `RIP`.
- x86_64 owns boot, paging, privilege, interrupts, atomics, syscalls, and TSO.
- Foreign architectural state is XSAVE-style state, not hidden emulator state.
- Cross-ISA calls use fixed-latency control transfers plus register-only ABI slots.
- Software thunks handle stack args, aggregates, variadics, lazy binding, and libc policy.
- Foreign syscalls, traps, illegal instructions, and import misses produce OS-neutral trap packets.

## Control Ops

Prototype encodings live in `tools/include/polycpuid.h`. Silicon should use
vendor-allocated decoded opcodes, not `#UD` envelopes.

`PENTER`, `PSWITCH`, `PCALL`, `PCALL_SIG_IMM`, `PTRAPRET`, `PLANDING`.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
