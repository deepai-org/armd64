# armd64

Bochs-based x86_64 VM prototype with ISA extensions for running existing
precompiled AArch64 and RISC-V userspace code in the same virtual address
space.

- Bochs fork: `bochs-prepoly-src/`
- Detailed ISA notes: `docs/poly-isa.md`

## Run

Requires Docker with `linux/arm64` container support.

```bash
make image
make boot                         # baseline x86_64 guest
make boot-poly-arch-traps         # CPUID, xstate, OS-neutral traps
make boot-poly-call-arch-traps    # cross-ISA calls, threads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
make boot-poly-full-arch-traps    # full poly test set
```

Check the serial log after a run:

```bash
grep -a -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Run `make image` again after changing Bochs, tools, or guest test sources.

## ISA Delta From x86_64

The machine is still x86_64. Paging, privilege levels, interrupts, exceptions,
syscall entry, atomics, and memory ordering follow x86_64 rules.

The extension adds:

- Frontend modes for raw AArch64 and RISC-V instruction fetch from `RIP`.
- x86 poly instructions for entering foreign modes, cross-ISA calls, trap
  return, and explicit foreign-state export/import.
- Native ABI bridges for x86_64 SysV calls into AArch64 AAPCS64 and RISC-V
  psABI code. Foreign code can return with ordinary native return instructions.
- Explicit foreign register state for non-aliased AArch64/RISC-V registers,
  modeled as xstate-style save/restore state rather than hidden OS policy.
- OS-neutral trap records for foreign syscalls, breakpoints, illegal
  instructions, and faults. Runtime/OS code decides how to handle them.

The current Bochs prototype uses temporary `0f 24 ... "POLY!"` encodings for
poly operations. Real hardware should allocate dedicated opcodes.
