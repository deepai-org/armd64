# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
a normal x86-64 Linux guest.

## Requirements

- Docker
- `linux/arm64` image support, usually through binfmt/qemu-user on x86 hosts

## Run

```bash
make image
make boot-poly-full-arch-traps
```

- `make boot`: boot the x86-64 guest without polyglot support.
- `make boot-poly-call-arch-traps`: run mixed-ISA call/thread/signal/loader tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: run the broad polyglot regression set.

Guest output is in `out/serial.log`; Bochs trace output is in `out/bochs.log`.

## How The ISA Differs From x86-64

This is still an x86-64 machine for paging, privilege, exceptions, interrupts,
virtual memory, syscalls, and memory ordering. Foreign code runs in the same
guest address space and uses the same page permissions as x86-64 code.

- `CPUID 0x40000000+`: polyglot feature discovery.
- `0f 24 <op> "POLY!"`: Bochs prototype opcode slot for poly operations.
- `PENTER.A64` / `PENTER.RV64`: switch from x86-64 fetch to raw AArch64 or
  RISC-V fetch.
- AArch64 `brk #0x7fff` / RISC-V `0x0000000b`: escape back to x86 control.
- `PCALL.*.SYSV`: call AAPCS64 or RISC-V psABI functions from x86-64 SysV.
- Foreign `svc`, `ecall`, `brk`, and illegal instructions produce OS-neutral
  trap packets; the CPU does not emulate Linux or libc.
- XSAVE component 20 is the architectural save area for non-x86 state.

The target is compatibility with existing precompiled cross-ISA objects, not a
new compiler-only ABI. See `docs/poly-isa.md` for details.
