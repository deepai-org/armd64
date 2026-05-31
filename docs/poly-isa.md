# Poly ISA

Poly extends x86_64 with user-mode AArch64 and RISC-V64 frontends in the same
virtual address space. The goal is compatibility with existing native ABI code.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, and the global TSO memory model.
- Foreign frontends fetch native instructions directly from `RIP`; no
  per-instruction `#UD` envelopes.
- All frontends share one x86_64 virtual address space and permission model.
- Extra foreign registers are explicit XSAVE-style state, not hidden emulator
  state keyed by CR3 or process.

## Control Ops

`PENTER frontend` enters a frontend. `PSWITCH frontend, target` tail-branches
to another frontend. `PCALL frontend, target, sig` cross-calls using ABI
signature slot `sig`. `PTRAPRET` resumes from a Poly trap packet. `PLANDING`
validates an indirect cross-frontend landing pad.

Native returns stay native: x86_64 `ret`, AArch64 `ret`, and RISC-V `ret`.
Cross-frontend returns use a hardware transition stack plus return cookie so
same-ISA returns do not pay a Poly check.

## Hardware/Software Split

Hardware handles frontend switching, call/return state, precise trap packets,
and register-only ABI signatures. A real implementation can apply those
signatures in rename/RAT logic without moving data through execution units.

Software handles linking policy, syscalls, libcalls, stack arguments,
aggregates, variadics, lazy binding, and other memory-shaped ABI translation.
Foreign `svc`, `ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable exits produce OS-neutral trap packets for runtime or OS policy.

## Prototype Encodings

The Bochs prototype decodes temporary Poly control opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 opcode family

These are decoded control instructions, not fast-path `#UD` traps. Real
hardware should allocate normal frontend opcodes.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
