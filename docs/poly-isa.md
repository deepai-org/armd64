# Poly ISA

Poly is an x86_64 extension for executing existing AArch64 and RISC-V64 user
code in the same virtual address space. Detailed rationale lives in
`docs/poly-isa-design-directions.md`.

## What Changes From x86_64

- Adds three user-mode instruction frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- AArch64 fetch is native 32-bit aligned; RISC-V64 fetch is native 16/32-bit with RVC.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts, VM control, kernel syscalls, atomics, and the global TSO memory model.
- Cross-ISA calls target native ABI code: SysV x86_64, AAPCS64, and RISC-V psABI.
- Per-thread Poly state is XSAVE-style architectural state, not hidden emulator state.

## Control Instructions

Controls are decoded instructions, not `#UD` trap envelopes. Current Bochs
prototype encodings:

```text
x86_64    0f 3a fc <subop>
AArch64   0xd503201f | ((subop & 0x7f) << 5)
RISC-V64  0x0000700b | ((subop & 0x7f) << 25)
```

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER frontend` |
| `0x04` | `PSWITCH frontend, target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL frontend, target, signature` |
| `0x30..0x3c` | `PCALL_SLOT 0..12` |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state/control setup |

## Hardware Boundary

- Fast `PCALL` uses register-only ABI signature slots for fixed-latency register-name remaps.
- Runtime thunks handle stack arguments, aggregates, variadics, vectors, lazy binding, syscalls, and helper libraries.
- `PCALL` records caller frontend, PC, SP, and flags, applies the signature, installs a native return cookie, and branches.
- Native `ret`/`ret x30`/RISC-V `ret` cross back through the cookie.
- Hardware may switch frontends, remap registers, validate landings, maintain the transition stack, and emit precise trap records.
- Hardware must not parse user-memory descriptors, repack stacks, implement libraries, translate syscalls, or encode OS policy.
