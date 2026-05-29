# Poly ISA

Short reference for the prototype ISA extension used by this repo. The goal is
running existing x86_64, AArch64, and RISC-V64 userspace objects in one x86_64
virtual address space. This is native ABI compatibility, not a new compiler-only
ABI.

Implementation constants live in `tools/include/polycpuid.h`. Longer design
rationale lives in `docs/poly-isa-design-directions.md`.

## Run It

Build the image:

```sh
make image
```

Useful boot gates:

```sh
make boot-poly-binfmt-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps
```

Quick log check after a boot:

```sh
rg -n "POLY.*(OK|FAIL)|Kernel panic|Segmentation fault|BUG:" out/serial.log out/bochs*.log
```

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, virtual memory, atomics, and TSO memory ordering.
- CPL3 code can switch the instruction frontend to raw AArch64 or raw RISC-V64.
- Foreign instructions are fetched directly. There is no per-instruction `#UD`
  envelope.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Foreign architectural state is explicit XSAVE-style state, not hidden
  CR3-scoped emulator state.
- Foreign syscalls, breakpoints, illegal instructions, unsupported operations,
  and unresolved imports become OS-neutral trap records.
- The hardware boundary is deliberately small: frontend switch, register alias
  signatures, return cookies, trap packets, and XSAVE state.
- The loader/runtime still handles ELF loading, relocations, PLT/GOT binding,
  TLS, syscall translation policy, libc helpers, stack arguments, aggregates,
  variadics, and incompatible vector layouts.

## Frontends

Frontend IDs:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned so compressed
instructions remain valid.

The architectural operations are:

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a raw foreign frontend from x86_64/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without a return cookie. |
| `PCALL frontend, target[, slot]` | Apply an optional register-only ABI signature, push hardware transition-stack state, install a native return cookie, and branch. |
| `PIRET` | Restore a previously interrupted frontend after trap handling. |

Every transition ends the current decode block and records precise source and
destination PCs.

## Prototype Encodings

The Bochs prototype uses a temporary decoded x86 control page:

```text
0f 3a fc <subop>
```

This is not an exception path. A silicon allocation can change the bytes, but it
should preserve fixed decode, frontend redirect, no user-memory descriptor
parsing, and no stack rewriting.

Important x86 subops:

| Subop | Operation | Inputs |
| --- | --- | --- |
| `0x03` | `PENTER_MODE` | `R15=frontend` |
| `0x04` | `PSWITCH_MODE` | `R15=frontend`, `RBX=target` |
| `0x2d` | `PCALL_SIG_MODE` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=slot` |
| `0x2e <slot>` | `PCALL_SIG_IMM_MODE` | `R15=frontend`, `RBX=target`, `R11=return` |
| `0x69..0x6e` | state controls | ABI signature, monitor packet, and landing policy get/set |

Foreign controls are reserved fixed-width instructions:

| Frontend | Exit | Switch | Call | Call With Slot |
| --- | --- | --- | --- | --- |
| AArch64 | `0xd5032e1f` | `0xd5032f1f` with `x16=target`, `x17=frontend` | `0xd5032f3f` with `x18=return` | `0xd5032c1f + (slot << 5)` |
| RISC-V64 | `0x0000700b` | `0x1000700b` with `x5=target`, `x6=frontend` | `0x1200700b` with `x7=return` | `0x2000700b + (slot << 25)` |

AArch64 `BRK` and RISC-V `EBREAK` are normal trap exits for debugger/runtime
policy. Native return instructions may cross frontends when they return to a
hardware return cookie installed by `PCALL`.

## ABI Bridge

The baseline exchange window aliases the common hot argument/result lanes:

| Lane | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0` | `RAX` | `x0` | `a0` |
| `P1` | `RDX` | `x1` | `a1` |
| `P2` | `RCX` | `x2` | `a2` |
| `P3` | `RDI` | `x3` | `a3` |
| `P4` | `RSI` | `x4` | `a4` |
| `P5` | `R8` | `x5` | `a5` |
| `P6` | `R9` | `x6` | `a6` |
| `P7` | `R10` | `x7` | `a7` |

Signature slots are cached register-renaming templates selected by `PCALL`.
They may remap compatible integer, FP, and fixed SIMD ABI lanes without move
instructions or memory access. They must not describe stack layouts, by-value
aggregates, variadic metadata, PLT/GOT policy, or lazy binding.

Default prototype slots:

| Slot | Kind |
| --- | --- |
| `0` | baseline exchange window |
| `1` | x86 SysV register-only |
| `2` | x86 SysV register-only with two-GPR integer return |
| `3` | preferred native-register mapping |
| `4` | preferred native-register mapping with two-GPR integer return |

FP lanes `XMM0..XMM7`, AArch64 `v0..v7`, and RISC-V `fa0..fa7` are bridged for
register-only calls. Wider AVX, SVE, RVV, stack FP overflow, incompatible
vectors, aggregates, and variadics require runtime thunks.

## Traps, Syscalls, And State

Hardware does not emulate Linux, libc, libgcc, or libatomic. It emits precise
trap records for foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved imports,
illegal instructions, and unsupported instructions.

If a per-thread user-space monitor is installed, hardware writes the trap packet
there and transfers to the Ring 3 monitor. Otherwise syscall/import traps surface
as x86 `#UD`; breakpoint traps surface as x86 `#BP`.

Asynchronous events during foreign fetch are precise. Hardware records the
interrupted frontend mode and PC, enters the normal x86_64 interrupt/fault path,
and restores the recorded foreign frontend on architectural return when needed.

The prototype exposes Poly state as XCR0 component `20`, layout version `8`,
4096 bytes. It contains mode/trap state, transition-stack state, AArch64 and
RISC-V GPR/FP state, monitor registers, ABI signature slots, frontend TLS, and
landing-policy flags.

Private CPUID leaves start at `0x40000000`; probe the live contract instead of
hardcoding optional feature assumptions.
