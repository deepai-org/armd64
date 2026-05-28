# armd64

Bochs prototype for running existing precompiled AArch64 and RISC-V userspace
code inside an x86_64 Linux guest process.

- Modified Bochs tree: `bochs-prepoly-src/`
- Detailed ISA notes: `docs/poly-isa.md`

## Run

Requires Docker with `linux/arm64` container support.

```bash
make image
```

Common boot targets:

```bash
make boot                         # baseline x86_64 guest
make boot-poly-arch-traps         # CPUID, xstate, traps, native checks
make boot-poly-call-arch-traps    # cross-ISA calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
make boot-poly-full-arch-traps    # all poly checks
```

Check the serial log:

```bash
grep -a -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Run `make image` again after changing Bochs or guest test sources.

## ISA Delta From x86_64

The base CPU is still x86_64. Long mode, paging, privilege levels, interrupts,
exceptions, syscall entry, atomics, and memory ordering use x86_64 rules.

The polyglot extension adds:

- Raw AArch64 and RISC-V frontend modes that fetch native instructions from the
  same guest virtual address space.
- CPUID-gated x86 poly operations. The Bochs prototype encodes them as
  `0f 24 <op> 50 4f 4c 59 21`; production hardware would use real allocated
  opcodes, not `UD2` traps.
- Native ABI call bridges for existing code, not a new compiler-only ABI:
  x86_64 SysV calls can enter AArch64 AAPCS64 or RISC-V psABI code and return
  through normal foreign return instructions.
- Architectural foreign state for registers that do not alias x86_64 state.
  The prototype exposes export/import operations using the same fixed xstate
  layout that hardware should make OS-saveable.
- OS-neutral trap records for foreign syscalls, breakpoints, illegal
  instructions, and faults. Software decides how to translate those events;
  the CPU path does not implement Linux or libc policy.

See `docs/poly-isa.md` for opcode IDs, CPUID leaves, trap-frame layout, and the
current ABI bridge coverage.
