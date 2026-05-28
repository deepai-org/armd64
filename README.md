# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code in a
normal x86-64 Linux guest.

## Run

Requirements: Docker plus `linux/arm64` binfmt/qemu-user support on x86 hosts.

```bash
make image
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: boot plain x86-64 Linux.
- `make boot-poly-call-arch-traps`: run mixed-ISA call/thread/signal/loader tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: run the broad polyglot regression set.

Logs: `out/serial.log` has guest/test output; `out/bochs.log` has Bochs traces.

## ISA Delta From x86-64

The base machine is still x86-64: paging, privilege levels, exceptions,
interrupts, virtual memory, syscalls, and memory ordering remain x86-64.
Foreign code shares the same guest address space and page permissions.

- `CPUID 0x40000000+`: discovers the experimental polyglot features.
- `0f 24 <op> "POLY!"`: Bochs prototype opcode family.
- `PENTER.A64` / `PENTER.RV64`: switches to raw AArch64 or RISC-V fetch.
- AArch64 `brk #0x7fff` and RISC-V `0x0000000b`: escape back to x86-64 fetch.
- `PCALL.*.SYSV`: calls precompiled AAPCS64/RISC-V psABI functions from x86-64.
- Foreign `svc`, `ecall`, `brk`, and illegal instructions produce trap packets.
- XSAVE component 20 stores non-x86 architectural state.

The goal is compatibility with existing precompiled cross-ISA objects, not a new
compiler-only ABI. Detailed contracts live in `docs/poly-isa.md`.
