# armd64

Bochs x86_64 VM prototype for running existing precompiled AArch64/RISC-V
userspace code alongside x86_64 code. Detailed ISA notes are in
`docs/poly-isa.md`.

## Run

Requires Docker with `linux/arm64` support. Re-run `make image` after changing
Bochs, guest tools, or guest tests.

```bash
make image
make boot                         # baseline x86_64 guest
make boot-poly-arch-traps         # CPUID, xstate, trap ABI
make boot-poly-call-arch-traps    # cross-ISA calls, threads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt path
make boot-poly-bench-arch-traps   # interop benchmarks
make boot-poly-full-arch-traps    # full test set
```

Check the serial log:

```bash
grep -a -E 'OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Differences From x64

- x64 stays in charge of page tables, privilege, interrupts, exceptions, syscall
  entry, atomics, virtual memory, and TSO memory ordering.
- Explicit x64 poly operations switch to raw AArch64/RISC-V frontends; foreign
  instructions are fetched directly, not wrapped in one `#UD` envelope each.
- Foreign code shares the x64 virtual address space, permissions, and memory model.
- Cross-ISA calls bridge real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Non-x64 registers are architectural XSAVE-style state, not hidden emulator state.
- Foreign syscalls, imports, breakpoints, illegal instructions, and faults produce
  OS-neutral trap records; software decides policy.
- Bochs currently uses temporary `0f 24 ... "POLY!"` encodings. Hardware would need
  assigned opcodes.
