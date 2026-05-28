# armd64

Bochs-based prototype for running precompiled AArch64 and RISC-V code inside an
x86_64 Linux process. x86_64 remains the boot, kernel, and default userspace ISA.

## Run

Prerequisite: Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: boot the baseline x86_64 guest.
- `make boot-poly`: boot with the polyglot CPU enabled.
- `make boot-poly-call-arch-traps`: run cross-ISA call/linker tests.
- `make boot-poly-binfmt-arch-traps`: run foreign executable/binfmt tests.
- `make boot-poly-full-arch-traps`: run the broadest current boot test set.

Logs are written to `out/serial.log` and `out/bochs.log`.

## ISA Changes From x86_64

Ordinary x86_64 code is unchanged. Polyglot behavior starts only when code
executes a polyglot opcode.

- Discovery uses private CPUID leaves starting at `0x40000000`.
- Polyglot x86 opcodes use prototype `0f 24 <op> "POLY!"` encodings, not
  `UD2` or exception-based instruction envelopes.
- `PENTER.A64` and `PENTER.RV64` switch the frontend into raw AArch64 or RISC-V
  fetch from the same guest virtual address space.
- AArch64 exits with `brk #0x7fff`; RISC-V exits with custom-0
  `0x0000000b`.
- `PCALL.*.SYSV` bridges x86_64 SysV callers to native AAPCS64 or RISC-V psABI
  callees so existing foreign objects can be linked and called.
- Foreign memory accesses use the same virtual memory and page-fault machinery
  as x86_64. The prototype gives foreign modes x86-style TSO ordering.
- Foreign syscalls, breakpoints, illegal instructions, and unsupported ops are
  architectural trap exits for OS or userspace policy. They are not hidden
  Bochs libcalls.
- Extra foreign register state is explicit ISA state. Bochs stores it
  internally; hardware should expose it through CPUID/XCR0/XSAVE-style state.

Detailed ISA notes live in `docs/poly-isa.md`.
