# armd64

Modified Bochs harness for an x64 polyglot ISA prototype. x64 Linux stays the
native environment while selected instructions enter, run, and call existing
precompiled AArch64/RISC-V code in the same process.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Shorter targets: `make boot`, `make boot-poly`,
`make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`.
Logs: `out/serial.log` and `out/bochs.log`.

## ISA vs x64

x64 remains the boot ISA, kernel ISA, and default userspace ISA. Ordinary x64
code is unchanged unless it executes a polyglot instruction.

- Discovery: private CPUID leaves starting at `0x40000000`.
- Hot opcodes: prototype x64 `0f 24 <op> "POLY!"` forms, not `UD2`/`#UD`.
- Entry: `PENTER.A64`/`PENTER.RV64` switch to raw foreign fetch in the same
  guest virtual address space.
- Exit: AArch64 `brk #0x7fff`; RISC-V custom-0 opcode `0x0000000b`.
- Calls: `PCALL.*.SYSV` maps ordinary x64 SysV call state to native AAPCS64 or
  RISC-V psABI state so precompiled foreign functions can run.
- Memory: same virtual memory/page-fault path as x64; prototype foreign memory
  ordering is x64 TSO.
- Traps: foreign syscalls, breakpoints, illegal instructions, and unsupported
  ops produce architectural records for OS/userspace policy.
- State: Bochs stores extra foreign registers internally; hardware should expose
  that state through CPUID/XCR0/XSAVE-style context switching.

More detail: `docs/poly-isa.md`.
