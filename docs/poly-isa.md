# Poly ISA

Poly ISA runs existing precompiled x86_64, AArch64, and RISC-V64 userspace code in one x86_64 virtual address space.

Detailed rationale: `docs/poly-isa-design-directions.md`. Live constants: `tools/include/polycpuid.h`.

## Run

```sh
make image
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## ISA Delta

- x86_64 remains the system frontend for boot, privilege, paging, interrupts, and faults.
- Userspace can redirect fetch/decode to raw AArch64 or RISC-V64 frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- Foreign instructions are direct-fetched from `RIP`; there are no per-instruction `#UD` envelopes.
- Control operations are `PENTER`, `PSWITCH`, `PCALL`, `PCALL ... sig`, and `PIRET`.
- Every frontend switch ends the current decode block; AArch64 fetch is 4-byte aligned and RISC-V fetch is 2-byte aligned.
- Prototype encodings are x86 `0f 3a fc <subop>`, AArch64 reserved `HINT`, and RISC-V `custom-0`.

## ABI And State

- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.
- Fast path: cached signature slots rename compatible integer, FP, and fixed-SIMD ABI registers.
- Baseline exchange: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` map to `x0-x7` and `a0-a7`.
- Software thunks handle stack arguments, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vectors.
- Native returns cross frontends through `PCALL` return cookies.
- Extra frontend state is explicit XSAVE-style architectural state: prototype component `20`, layout version `8`, size `4096`.

## Traps

The ISA is OS-neutral. Hardware does not emulate Linux syscalls, libc, imports, or stack layouts. With a per-thread monitor installed, traps write a packet and jump to userspace; otherwise syscall/import traps become x86 `#UD` and breakpoint traps become x86 `#BP`.
