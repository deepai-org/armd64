# armd64

Bochs-based x86_64 VM prototype for running precompiled AArch64 and RISC-V userspace code in one x86_64 virtual address space.

Goal: run and link existing native ABI objects, not define a new ABI or trap every foreign instruction.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-binfmt-arch-traps
```

Other useful targets: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-bench-arch-traps`, `make boot-poly-full-arch-traps`.

Check `out/serial.log`:

```bash
grep -a -E 'OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Differences From x86_64

- x86_64 remains the system ISA for page tables, privilege, interrupts, faults, syscalls, atomics, virtual memory, and TSO ordering.
- Poly instructions switch the frontend to raw AArch64 or RISC-V fetch; there is no per-instruction `#UD` envelope.
- Foreign code shares the x86_64 virtual address space and memory permissions.
- Cross-ISA calls bridge real native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Foreign registers are explicit XSAVE-style architectural state, not hidden CR3-scoped emulator state.
- Foreign syscalls, breakpoints, illegal instructions, and faults become OS-neutral trap records for runtime or OS policy.
- The Bochs prototype uses temporary `0f 24 ... "POLY!"` encodings; hardware or FPGA implementations should allocate real opcodes.

Full architecture notes live in `docs/poly-isa.md`.
