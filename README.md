# armd64

x86_64 Bochs VM prototype for running existing precompiled AArch64 and RISC-V
userspace code in the same virtual address space.

## Run

Requires Docker with `linux/arm64` container support:

```bash
make image
make boot                         # baseline x86_64 guest
make boot-poly-arch-traps         # poly CPUID/xstate/trap ABI
make boot-poly-call-arch-traps    # cross-ISA calls, threads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
make boot-poly-full-arch-traps    # full poly test set
```

After a run, check the serial log:

```bash
grep -a -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Run `make image` again after changing Bochs, guest tools, or guest tests.

## ISA Delta From x86_64

The base machine remains x86_64. Paging, privilege levels, interrupts,
exceptions, syscall entry, atomics, virtual memory, and TSO memory ordering stay
x86_64-owned.

The extension adds:

- Raw AArch64/RISC-V frontends: 32-bit fetch from shared `RIP`.
- Poly x86 operations: enter foreign modes, cross-ISA call, trap return, and
  foreign state save/restore.
- Native ABI bridges: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI, with
  ordinary foreign return instructions supported.
- Xstate-style storage for non-aliased foreign registers.
- OS-neutral trap records for foreign syscalls, breakpoints, illegal
  instructions, and faults.

Prototype note: Bochs currently uses temporary `0f 24 ... "POLY!"` encodings.
Hardware would need dedicated opcodes.
