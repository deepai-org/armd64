# Poly ISA

Poly is an x86_64 extension that adds direct AArch64 and RISC-V64 user-mode
frontends for existing precompiled code and fast cross-ISA library calls.

## Run It

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- x86_64 remains the system frontend. It owns boot, privilege, paging,
  interrupts, faults, and the shared TSO memory model.
- AArch64 and RISC-V64 are user-mode instruction frontends in the same virtual
  address space, not high-level emulated coprocessors.
- Foreign frontends fetch native 32-bit instructions directly from `RIP`.
- There are no legacy per-instruction `#UD` envelopes.
- Cross-ISA calls target real native ABIs. Hardware switches frontends and
  aliases register arguments; runtime thunks handle stack/layout work.
- Foreign architectural state is explicit XSAVE-style Poly state, not hidden
  emulator state keyed by `CR3` or TLS.
- Traps are OS-neutral packets. Hardware does not implement Linux syscalls,
  libc helpers, dynamic-linker policy, or user-memory call descriptors.

## Frontends

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

## Operations

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend, target`: branch to another frontend without a return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend landing pad.

`PCALL` records caller frontend, PC, SP, and flags in Poly architectural state
and installs a reserved return cookie in the callee's native return location;
ordinary native returns cross back by returning to that cookie.

## ABI Boundary

Fast calls use register-only ABI signature slots. A slot remaps source
argument/result registers to target registers without touching memory. Runtime
software handles stack arguments, aggregates, variadics, hidden structure
returns, incompatible vectors, syscalls, libcalls, unresolved imports, and lazy
binding.

## Prototype Encodings

These are Bochs prototype encodings, not final silicon allocations:

- CPUID base: `0x40000000`
- XSAVE component: `20`
- State import layout version: `8`
- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Detailed hardware and ABI rationale: `docs/poly-isa-design-directions.md`.
