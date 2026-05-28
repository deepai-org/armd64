# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an ordinary x86-64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: x86-64 guest smoke test, poly disabled.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt path.
- `make boot-poly-full-arch-traps`: broad poly regression run.

Logs:

- `out/serial.log`: guest-visible test output.
- `out/bochs.log`: emulator trace/debug output.

## How The ISA Differs From x86-64

The machine is still x86-64 for boot, paging, privilege, virtual memory,
interrupt delivery, and the normal Linux syscall ABI.

The polyglot extension adds:

- `CPUID 0x40000000+` feature discovery.
- Prototype x86 poly opcodes encoded as `0f 24 <op> "POLY!"`.
- `PENTER.A64` and `PENTER.RV64` to switch instruction fetch into raw 32-bit
  AArch64 or RISC-V mode.
- AArch64 `brk #0x7fff` and RISC-V custom instruction `0x0000000b` to return to
  x86-64 fetch.
- `PCALL.*.SYSV` bridges for x86-64 SysV callers invoking precompiled AArch64
  AAPCS64 or RISC-V psABI functions.
- Shared x86-64 virtual memory, page faults, permissions, and TSO ordering for
  all modes.
- OS-neutral trap packets for foreign syscalls, traps, imports, and unsupported
  operations.
- Non-x86 architectural state exposed through XSAVE component 20.

The goal is compatibility with existing precompiled cross-ISA objects, not a
new compiler-only ABI. See `docs/poly-isa.md` for the full contract.
