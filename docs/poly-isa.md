# Poly ISA Quick Reference

Poly extends x86_64 with direct user-mode AArch64 and RISC-V64 frontends in the
same virtual address space. x86_64 remains the system ISA for boot, privilege,
paging, faults, interrupts, atomics, and the effective memory model.

Design rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Running It

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Architectural Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- Foreign code fetches native instructions directly from x86 virtual memory.
- Foreign architectural state is explicit XSAVE-style state.
- Hardware provides fixed-latency frontend switches and register aliasing only.
- Software handles ABI cases involving memory, policy, or dynamic linking.

## Control Instructions

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: cross-frontend tail branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend target when enabled.

Prototype encodings are x86 `0f 3a fc <subop>`, AArch64 reserved HINT, and
RISC-V custom-0. Final hardware should use decoded opcodes, not `#UD` traps.

## Compatibility Rules

- Target ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Native returns cross frontends through a hardware transition stack plus
  reserved return cookies.
- Recoverable foreign syscalls, breakpoints, unresolved imports, and unsupported
  instructions produce OS-neutral trap packets for a Ring 3 monitor.
- Hardware must not parse user-memory descriptors, repack stacks, marshal
  structs, implement libcalls, or emulate Linux syscalls.
