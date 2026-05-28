# armd64

Bochs prototype for running existing precompiled AArch64 and RISC-V userspace
code inside an x86_64 Linux guest process.

## Run

Prerequisite: Docker must be able to run `linux/arm64` containers. On x86 hosts,
install/enable binfmt/qemu-user support first.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: plain x86_64 guest boot.
- `make boot-poly-arch-traps`: native opcode/trap smoke tests.
- `make boot-poly-call-arch-traps`: mixed-ISA calls, loader, threads, signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: full current runtime suite.
- `make clean`: remove generated `cache/`, `out/`, and `tmp/`.

Logs:

- `out/serial.log`: guest console and test output.
- `out/bochs.log`: Bochs trace/debug output.

## ISA Delta From x86_64

The base machine remains x86_64. Paging, privilege levels, interrupts,
exceptions, syscalls, virtual memory, and TSO memory ordering are still x86_64.
The extension adds raw AArch64/RISC-V instruction frontends and ABI bridges in
the same process address space.

Current prototype differences:

- `0f 24 <op> 50 4f 4c 59 21` is the Bochs placeholder opcode family.
- `PENTER.A64` switches from x86_64 fetch to raw 32-bit AArch64 fetch.
- `PENTER.RV64` switches from x86_64 fetch to raw RISC-V fetch.
- AArch64 `brk #0x7fff` and RISC-V custom opcode `0x0000000b` exit back to
  x86_64 fetch.
- `PCALL.*.SYSV` bridges x86_64 SysV calls into precompiled AAPCS64 or RISC-V
  psABI functions.
- Normal foreign return instructions return through a CPU-managed cookie for
  cross-ISA calls.
- Foreign `svc`, `ecall`, breakpoints, and illegal instructions create neutral
  trap records. The CPU does not implement Linux libcalls.
- `CPUID 0x40000000+` advertises the experimental polyglot features.
- XSAVE component 20 carries non-x86 architectural state in the prototype.

Detailed architecture notes live in `docs/poly-isa.md`.
