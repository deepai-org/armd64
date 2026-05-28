# armd64

Prototype x86_64 machine with ISA extensions for running existing precompiled
AArch64 and RISC-V userspace code in the same virtual address space.

- Bochs CPU implementation: `bochs-prepoly-src/`
- Full ISA details: `docs/poly-isa.md`

## Build And Run

Requires Docker with `linux/arm64` container support.

```bash
make image
make boot                         # baseline x86_64 guest
make boot-poly-arch-traps         # CPUID, xstate, traps
make boot-poly-call-arch-traps    # cross-ISA calls, threads, signals
make boot-poly-binfmt-arch-traps  # foreign ELF/binfmt execution
make boot-poly-full-arch-traps    # full poly test set
```

Useful serial-log check:

```bash
grep -a -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|POLYBINFMT_OK|FAIL|Kernel panic' out/serial.log
```

Run `make image` again after changing Bochs, tools, or guest test sources.

## How The ISA Differs From x86_64

The base architecture is still x86_64: paging, privilege levels, interrupts,
exceptions, syscall entry, atomics, and memory ordering use x86_64 rules.

The extension adds:

- Raw AArch64 and RISC-V frontend modes. The CPU fetches native 32-bit foreign
  instructions from `RIP` in the same guest virtual address space.
- x86 poly instructions for entering foreign modes, cross-ISA calls, state
  export/import, and trap return. The prototype uses temporary Bochs-only
  encodings; hardware should use allocated opcodes.
- Native ABI interop. x86_64 SysV code can call AArch64 AAPCS64 and RISC-V
  psABI code, and foreign code can return with ordinary native return
  instructions.
- Architectural foreign register state. Registers that cannot alias x86_64
  state use a fixed xstate-style layout so an OS or runtime can save/restore
  them explicitly.
- OS-neutral trap records. Foreign syscalls, breakpoints, illegal instructions,
  and faults exit to x86 with structured state. The CPU does not implement
  Linux, libc, or loader policy.

See `docs/poly-isa.md` for the exact encodings, CPUID leaves, trap records, and
current ABI bridge coverage.
