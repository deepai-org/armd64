# armd64

Bochs prototype for running existing precompiled AArch64 and RISC-V userspace
code inside an x64 Linux process. The modified CPU lives in `bochs-prepoly-src/`.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps     # full polyglot test run
make boot                         # x86_64 baseline only
make boot-poly-call-arch-traps    # calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
```

Check `out/serial.log`:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Rebuild with `make image` after changing Bochs or test sources.

## How The ISA Differs From x64

The machine is still architecturally x64 for paging, privilege levels,
interrupts, exceptions, syscall entry, atomics, and memory ordering. The
extension adds only the pieces needed to enter, run, and return from foreign
userspace code:

- `PENTER`/`PEXIT`: switch the fetch/decode frontend between x64, raw
  AArch64, and raw RISC-V without per-instruction exception envelopes.
- `PCALL`: call existing foreign ABI code from x64 by mapping SysV arguments
  and returns to AAPCS64 or RISC-V psABI registers/stack layout.
- Explicit foreign register state and OS-neutral trap records for foreign
  syscalls, breakpoints, illegal instructions, and faults.

Current Bochs opcode-family placeholder:

```text
0f 24 <op> 50 4f 4c 59 21
```

Full architecture notes: `docs/poly-isa.md`.
