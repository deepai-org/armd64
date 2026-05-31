# Poly ISA

Poly is an x86_64 CPU extension for running existing x86_64, AArch64, and
RISC-V64 userspace code in one address space. x86_64 remains the system ISA;
AArch64 and RISC-V64 are user-mode frontends.

Full rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 owns boot, privilege, paging, faults, interrupts, atomics, and TSO.
- Foreign code fetches native instructions directly from shared x86 virtual
  memory.
- Poly architectural state is explicit XSAVE-style state, not hidden CR3 or
  process-global emulator state.
- Hardware accelerates fixed-latency frontend switches and register aliasing.
- Software thunks handle stack args, aggregates, variadics, dynamic linking,
  libc policy, and syscall policy.

## Controls

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: cross-frontend tail branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend target when enabled.

Prototype encodings are x86 `0f 3a fc <subop>`, AArch64 reserved HINT, and
RISC-V custom-0. Final hardware should use decoded opcodes, not `#UD` traps.

## Compatibility

- Target ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Native returns cross frontends through a hardware transition stack plus
  reserved return cookies.
- Recoverable foreign syscalls, breakpoints, unresolved imports, and unsupported
  instructions produce OS-neutral trap packets for a Ring 3 monitor.
- Hardware must not parse user-memory descriptors, repack stacks, marshal
  structs, implement libcalls, or emulate Linux syscalls.
