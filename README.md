# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
a normal x86-64 Linux guest.

## Run

Requires Docker with `linux/arm64` image support.

```bash
make image
make boot-poly-full-arch-traps
```

- `make boot`: boot the x86-64 guest without polyglot support.
- `make boot-poly-call-arch-traps`: run mixed-ISA call/thread/signal/loader tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: run the broad polyglot regression set.

Guest output is in `out/serial.log`; Bochs trace output is in `out/bochs.log`.

## ISA Delta

This is still an x86-64 machine: paging, privilege, exceptions, interrupts,
virtual memory, syscalls, and memory ordering remain x86-64/Linux/TSO. Foreign
code runs in the same guest address space and uses the same page permissions.

- `CPUID 0x40000000+`: polyglot feature discovery.
- `0f 24 <op> "POLY!"`: prototype x86 poly opcodes.
- `PENTER.A64` / `PENTER.RV64`: switch to raw 32-bit AArch64 or RISC-V fetch.
- AArch64 `brk #0x7fff` / RISC-V `0x0000000b`: escape back to x86 control.
- `PCALL.*.SYSV`: call AAPCS64 or RISC-V psABI functions from x86-64 SysV.
- OS-neutral trap packets: no CPU-side Linux or libc emulation.
- XSAVE component 20: save/restore non-x86 architectural state.

The target is compatibility with existing precompiled cross-ISA objects, not a
new compiler-only ABI. See `docs/poly-isa.md` for details.
