# Poly ISA Quick Reference

Poly ISA lets existing precompiled x86_64, AArch64, and RISC-V64 userspace code run in one x86_64 virtual address space.

Deep rationale: `docs/poly-isa-design-directions.md`. Live constants: `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## Model

- x86_64 owns boot, privilege, paging, interrupts, and faults.
- CPL3 can switch the frontend to raw AArch64 or RISC-V64.
- Foreign instructions are direct-fetched from `RIP`; no per-instruction `#UD` envelope.
- Cross-ISA calls target real ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Foreign register state is explicit XSAVE-style state.
- The ISA is OS-neutral: no Linux syscall, libc, import, or stack-layout emulation in hardware.
Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

## Controls

- `PENTER frontend`: enter a raw frontend.
- `PSWITCH frontend, target`: branch to another frontend.
- `PCALL frontend, target[, sig]`: call another frontend, optionally applying a cached register signature.
- `PIRET`: resume an interrupted frontend.

Rules: AArch64 fetch is 4-byte aligned; RISC-V fetch is 2-byte aligned; every frontend switch ends the current decode block; cross-frontend native returns use `PCALL` return cookies.

Prototype encodings: x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, RISC-V `custom-0`. Final silicon encodings may change, but must remain fixed-decode direct frontend redirects. Hardware must not parse user-memory descriptors or rewrite stacks.

## ABI Fast Path

Integer exchange window:

| x86_64 | RAX | RDX | RCX | RDI | RSI | R8 | R9 | R10 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| AArch64 | x0 | x1 | x2 | x3 | x4 | x5 | x6 | x7 |
| RISC-V | a0 | a1 | a2 | a3 | a4 | a5 | a6 | a7 |

Signature slots are cached register-renaming templates for compatible integer, FP, and fixed SIMD ABI lanes.

Software thunks handle stack arguments, aggregates, variadics, PLT/GOT policy, lazy binding, and incompatible vectors.

Prototype XSAVE state: component `20`, layout version `8`, size `4096`.

## Traps

With a per-thread monitor installed, traps write an OS-neutral packet and jump to the monitor. Without one, syscall/import traps become x86 `#UD`; breakpoint traps become x86 `#BP`.

Private CPUID leaves start at `0x40000000`. Probe the live contract instead of hardcoding optional features.
