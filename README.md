# armd64

Bochs-based prototype for running existing precompiled AArch64 and RISC-V
userspace code inside an x86_64 Linux guest process.

Modified Bochs lives in `bochs-prepoly-src/`. Full ISA details live in
`docs/poly-isa.md`.

## How To Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Common faster checks:

```bash
make boot                         # x86_64 baseline
make boot-poly-call-arch-traps    # cross-ISA calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
```

Inspect the serial log:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Run `make image` again after changing Bochs or guest test sources.

## How The ISA Differs From x86_64

The base CPU is still x86_64. Paging, privilege levels, interrupts, exceptions,
syscall entry, atomics, and the memory model follow x86_64 rules.

The polyglot extension adds only the cross-ISA machinery:

- Enter raw AArch64 or RISC-V fetch/decode from x86_64.
- Exit raw AArch64 or RISC-V fetch/decode back to x86_64.
- Bridge calls between x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI code.
- Expose non-x86 register state as architectural save/restore state.
- Report foreign syscalls, breakpoints, illegal instructions, and faults as
  OS-neutral traps instead of emulating Linux or libc in the CPU.

Current prototype opcodes use the Bochs-only family:

```text
0f 24 <op> 50 4f 4c 59 21
```
