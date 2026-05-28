# armd64

Bochs x86_64 VM prototype for running precompiled AArch64 and RISC-V userspace
code in the same virtual address space as x86_64 code.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot                       # baseline x86_64 Linux guest
make boot-poly-full-arch-traps  # broad poly regression run
```

Focused targets: `boot-poly-arch-traps`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, and `boot-poly-bench-arch-traps`.

Rebuild the image after changing Bochs, guest tools, or guest tests. Check the log:

```bash
grep -a -E 'OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Differences From x64

- x86_64 owns page tables, privilege, interrupts, exceptions, syscalls, atomics,
  virtual memory, and TSO ordering.
- Poly enter/call instructions switch to raw AArch64 or RISC-V fetch. There are no
  per-instruction `#UD` envelopes.
- Foreign code shares the x86_64 virtual address space, permissions, and memory model.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, RISC-V psABI.
- Foreign state is XSAVE-style CPU state, not hidden CR3-scoped emulator state.
- Foreign syscalls, imports, breakpoints, illegal instructions, and faults become
  OS-neutral trap records.
- Prototype encodings use temporary `0f 24 ... "POLY!"`; hardware needs real opcodes.

Full ISA notes: `docs/poly-isa.md`.
