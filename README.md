# armd64

Bochs prototype for running existing AArch64 and RISC-V userspace code inside
an x86_64 Linux process. The CPU implementation is in `bochs-prepoly-src/`.

## Run

Prerequisite: Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful shorter runs:

```bash
make boot                         # baseline x86_64 boot
make boot-poly-call-arch-traps    # cross-ISA calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt path
```

Inspect the serial log:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Re-run `make image` after changing Bochs code.

## ISA Difference From x64

The base machine is still x86_64: paging, privilege rings, interrupts,
exceptions, syscall entry, atomics, and memory ordering remain x86_64.

The extension adds:

- Raw AArch64 and RISC-V frontend modes fetched from the same guest `RIP`.
- Enter/switch/call/return instructions for moving between x86_64, AArch64,
  and RISC-V without per-instruction traps.
- Native ABI bridges for existing compiled code: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Shared virtual memory and page-fault behavior through the x86 memory system.
- Explicit xstate-style storage for architectural AArch64/RISC-V registers that
  x86_64 does not already provide.
- OS-neutral trap records for foreign syscalls, breakpoints, illegal
  instructions, and faults. The CPU records the trap and exits to software; it
  does not implement Linux, libc, or library-call policy.

Current Bochs prototype x86 encoding prefix: `0f 24 <op> 50 4f 4c 59 21`.

Detailed architecture: `docs/poly-isa.md`.
