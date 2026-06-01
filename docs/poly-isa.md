# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 system ISA.

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## ISA Delta

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 variable-width, AArch64 4-byte aligned, RISC-V64 2-byte aligned with RVC.
- x86_64 owns privilege, paging, faults, interrupts, atomics, virtual memory, and TSO.
- All frontends share one x86_64 virtual address space.
- Foreign registers and frontend TLS are XSAVE-style per-thread state.
- `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING` are decoded controls, not `#UD` envelopes.
- Foreign traps produce OS-neutral packets for a runtime/monitor.

## ABI Boundary

Target ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI. Register-only calls use ABI signature slots; stack arguments, aggregates, variadics, lazy binding, libcalls, and syscall policy stay in software thunks.

Null signature: `P0..P7` maps `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` to `x0..x7` / `a0..a7`; `F0..F7` maps `XMM0..XMM7` to `v0..v7` / `fa0..fa7`.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`.
- AArch64: reserved `HINT`, `0xd503201f | (subop << 5)`.
- RISC-V64: `custom-0`, `0x0000700b | (subop << 25)`.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
