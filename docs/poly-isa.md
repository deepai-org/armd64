# Poly ISA

Poly is an x86_64 CPU extension for running existing x86_64, AArch64, and
RISC-V64 userspace code in one virtual address space. x86_64 remains the system
ISA; AArch64 and RISC-V64 are user-mode frontends.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.
- x86_64 owns boot, privilege, paging, interrupts, faults, atomics, and the TSO
  memory model.
- Foreign frontends fetch native code directly: AArch64 is 4-byte aligned;
  RISC-V supports normal RVC alignment.
- All frontends share x86 virtual memory, page permissions, and explicit
  XSAVE-style Poly state.
- Fast calls use decoded frontend-switch instructions plus cached register
  signature slots. Software thunks handle stack arguments, aggregates,
  variadics, dynamic linking, libc, and syscall policy.

## Controls

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: cross-frontend tail branch.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature `sig`.
- `PTRAPRET`: resume after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend target when enabled.

Prototype encodings: x86 `0f 3a fc <subop>`, AArch64 reserved HINT, RISC-V
custom-0. Final hardware should use decoded opcodes, not `#UD` traps.

## Interop

- Compatibility target: native x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI
  objects, not a new compiler-only ABI.
- Native returns remain valid through a hardware transition stack and reserved
  return cookies.
- Recoverable foreign syscalls, breakpoints, unresolved imports, and unsupported
  instructions produce OS-neutral trap packets for a Ring 3 monitor.
- Hardware does not parse user-memory call descriptors, repack stacks, marshal
  structs, implement libcalls, or emulate Linux syscalls.
