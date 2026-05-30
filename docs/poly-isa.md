# Poly ISA

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU. The target is fast compatibility with existing precompiled objects, not a new compiler-only ABI.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused boots: `boot-poly-probe-arch-traps`, `boot-poly-call-arch-traps`, `boot-poly-thread-arch-traps`, `boot-poly-neutral-arch-traps`, `boot-poly-full-arch-traps`.

## How It Differs From x86_64

- x86_64 still owns boot, privilege levels, paging, faults, interrupts, and TSO.
- Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign frontends fetch native 32-bit instructions directly from guest memory; there are no per-instruction `#UD` envelopes.
- Cross-ISA control is `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.
- `PCALL` records return state; ordinary native returns cross back through return cookies.
- Fast calls use register-only ABI signature slots/RAT remapping; stack args, aggregates, variadics, syscalls, libcalls, and layout conversion are software runtime/loader work.
- Trap packets are OS-neutral and report frontend, status, PC, and the first eight native ABI argument registers.
- Non-x86 architectural state is XSAVE-style Poly state: component `20`, import layout version `8`.

## Prototype Encodings

These are temporary Bochs encodings, not final silicon encodings:

- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- Prototype CPUID base: `0x40000000`

Design rationale: `docs/poly-isa-design-directions.md`.
