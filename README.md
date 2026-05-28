# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an ordinary x86-64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: x86-64 guest smoke test, poly disabled.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt path.
- `make boot-poly-full-arch-traps`: broad regression run.

Logs are written to `out/serial.log` for guest output and `out/bochs.log` for
Bochs trace/debug output.

## ISA Delta From x86-64

The system still boots and runs as x86-64: paging, privilege, interrupts,
virtual memory, and the Linux syscall ABI remain x86-64.

The polyglot extension adds only the cross-ISA pieces:

- `CPUID 0x40000000+` discovery for the experimental contract.
- Prototype x86 poly opcodes: `0f 24 <op> "POLY!"`.
- `PENTER.A64` / `PENTER.RV64` raw fetch modes for AArch64 and RISC-V code.
- Native foreign escapes: AArch64 `brk #0x7fff`, RISC-V `0x0000000b`.
- `PCALL.*.SYSV` bridges from x86-64 SysV into AAPCS64 or RISC-V psABI code.
- Shared x86-64 memory translation, page faults, permissions, and TSO ordering.
- OS-neutral trap packets instead of CPU-side Linux/libc emulation.
- Non-x86 state exposed through XSAVE component 20.

The compatibility target is existing precompiled cross-ISA objects, not a new
compiler-only ABI. Full details: `docs/poly-isa.md`.
