# Bochs Polyglot CPU Harness

This boots x86-64 Linux in a modified Bochs CPU model and tests a prototype
ISA extension for running existing AArch64 and RISC-V code inside an x86-64
process. The goal is compatibility with real precompiled code and fast
cross-ISA interop, not a new compiler-only ABI.

## Run

Requires Docker with `linux/arm64` support:

```bash
make image                         # Build the Bochs/test image.
make boot                          # Baseline x86-64 Linux boot.
make boot-poly                     # Main smoke test.
make boot-poly-arch-traps          # Standalone foreign ELF/trap tests.
make boot-poly-call-arch-traps     # Cross-ISA call/thread/signal tests.
make boot-poly-binfmt-arch-traps   # binfmt-launched foreign ELF tests.
make boot-poly-full-arch-traps     # Broadest current regression run.
make clean                         # Remove generated cache/out/tmp files.
```

- `out/serial.log`: guest test output and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.

## ISA Delta From x86-64

x86-64 remains the boot ISA, kernel ISA, and default userspace ISA. Normal
x86-64 binaries run unchanged unless they execute the polyglot extension.

- Discovery uses private CPUID leaves starting at `0x40000000`.
- The prototype uses direct Bochs decode hooks: `0f 24 <op> 50 4f 4c 59 21`
  (`POLY!`), not hot-path `UD2`/`#UD` envelopes.
- `PENTER.A64` and `PENTER.RV64` switch the frontend from x86-64 decode to raw
  foreign decode in the same virtual address space.
- Native foreign escapes return through ordinary architectural instructions:
  AArch64 `brk #0x7fff` and a RISC-V custom opcode.
- `PCALL.*.SYSV` bridges x86-64 SysV calls to native AAPCS64 or RISC-V psABI
  callees by shuffling real ABI registers.
- Foreign memory uses the same guest virtual memory and page-fault path as
  x86-64. The current memory-ordering contract is x86-64 TSO.
- Foreign syscalls, breakpoints, illegal instructions, and unsupported
  operations surface as architectural trap records, not hard-coded Linux/libc
  policy.
- Extra foreign register state is internal in the Bochs prototype. A hardware
  design should expose it through CPUID/XCR0/XSAVE-style OS-managed state.

Detailed architecture notes are in `docs/poly-isa.md`.
