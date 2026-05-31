# Poly ISA Quick Reference

Poly runs existing x86_64, AArch64, and RISC-V64 userspace code in one x86_64 virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns privilege, paging, faults, interrupts, atomics, VM control, and TSO.
- Foreign frontends fetch native instructions directly from `RIP`; no per-instruction `#UD` envelope.
- Fetch alignment is AArch64 4 bytes and RISC-V 2 bytes for RVC.
- Foreign registers are XSAVE-style architectural state.

## Controls

- `PENTER frontend`
- `PSWITCH frontend, target`
- `PCALL frontend, target, sig`
- `PTRAPRET`
- `PLANDING`

Same-ISA returns stay native. Cross-frontend returns use ordinary native return instructions plus transition-stack cookies.

## Boundary

- Hardware: switching, call/return cookies, trap packets, XSAVE state, and fixed-latency register-only ABI remapping.
- Software: syscalls, libcalls, linking, stack arguments, aggregates, variadics, incompatible vectors, and memory-shaped ABI translation.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 opcode family

Rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
