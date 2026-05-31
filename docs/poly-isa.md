# Poly ISA

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine so
existing native ABI objects can link and run in one virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
make boot-poly-exec-cross-arch-traps
rg -a 'POLY_EXEC_CROSS_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## ISA Delta

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, VM control, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends in the same address space.
- AArch64 fetches aligned 32-bit instructions; RISC-V64 fetches 16/32-bit
  instructions including RVC.
- Cross-ISA control uses decoded instructions, not `#UD` envelopes.
- Poly state is explicit per-thread XSAVE-style state. Import layout: `9`.
- Hardware only switches frontends, branches, records trap packets, handles
  return cookies, and applies register-only ABI signature slots.
- Software handles loading, relocation, syscalls, libcalls, stack arguments,
  variadics, aggregates, and incompatible vector layouts.

## Controls

| Instruction | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime code. |
| `PSWITCH frontend, target` | Switch frontend and branch without return. |
| `PCALL frontend, target, sig` | Call through ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a precise Poly trap packet. |
| `PLANDING` | Validate indirect cross-frontend landing targets. |

Native returns stay native; cross-ISA returns use hardware return cookies.

Prototype encodings: frontend IDs are `0` x86_64, `1` AArch64, `2` RISC-V64.
x86_64 uses decoded `0f 3a fc <subop>`, AArch64 uses a reserved HINT subspace,
and RISC-V64 uses custom-0.

Detailed design notes: [poly-isa-design-directions.md](poly-isa-design-directions.md).
