# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code in a
normal x86-64 Linux guest.

## Run

Requirement: Docker with `linux/arm64` image support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: boot the x86-64 guest with poly disabled.
- `make boot-poly-call-arch-traps`: run cross-ISA call/thread/signal tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: run the broad polyglot regression set.

Guest output is in `out/serial.log`. Bochs trace output is in `out/bochs.log`.

## How The ISA Differs From x86-64

The base machine remains x86-64:

- Paging, privilege, interrupts, exceptions, and virtual memory are x86-64.
- Memory ordering follows x86 TSO.
- Syscalls enter the x86-64 Linux syscall ABI.
- Foreign code uses the same guest address space and page permissions.

The prototype adds:

- `CPUID 0x40000000+` leaves for polyglot feature discovery.
- Prototype x86 poly opcodes using `0f 24 <op> "POLY!"`.
- `PENTER.A64` and `PENTER.RV64` to switch into raw 32-bit foreign fetch.
- AArch64 `brk #0x7fff` and RISC-V `0x0000000b` to escape back to x86 control.
- `PCALL.*.SYSV` bridges from x86-64 SysV into AAPCS64 or RISC-V psABI code.
- OS-neutral trap packets instead of CPU-side Linux or libc emulation.
- XSAVE component 20 for non-x86 architectural state.

The goal is compatibility with existing precompiled cross-ISA objects, not a new
compiler-only ABI. Full ISA details are in `docs/poly-isa.md`.
