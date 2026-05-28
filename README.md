# armd64

Bochs prototype for running existing precompiled AArch64 and RISC-V userspace
code inside an x86_64 Linux process.

The modified emulator is in `bochs-prepoly-src/`. Detailed ISA notes are in
`docs/poly-isa.md`.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful shorter runs:

```bash
make boot                         # x86_64 baseline
make boot-poly-call-arch-traps    # cross-ISA calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
```

Check the serial log:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Re-run `make image` after changing Bochs or test sources.

## ISA Delta From x64

The base machine remains x86_64: paging, privilege levels, interrupts,
exceptions, syscall entry, atomics, and memory ordering follow x64 rules.

The polyglot extension adds:

- `PENTER.A64` / `PENTER.RV64`: switch fetch/decode from x64 to raw fixed-width
  AArch64 or RISC-V instructions.
- `PEXIT`: switch fetch/decode back to x64.
- `PCALL`: bridge x86_64 SysV calls to existing AAPCS64 or RISC-V psABI code by
  mapping native argument, return, stack, FP, and vector ABI state.
- Explicit foreign state: non-x64 registers are architectural state that can be
  saved/restored instead of hidden emulator-only process state.
- OS-neutral traps: foreign syscalls, breakpoints, illegal instructions, and
  faults produce trap records for software to handle; the ISA does not bake in
  Linux or libc policy.

Current Bochs prototype opcode family:

```text
0f 24 <op> 50 4f 4c 59 21
```
