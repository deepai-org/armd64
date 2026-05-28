# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code in an
ordinary x86_64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: x86_64 guest smoke test, poly disabled.
- `make boot-poly`: basic polyglot boot gate.
- `make boot-poly-call-arch-traps`: cross-ISA call, thread, and signal tests.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt path.
- `make boot-poly-full-arch-traps`: broad regression run.

Logs:

- `out/serial.log`: guest output.
- `out/bochs.log`: Bochs output.

## ISA vs x86_64

The machine is still x86_64. Boot, paging, privilege levels, virtual memory,
ordinary x86_64 code, and the Linux syscall ABI remain x86_64.

The prototype adds:

- Polyglot feature discovery under `CPUID 0x40000000+`.
- A prototype x86 opcode family: `0f 24 <op> "POLY!"`.
- `PENTER.A64` and `PENTER.RV64` to enter raw AArch64 or RISC-V fetch.
- Native exits back to x86_64: AArch64 `brk #0x7fff` and RISC-V custom-0
  `0x0000000b`.
- `PCALL.*.SYSV` instructions that bridge x86_64 SysV calls to precompiled
  AArch64 AAPCS64 or RISC-V psABI functions.
- Shared x86_64 virtual memory, page faults, permissions, and TSO memory
  ordering for foreign code.
- OS-neutral trap packets for foreign syscalls, breakpoints, illegal
  instructions, imports, and unsupported operations.
- XSAVE component 20 for non-x86 foreign state. Stock guest Linux enumerates it
  but does not enable `XCR0[20]`.

This is not a new compiler-only ABI. The goal is fast compatibility with real
precompiled AArch64 and RISC-V objects linked into x86_64 processes.

Full ISA details live in `docs/poly-isa.md`.
