# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
a normal x86-64 Linux guest.

## Run

Requirements: Docker, including `linux/arm64` image support.

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

## ISA Delta

The base machine is still x86-64. Paging, privilege, interrupts, exceptions,
virtual memory, TSO ordering, and the Linux syscall ABI remain x86-64.

The extension adds:

- `CPUID 0x40000000+` leaves for polyglot feature discovery.
- Prototype x86 entry/call opcodes: `0f 24 <op> "POLY!"`.
- Raw 32-bit fetch modes for AArch64 and RISC-V: `PENTER.A64`, `PENTER.RV64`.
- Native escapes back to x86 control: AArch64 `brk #0x7fff`, RISC-V `0x0000000b`.
- `PCALL.*.SYSV` bridges from x86-64 SysV into AAPCS64 or RISC-V psABI code.
- Shared x86-64 address translation, page faults, and memory permissions.
- OS-neutral trap packets rather than CPU-side Linux/libc emulation.
- XSAVE component 20 for non-x86 architectural state.

The target is compatibility with existing precompiled cross-ISA objects, not a
new compiler-only ABI. See `docs/poly-isa.md` for the full ISA contract.
