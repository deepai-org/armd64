# Poly ISA Quick Reference

Poly adds user-mode AArch64 and RISC-V64 frontends to an x86_64 machine. The
target is existing precompiled native-ABI code linked into one virtual address
space, not a new compiler-only ISA and not per-instruction `#UD` trapping.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Focused checks:

```sh
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 stays the system ISA for boot, privilege, paging, interrupts, VM
  control, atomics, and global TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends over the same x86_64 virtual
  address space and memory permissions.
- Foreign code is fetched and decoded directly. AArch64 fetch is 4-byte
  aligned; RISC-V fetch supports 2-byte alignment for RVC.
- Recoverable exits produce precise trap packets. Runtime/kernel policy decides
  whether to translate, resume, signal, or terminate.
- Non-x86 state is explicit per-thread XSAVE-style architectural state, not
  CR3-scoped hidden emulator state.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Operation | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Cross-frontend branch without return. |
| `PCALL frontend, target, sig` | Cross-frontend call using ABI signature slot `sig`. |
| `PLANDING` | Validate an indirect Poly landing target. |
| `PTRAPRET` | Resume from a precise Poly trap packet. |

The Bochs prototype uses temporary silicon-shaped encodings: x86 under
`0f 3a fc <subop>`, AArch64 in reserved HINT space, and RISC-V in custom-0.
Constants are in `tools/include/polycpuid.h`.

## ABI Boundary

- Supported native ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Register-only calls should use ABI signature slots for hardware register
  alias/RAT remapping.
- Stack arguments, variadics, large aggregates, incompatible vector layouts,
  lazy binding, and loader policy remain software-thunk responsibilities.
- Cross-ISA calls should return through ordinary native returns: x86_64 `ret`,
  AArch64 `ret x30`, and RISC-V `ret`/`jalr x0, ra, 0`.

## Detailed Design

- `docs/poly-isa-design-directions.md`
- `tools/include/polycpuid.h`
