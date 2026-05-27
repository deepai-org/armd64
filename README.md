# Bochs Polyglot CPU Harness

Boots x86_64 Linux in modified Bochs and tests a prototype CPU extension for
running precompiled AArch64 and RISC-V code from x86_64 userspace. The target
is real binary compatibility, including cross-ISA libraries linked into x86_64
programs.

## Requirements

- Docker with `linux/arm64` support.
- The checked-in `bochs-prepoly-src/` tree.

## Run

```bash
make image                       # Build the Bochs/test image.
make boot                        # Baseline x86_64 Linux boot.
make boot-poly                   # Main poly smoke tests.
make boot-poly-call-arch-traps   # Cross-ISA library calls.
make boot-poly-binfmt-arch-traps # Foreign ELF app execution.
make boot-poly-full-arch-traps   # Broad poly test set.
make clean                       # Remove generated output.
```

- `out/serial.log`: guest console and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.

Useful markers: `BOOT_OK`, `POLY_PROBE_OK`, `POLYAPP_OK`, `POLYCALL_OK`,
`POLYTHREAD_OK`, `POLYSIGNAL_OK`, `POLYBINFMT_OK`.

## ISA Difference From x86_64

x86_64 remains the boot ISA, kernel ISA, and default userspace ISA. Ordinary
x86_64 code runs unchanged unless it executes the polyglot extension.

- Discovery uses private CPUID leaves starting at `0x40000000`.
- Prototype Bochs opcodes use `0f 24 <op> 50 4f 4c 59 21` (`POLY!`).
- Hardware should use dedicated non-exception opcodes, not `UD2` traps.
- `PENTER.A64` and `PENTER.RV64` switch from x86 variable-length decode to raw
  AArch64 or RISC-V decode at the shared program counter.
- `PCALL.*.SYSV` bridges x86_64 SysV callers into native AAPCS64 or RISC-V
  psABI callees. Existing ABI compatibility wins over a custom compiler ABI.
- Native foreign returns use return cookies to resume the x86_64 caller.
- Foreign syscalls, breakpoints, illegal instructions, and traps exit as
  architectural trap records for OS/runtime policy.
- Foreign memory uses the same guest virtual memory path as x86_64 and inherits
  x86 TSO ordering in the prototype.
- Extra foreign state is internal in Bochs today; hardware should expose it via
  CPUID/XCR0/XSAVE-like OS-managed state.

Foreign escape instructions:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

Current limits: AArch64/RISC-V instruction coverage is a tested subset,
`polycall` is not a complete Linux dynamic linker, and equal-speed execution is
not yet a measured result. Detailed architecture: `docs/poly-isa.md`.
