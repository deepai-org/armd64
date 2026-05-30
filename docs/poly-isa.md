# Poly ISA

Poly is an x86_64 CPU extension that adds direct AArch64 and RISC-V64 user-mode
frontends for existing precompiled code and fast cross-ISA library calls.

Run the current smoke test:

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Architectural contract:

- x86_64 owns boot, privilege, paging, interrupts, faults, and the shared TSO
  memory model.
- Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign modes fetch native 32-bit instructions directly. There are no legacy
  per-instruction `#UD` envelopes.
- Cross-ISA control uses `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and
  `PLANDING`.
- Fast register arguments use ABI signature slots. Stack args, aggregates,
  variadics, syscalls, libcalls, and layout conversion are software runtime work.
- Native returns cross back through return cookies recorded by `PCALL`.
- Trap packets are OS-neutral and include frontend, status, PC, and native
  argument registers.
- Foreign architectural state is XSAVE-style Poly state. Prototype component:
  `20`. State import layout version: `8`.

Prototype encodings, not final silicon allocations:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- CPUID base: `0x40000000`

Design rationale: `docs/poly-isa-design-directions.md`.
