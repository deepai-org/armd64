# Bochs Polyglot CPU Harness

This boots x86-64 Linux in a modified Bochs CPU and tests a prototype ISA
extension for running existing precompiled AArch64 and RISC-V code inside an
x86-64 process.

## How To Run

Requires Docker with `linux/arm64` support.

```bash
make image                        # Build the Docker test image.
make boot-poly-full-arch-traps    # Broadest current regression run.
make boot                         # Baseline x86-64 Linux boot.
make boot-poly                    # Main smoke test.
make boot-poly-arch-traps         # Standalone foreign ELF/trap tests.
make boot-poly-call-arch-traps    # Cross-ISA call, thread, and signal tests.
make boot-poly-binfmt-arch-traps  # binfmt-launched foreign ELF tests.
make clean                        # Remove cache/out/tmp.
```

- `out/serial.log`: guest test output and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.

## ISA Differences From x86-64

x86-64 stays the boot ISA, kernel ISA, and default userspace ISA. Normal x86-64
code is unchanged unless it executes a polyglot instruction.

- Private CPUID discovery leaves starting at `0x40000000`.
- Prototype x86 poly opcodes use `0f 24 ... "POLY!"`; hot operations do not use
  `UD2` or `#UD` envelopes.
- `PENTER.A64` and `PENTER.RV64` frontend switches. After entry, Bochs fetches
  raw AArch64 or RISC-V instructions directly from the same guest virtual
  address space.
- Native foreign exits: AArch64 uses `brk #0x7fff`; RISC-V uses a custom opcode.
- `PCALL.*.SYSV` ABI bridges from x86-64 SysV callers to native AAPCS64 or
  RISC-V psABI callees.
- Shared virtual memory, page faults, and x86-64 TSO memory ordering for foreign
  execution.
- Architectural trap records for foreign syscalls, breakpoints, illegal
  instructions, and unsupported operations. The CPU records the trap; OS or
  userspace policy handles it.
- Bochs still holds extra foreign registers in synthetic state. Hardware should
  expose that state through CPUID/XCR0/XSAVE-style context switching.

Detailed architecture notes are in `docs/poly-isa.md`.
