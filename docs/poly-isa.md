# Poly ISA

Poly ISA lets existing precompiled x86_64, AArch64, and RISC-V64 userspace code run in one x86_64 virtual address space.

Detailed rationale: `docs/poly-isa-design-directions.md`. Live constants: `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## x86_64 Differences

- x86_64 owns boot, privilege, paging, interrupts, and faults.
- CPL3 can switch the instruction frontend to raw AArch64 or RISC-V64.
- Foreign code is direct-fetched from `RIP`; there is no per-instruction `#UD` envelope.
- Cross-ISA calls target real ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Non-current frontend state is explicit XSAVE-style architectural state.
- Hardware stays OS-neutral: no Linux syscall, libc, import, or stack-layout emulation.

## Controls

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

- `PENTER frontend`: enter a raw frontend.
- `PSWITCH frontend, target`: branch to another frontend.
- `PCALL frontend, target[, sig]`: call another frontend; optional `sig` selects a cached register-renaming signature.
- `PIRET`: resume an interrupted frontend.

Rules: every frontend switch ends the current decode block. AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned. Cross-frontend native returns use `PCALL` return cookies.

Prototype encodings: x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, RISC-V `custom-0`. Final silicon encodings may change, but they must stay fixed-decode frontend redirects. Hardware must not parse user-memory descriptors or rewrite stacks.

## ABI Contract

- Fast path: cached signature slots rename compatible integer, FP, and fixed SIMD register lanes.
- Baseline integer exchange: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` map to `x0-x7` and `a0-a7`.
- Software thunks handle stack arguments, aggregates, variadics, PLT/GOT policy, lazy binding, and incompatible vectors.
- Prototype XSAVE state: component `20`, layout version `8`, size `4096`.

## Traps

With a per-thread monitor installed, traps write an OS-neutral packet and jump to the monitor. Without one, syscall/import traps become x86 `#UD`; breakpoint traps become x86 `#BP`.

Private CPUID leaves start at `0x40000000`. Probe the live contract instead of hardcoding optional features.
