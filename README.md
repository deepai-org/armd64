# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an ordinary x86_64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: x86_64 guest smoke test, poly disabled.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt path.
- `make boot-poly-full-arch-traps`: broad regression run.

Logs are written to `out/serial.log` and `out/bochs.log`.

## ISA Difference From x86-64

The base machine is still x86-64: boot, paging, privilege, virtual memory,
ordinary x86 code, and the Linux syscall ABI remain x86-64.

The prototype adds hardware-style polyglot execution:

- `CPUID 0x40000000+` feature discovery.
- Prototype x86 poly opcodes: `0f 24 <op> "POLY!"`.
- `PENTER.A64` / `PENTER.RV64` switch the frontend into raw 32-bit AArch64 or
  RISC-V instruction fetch.
- AArch64 `brk #0x7fff` and RISC-V custom instruction `0x0000000b` exit back to
  x86-64.
- `PCALL.*.SYSV` bridges x86-64 SysV callers to precompiled AArch64 AAPCS64 and
  RISC-V psABI functions.
- Foreign code uses the same x86-64 virtual address space, page faults,
  permissions, and TSO memory model.
- Foreign syscalls, traps, imports, and unsupported operations report
  OS-neutral trap packets rather than Linux-specific emulator callbacks.
- Non-x86 architectural state is exposed as XSAVE component 20.

The goal is compatibility with real precompiled cross-ISA objects, not a new
compiler-only ABI. Full details are in `docs/poly-isa.md`.
