# armd64

Bochs prototype for running existing precompiled AArch64 and RISC-V userspace
code inside an x86_64 Linux guest process.

- Modified Bochs: `bochs-prepoly-src/`
- Full ISA contract: `docs/poly-isa.md`

## Run

Requires Docker with `linux/arm64` container support.

```bash
make image
make boot-poly-arch-traps         # CPUID, xstate, traps, native checks
make boot-poly-call-arch-traps    # cross-ISA calls, pthreads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
make boot-poly-full-arch-traps    # all checks
```

Baseline x86_64 guest:

```bash
make boot
```

Check results:

```bash
grep -a -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Re-run `make image` after changing Bochs or guest test sources.

## ISA Delta From x86_64

The base machine remains x86_64: paging, privilege, interrupts, exceptions,
syscall entry, atomics, and TSO memory ordering follow x86_64 rules.

The extension adds raw AArch64/RISC-V frontend modes, native escapes back to
x86_64, SysV/AAPCS64/RISC-V psABI call bridges, architectural foreign-register
state, and OS-neutral trap packets for foreign syscalls, breakpoints, illegal
instructions, and faults.

The CPU does not implement Linux or libc as high-level emulation; software
handles foreign trap events.  Prototype x86 poly operations currently use:

```text
0f 24 <op> 50 4f 4c 59 21
```
