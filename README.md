# Bochs Polyglot CPU Harness

Boots x86_64 Linux in a modified Bochs CPU model and tests a prototype ISA
extension for running precompiled AArch64 and RISC-V code from x86_64
userspace.

## Run

Requires Docker with `linux/arm64` support and the checked-in
`bochs-prepoly-src/` tree.

```bash
make image                       # Build the Bochs/test image.
make boot                        # Baseline x86_64 Linux boot.
make boot-poly                   # Main poly smoke tests.
make boot-poly-call-arch-traps   # Cross-ISA library call tests.
make boot-poly-binfmt-arch-traps # Foreign ELF execution through binfmt.
make boot-poly-full-arch-traps   # Broadest current boot test set.
make clean                       # Remove generated artifacts.
```

Read `out/serial.log` for guest pass/fail markers and `out/bochs.log` for the
Bochs CPU/device log.

## How The ISA Differs From x86_64

x86_64 is still the boot ISA, kernel ISA, and default userspace ISA. Normal
x86_64 code runs unchanged unless it executes the polyglot extension.

- CPUID advertises the extension through private leaves starting at
  `0x40000000`.
- The Bochs prototype encodes poly instructions as
  `0f 24 <op> 50 4f 4c 59 21` (`POLY!`). Real hardware should use dedicated
  non-exception opcodes.
- `PENTER.A64` and `PENTER.RV64` switch from x86 variable-length decode to raw
  32-bit AArch64 or RISC-V decode at the same virtual address space.
- `PCALL.*.SYSV` bridges x86_64 SysV callers into native AAPCS64 or RISC-V
  psABI callees. Existing binary compatibility is the goal, not a custom ABI.
- Foreign returns, syscalls, illegal instructions, and breakpoints become
  architectural trap/return records for the x86-side runtime or OS policy.
- Foreign memory uses the same guest virtual memory path as x86_64 and currently
  inherits x86 TSO ordering.
- Extra foreign state is internal in Bochs today; hardware should expose it via
  CPUID/XCR0/XSAVE-style OS-managed state.

Detailed architecture notes live in `docs/poly-isa.md`.
