# armd64

Bochs prototype for running precompiled AArch64/RISC-V userspace code inside an x86_64 Linux guest.

## Quickstart

Requires Docker with `linux/arm64` binfmt/qemu-user support on x86 hosts.

```bash
make image
make boot-poly-full-arch-traps
```

- `make boot`: plain x86_64 guest boot.
- `make boot-poly-call-arch-traps`: mixed-ISA calls, loader, threads, signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt tests.

Logs: `out/serial.log` for guest/test output, `out/bochs.log` for Bochs traces.

## ISA Difference From x86_64

The base machine is still x86_64: paging, privilege, interrupts, exceptions, syscalls, virtual memory, and TSO memory ordering remain x86_64. The extension adds raw AArch64/RISC-V frontends and ABI bridges in the same address space.

- `0f 24 <op> "POLY!"`: Bochs placeholder opcode family.
- `PENTER.A64` / `PENTER.RV64`: switch the frontend to raw AArch64 or RISC-V instruction fetch.
- AArch64 `brk #0x7fff` and RISC-V custom opcode `0x0000000b`: return to x86_64 fetch.
- `PCALL.*.SYSV`: ABI bridges for calling precompiled AAPCS64/RISC-V psABI functions from x86_64 SysV code.
- Foreign `svc`, `ecall`, breakpoints, and illegal instructions create neutral trap packets; there are no CPU-side Linux libcalls.
- `CPUID 0x40000000+` advertises features; XSAVE component 20 carries non-x86 architectural state.

Full architecture notes are in `docs/poly-isa.md`.
