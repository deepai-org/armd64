# Poly ISA

Prototype extension for running existing precompiled x86_64, AArch64, and
RISC-V64 userspace code in one x86_64 virtual address space.

This file is the short operational reference. Constants are in
`tools/include/polycpuid.h`; rationale is in
`docs/poly-isa-design-directions.md`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, interrupts, and
  faults.
- CPL3 code can switch the frontend to raw AArch64 or RISC-V64.
- Foreign code is direct-fetched from `RIP`; there is no per-instruction `#UD`
  envelope.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Foreign state is explicit XSAVE-style state, not hidden emulator state.
- Hardware is OS-neutral: no Linux, libc, import, syscall, or stack-layout
  emulation.

## Operations

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a raw frontend. |
| `PSWITCH frontend, target` | Branch to another frontend. |
| `PCALL frontend, target[, sig]` | Cross-ISA call, optionally applying a register signature slot. |
| `PIRET` | Resume an interrupted frontend. |

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

Rules: AArch64 fetch is 4-byte aligned; RISC-V fetch is 2-byte aligned; every
transition ends the current decode block; native returns cross frontends only
through a `PCALL` return cookie.

## Encodings

The Bochs prototype uses temporary decoded x86 controls: `0f 3a fc <subop>`.
Foreign controls use reserved AArch64 `HINT` encodings and RISC-V64 `custom-0`
encodings. These are not exception paths.

A final silicon encoding may change the bytes, but must preserve fixed decode,
direct frontend redirect, no user-memory descriptor parsing, and no stack
rewriting.

## ABI State

Fast integer exchange window:

| x86_64 | RAX | RDX | RCX | RDI | RSI | R8 | R9 | R10 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| AArch64 | x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7 |
| RISC-V | a0 | a1 | a2 | a3 | a4 | a5 | a6 | a7 |

Signature slots are cached register-renaming templates for compatible integer,
FP, and fixed SIMD ABI lanes. Stack arguments, aggregates, variadics, PLT/GOT
policy, lazy binding, and incompatible vectors stay in software thunks.

Prototype XSAVE state: XCR0 component `20`, layout version `8`, size `4096`.

## Traps

With a per-thread user monitor installed, hardware writes an OS-neutral trap
packet and transfers to the monitor. Without one, syscall/import traps surface
as x86 `#UD`; breakpoint traps surface as x86 `#BP`.

Private CPUID leaves start at `0x40000000`; probe the live contract instead of
hardcoding optional features.
