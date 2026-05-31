# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine so
existing native objects can run in one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- Frontends are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts,
  atomics, and the global TSO memory model.
- Foreign code fetches native 32-bit instructions directly from `RIP`; Poly
  does not wrap each instruction in `#UD`.
- All frontends share x86_64 virtual memory, permissions, and TSO ordering.
- Non-x86 register state is explicit XSAVE-style architectural state, not
  emulator-hidden CR3/process state.

## Control Instructions

`PENTER` enters a frontend. `PSWITCH` tail-branches. `PCALL` cross-calls using
an ABI signature slot. `PTRAPRET` resumes from a trap packet. `PLANDING`
validates an indirect cross-frontend landing pad.

Native returns remain native. Cross-frontend returns use a hardware transition
stack plus return cookie, so ordinary same-ISA `ret` paths stay fast.

## Hardware Boundary

Hardware owns frontend switching, call/return state, precise trap packets, and
register-only ABI signatures. Software owns linking policy, syscalls, libcalls,
stack arguments, aggregates, variadics, lazy binding, and memory-shaped ABI
work. Recoverable exits produce OS-neutral trap packets for runtime or OS
policy.

## Prototype Encodings

The Bochs prototype uses temporary decoded control opcodes:

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 opcode family

These are not fast-path `#UD` traps. Real hardware should allocate normal
frontend opcodes.

Long-form design notes: [poly-isa-design-directions.md](poly-isa-design-directions.md).
