# Poly ISA

Short operating reference for the prototype Poly ISA extension.

Goal: run existing precompiled x86_64, AArch64, and RISC-V64 userspace objects
in one x86_64 virtual address space. This is native ABI compatibility, not a
new compiler-only ABI target.

Implementation constants live in `tools/include/polycpuid.h`. Longer rationale
and silicon notes live in `docs/poly-isa-design-directions.md`.

## Run It

Build:

```sh
make image
```

Useful boot gates:

```sh
make boot-poly-binfmt-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps
```

Check logs:

```sh
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, virtual memory, atomics, and TSO memory ordering.
- CPL3 code can switch the instruction frontend to raw AArch64 or raw RISC-V64.
- Foreign code is fetched directly from `RIP`; there is no per-instruction
  `#UD` envelope.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Non-x86 architectural state is explicit XSAVE-style state, not hidden
  CR3-scoped emulator state.
- Hardware does not emulate Linux, libc, libgcc, or libatomic. Foreign syscalls,
  breakpoints, illegal instructions, unsupported operations, and unresolved
  imports produce OS-neutral trap records.
- The hardware boundary is intentionally small: frontend switch, register alias
  signatures, return cookies, trap packets, and XSAVE state.
- The loader/runtime handles ELF loading, relocations, PLT/GOT binding, TLS,
  syscall policy, libc helpers, stack arguments, aggregates, variadics, and
  incompatible vector layouts.

## Core ISA Model

Frontend IDs are `0=x86_64`, `1=AArch64`, and `2=RISC-V64`.

Operations:

- `PENTER frontend`: enter a raw foreign frontend from x86_64/system code.
- `PSWITCH frontend, target`: branch to another frontend without a return
  cookie.
- `PCALL frontend, target[, slot]`: apply a register-only ABI signature, push
  transition state, install a native return cookie, and branch.
- `PIRET`: restore a previously interrupted frontend after trap handling.

Rules:

- AArch64 fetch is 4-byte aligned.
- RISC-V fetch is 2-byte aligned so compressed instructions remain valid.
- Every transition ends the current decode block and records precise source and
  destination PCs.
- Native return instructions may cross frontends only when returning to a
  hardware return cookie installed by `PCALL`.

## Prototype Controls

The Bochs prototype uses temporary decoded x86 controls:

```text
0f 3a fc <subop>
```

This is not an exception path. A silicon allocation can change the bytes, but
must preserve fixed decode, frontend redirect, no user-memory descriptor
parsing, and no stack rewriting.

Foreign exit/switch/call controls use reserved AArch64 `HINT` encodings and
RISC-V64 `custom-0` encodings. Exact opcodes, subops, and CPUID feature bits are
defined in `tools/include/polycpuid.h`.

## ABI And State

The baseline exchange window aliases hot integer lanes: `RAX/RDX/RCX/RDI/RSI/R8/R9/R10`
to AArch64 `x0..x7` and RISC-V `a0..a7`.

Signature slots are cached register-renaming templates selected by `PCALL`.
They may remap compatible integer, FP, and fixed SIMD ABI lanes without move
instructions or memory access. They must not describe stack layouts, by-value
aggregates, variadic metadata, PLT/GOT policy, or lazy binding.

FP lanes `XMM0..XMM7`, AArch64 `v0..v7`, and RISC-V `fa0..fa7` are bridged for
register-only calls. Wider AVX, SVE, RVV, stack FP overflow, incompatible
vectors, aggregates, and variadics require runtime thunks.

Poly state is exposed as XCR0 component `20`, layout version `8`, 4096 bytes in
the current prototype.

## Traps

If a per-thread user-space monitor is installed, hardware writes trap packets
there and transfers to the Ring 3 monitor. Otherwise syscall/import traps
surface as x86 `#UD`; breakpoint traps surface as x86 `#BP`.

Private CPUID leaves start at `0x40000000`; probe the live contract instead of
hardcoding optional feature assumptions.
