# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system CPU.
x86_64 still owns boot, privilege, paging, interrupts, atomics, syscalls, and
global TSO memory ordering.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Smaller targets: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, and `boot-poly-neutral-arch-traps`.

## Architecture Contract

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign code fetches native instructions from `RIP` in the same virtual
  address space as x86_64 code.
- Poly state is XSAVE-style architectural state, not hidden emulator state.
- `PCALL` uses register-only ABI signature slots. Software thunks handle stack
  arguments, aggregates, variadics, lazy binding, and policy.
- Foreign syscalls, breakpoints, illegal instructions, and import misses produce
  OS-neutral trap packets. Hardware does not implement Linux or libc behavior.

## Prototype Control Ops

These Bochs encodings are temporary. Real hardware should use vendor-allocated
decoded opcodes, not `#UD` envelopes.

| Op | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a foreign frontend from trusted runtime code. |
| `PSWITCH frontend, target` | Tail-branch to another frontend. |
| `PCALL frontend, target, sig` | Call another frontend with ABI slot `sig`. |
| `PCALL_SIG_IMM slot` | Compact in-frontend call using signature slot `slot`. |
| `PTRAPRET` | Resume after a precise Poly trap packet. |
| `PLANDING` | Mark/validate indirect cross-frontend targets. |

Current opcode details live in `tools/include/polycpuid.h`; hardware design
rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).
