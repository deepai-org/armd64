# Poly ISA Quick Reference

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets:

```text
boot-poly-probe-arch-traps        boot-poly-neutral-arch-traps
boot-poly-call-arch-traps         boot-poly-thread-arch-traps
boot-poly-full-arch-traps
```

## Contract

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU. It targets existing precompiled SysV x86_64, AAPCS64, and RISC-V psABI code, not a new compiler-only ABI.
- x86_64 owns boot, rings, paging, faults, interrupts, and memory ordering.
- Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign frontends fetch native 32-bit instructions directly from normal virtual memory; there are no per-instruction `#UD` envelopes.
- Control ops are `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.
- `PCALL` records return state; ordinary native returns cross back through return cookies.
- Fast ABI crossing is register-only, via signature slots/RAT remapping.
- Stack arguments, aggregates, variadics, syscalls, libcalls, and memory layout conversion remain software thunk/monitor work.
- Trap packets are OS-neutral and include source frontend, status, PC, and the first eight native foreign ABI argument registers.
- Non-x86 registers are XSAVE-style Poly state: prototype component `20`; state import layout version is `3`.

## Temporary Encodings

Temporary Bochs encodings, not final silicon encodings:
- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`
- Prototype CPUID base: `0x40000000`
Design rationale: `docs/poly-isa-design-directions.md`.
