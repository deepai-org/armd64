# Bochs Polyglot CPU Harness

This boots x86_64 Linux in a modified Bochs CPU model and tests a prototype ISA
extension for running precompiled AArch64 and RISC-V code inside x86_64
userspace.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image                         # Build the Bochs/test image.
make boot                          # Baseline x86_64 Linux boot.
make boot-poly                     # Main poly smoke test.
make boot-poly-call-arch-traps     # Cross-ISA call/thread/signal tests.
make boot-poly-binfmt-arch-traps   # Run foreign ELF binaries through binfmt.
make boot-poly-full-arch-traps     # Broadest current boot test.
make clean                         # Remove generated cache/out/tmp files.
```

- `out/serial.log`: guest test output and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.

## ISA Delta From x86-64

x86-64 remains the boot ISA, kernel ISA, and default userspace ISA. Ordinary
x86-64 binaries run unchanged unless they execute the polyglot extension.

- Discovery uses private CPUID leaves starting at `0x40000000`.
- Prototype Bochs opcodes use `0f 24 <op> 50 4f 4c 59 21` (`POLY!`). These are
  direct decode hooks, not legacy `#UD` envelopes.
- `PENTER.A64` and `PENTER.RV64` switch the frontend from x86 variable-length
  decode to raw foreign decode in the same virtual address space.
- AArch64 executes as fixed 32-bit instructions; RISC-V executes raw RISC-V
  instructions, including compressed forms where supported.
- `PCALL.*.SYSV` bridges x86_64 SysV calls into native AAPCS64 or RISC-V psABI
  callees. The goal is compatibility with existing compiled objects, not a new
  compiler-only ABI.
- Foreign returns, syscalls, illegal instructions, and breakpoints produce
  architectural trap/return records for OS or runtime policy. The CPU model does
  not hard-code libcalls or Linux policy.
- Foreign memory uses the same guest virtual memory path as x86-64 and currently
  follows x86 TSO ordering.
- Bochs currently stores extra foreign state internally. A hardware version
  should expose that state through CPUID/XCR0/XSAVE-style OS-managed state.

Detailed architecture notes are in `docs/poly-isa.md`.
