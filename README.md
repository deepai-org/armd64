# armd64

Bochs prototype for running precompiled AArch64 and RISC-V code inside an
x86_64 Linux process. The guest still boots as normal x86_64 Linux; polyglot
execution is entered explicitly by userspace code.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: baseline x86_64 guest.
- `make boot-poly`: polyglot CPU smoke test.
- `make boot-poly-call-arch-traps`: cross-ISA calls, imports, TLS, ctors/dtors, and signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt path.
- `make boot-poly-full-arch-traps`: broadest current test run.

Logs: `out/serial.log` and `out/bochs.log`.

## ISA Delta From x86_64

Normal x86_64 instructions, Linux boot, paging, and userspace execution are
unchanged. The extension adds explicit polyglot entry, call, trap, and state
operations.

- Discovery: private CPUID leaves at `0x40000000+`.
- x86 opcodes: prototype `0f 24 <op> "POLY!"` encodings; no `UD2` envelopes.
- Raw entry: `PENTER.A64` and `PENTER.RV64` switch fetch/decode to 32-bit
  AArch64 or RISC-V instructions at the current guest virtual address.
- Raw exit: AArch64 uses `brk #0x7fff`; RISC-V uses custom-0 `0x0000000b`.
- Cross-ISA calls: `PCALL.*.SYSV` maps x86_64 SysV callers to native AAPCS64 or
  RISC-V psABI callees for existing compiled objects.
- Memory: all modes share x86_64 virtual memory, page faults, permissions, and
  the prototype's x86-style TSO ordering.
- Traps/syscalls: foreign syscalls, breakpoints, illegal instructions, and
  unsupported ops exit architecturally for OS or userspace policy.
- State: non-x86 architectural state is explicit and discoverable; Bochs also
  provides prototype save/restore opcodes for tests.

Detailed design notes are in `docs/poly-isa.md`.
