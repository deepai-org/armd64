# Polyglot ISA Contract

This document separates the architectural target from the current Bochs
prototype.  The target is compatibility with existing precompiled AArch64 and
RISC-V code linked into an x86_64 process, not a new compiler-only ABI.

## Hardware Contract

Production hardware should not use `#UD` envelopes for hot operations.  The
Bochs envelopes are prototype encodings only.  A silicon or FPGA
implementation should expose CPUID-gated x86 instructions for:

- `PENTER.A64`: enter fixed-width AArch64 fetch at the next byte.
- `PENTER.RV64`: enter fixed-width RISC-V fetch at the next byte.
- `PEXIT`: return to x86_64 fetch without taking an exception.
- Optional call helpers only if they preserve native ABI return semantics.

Foreign architectural state must be explicit.  Non-aliased AArch64 and RISC-V
registers are not hidden CR3/FSBASE maps in the ISA contract.  A hardware
implementation should expose CPUID feature bits plus XCR0 state components so
the OS can save and restore foreign integer and FP/SIMD state with
`XSAVE`/`XRSTOR`.

Foreign traps are architectural exits, not hardware libcalls or Linux policy.
When foreign code executes AArch64 `svc`, RISC-V `ecall`, AArch64 `brk`, or
RISC-V `ebreak`, hardware records a trap frame, switches the frontend back to
x86_64, and transfers control to an OS/runtime-defined trap path.  The OS or
userspace runtime decides whether to translate a syscall, deliver a signal,
invoke a thunk, or reject the operation.

Interrupts, faults, and debug exceptions taken during foreign fetch must record
the interrupted foreign mode and PC before entering the x86_64 kernel path.
`IRET`/`SYSRET`/signal return must restore the foreign frontend mode when the
saved architectural state requires it.

## Bochs Prototype Contract

Bochs still uses fixed 8-byte invalid x86 envelopes because that is the least
invasive way to prototype the frontend switch:

- `65 0f 0b 52 41 57 36 34`: enter raw AArch64 fetch.
- `66 0f 0b 52 41 57 52 56`: enter raw RISC-V fetch.
- `64 0f 0b 58 4d 4f 44 45`: return to x86_64 mode.

The prototype now records a unified `POLYTRAP` state before running any
compatibility dispatcher:

- Reason `0`: no trap.
- Reason `1`: foreign syscall trap (`svc` or `ecall`).
- Reason `2`: foreign breakpoint trap (`brk` or `ebreak`).
- Mode records the raw source mode: `3` for AArch64, `4` for RISC-V.
- Number records the syscall number or breakpoint immediate/id.
- Arguments record the native foreign ABI argument registers.
- PC records the foreign instruction address that raised the trap.

The temporary deterministic syscall and libcall behavior in Bochs is a
compatibility runtime layered after this trap record.  It is not the final ISA
contract.  The final contract is the precise trap exit plus explicit state that
software can save, restore, inspect, and route.

## Compatibility Rule

Precompiled cross-ISA libraries remain native ABI objects.  Boundary thunks are
responsible for mapping x86_64 SysV arguments to AAPCS64 or RISC-V psABI
registers, setting a native return target, entering raw fetch, and handling trap
exits.  Direct register aliasing is an implementation optimization, not the
external ABI.
