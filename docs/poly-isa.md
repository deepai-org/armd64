# Poly ISA

Poly is a CPU extension for running existing x86_64, AArch64, and RISC-V64
code in one process and one virtual address space.

## Run

```bash
make image
make boot-poly
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_POLY_REAL_XSAVE_OK|POLYBENCH_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 remains the system ISA for privilege, paging, faults, interrupts,
  VM control, atomics, and global TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends. They fetch normal 32-bit native
  instructions directly from the same address space.
- Cross-ISA calls target the real native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Fast ABI calls use fixed register signature slots suitable for hardware
  rename/RAT remapping. Stack arguments, aggregates, variadics, and loader
  policy stay in software thunks.
- Syscalls, libcalls, dynamic linking, debugging, and policy are runtime or OS
  work. The ISA only reports precise trap packets.
- Foreign architectural state is explicit per-thread XSAVE-style state, not
  hidden CR3-scoped emulator state. The state import layout version is `9`.
- There are no single-instruction `#UD` envelopes in the fast path.

## Control Instructions

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

| Instruction | Purpose |
| --- | --- |
| `PENTER` | Enter a frontend from trusted runtime or system code. |
| `PSWITCH` | Switch frontend and branch without call semantics. |
| `PCALL` | Cross-ISA call using an ABI signature slot. |
| `PTRAPRET` | Return from a Poly monitor or trap path. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

Prototype encodings are x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`,
and RISC-V `custom-0`.

## Trap Packets

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable frontend exits produce OS-neutral trap packets. Import and
syscall packets preserve the first eight native foreign ABI argument registers
so the runtime can translate policy without CPU-side Linux/libc knowledge.

Detailed rationale lives in `docs/poly-isa-design-directions.md`.
