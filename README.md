# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an x86_64 Linux guest process.

## Run

Docker must support `linux/arm64` containers.

```bash
make image
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: boot the plain x86_64 guest.
- `make boot-poly-arch-traps`: run opcode and trap smoke tests.
- `make boot-poly-call-arch-traps`: run mixed-call, loader, thread, and signal tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make clean`: remove generated `cache/`, `out/`, and `tmp/`.

Logs:

- `out/serial.log`: guest console and test markers.
- `out/bochs.log`: Bochs trace/debug output.

## ISA Differences

The machine is still x86_64 for paging, privilege, interrupts, exceptions,
syscalls, virtual memory, and TSO ordering. The prototype adds extra instruction
frontends and cross-ISA call/trap plumbing:

- `0f 24 <op> 50 4f 4c 59 21`: temporary Bochs opcode family.
- `PENTER.A64` / `PENTER.RV64`: enter raw 32-bit AArch64 or RISC-V fetch.
- AArch64 `brk #0x7fff` / RISC-V `0x0000000b`: leave raw fetch for x86_64.
- `PCALL.*.SYSV`: call precompiled AAPCS64 or RISC-V psABI functions from x86_64.
- Foreign `ret`/`jalr ra`: return through CPU-managed cross-ISA call cookies.
- Foreign `svc`, `ecall`, breakpoint, and illegal-instruction events become
  neutral trap records; libcalls are not emulated by the CPU.
- `CPUID 0x40000000+`: advertises prototype polyglot features.
- XSAVE component 20: stores non-x86 architectural state.

Detailed notes: `docs/poly-isa.md`.
