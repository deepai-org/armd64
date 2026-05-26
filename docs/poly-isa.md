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
- `PCALL.A64.SYSV`: call an AArch64 AAPCS64 target from an x86_64 SysV caller.
- `PCALL.RV64.SYSV`: call a RISC-V psABI target from an x86_64 SysV caller.
- Descriptor-based `PCALL` forms for stack arguments, aggregates, variadics,
  unwind metadata, and other cases that cannot be encoded by one fixed shuffle.

The fixed `PCALL` fast path is an architectural ABI bridge, not a custom ABI.
For common scalar calls, hardware maps x86_64 SysV integer arguments into the
native foreign argument registers, preserves the shared FP argument registers,
sets the foreign link register to a return cookie, switches the frontend, and
starts fetching at the foreign target.  A normal foreign return instruction to
that cookie restores x86_64 fetch at the saved continuation and maps the native
return registers back to the x86_64 result registers.

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
- `40 0f 0b 50 43 41 36 34`: prototype `PCALL.A64.SYSV`.
- `40 0f 0b 50 43 52 56 36`: prototype `PCALL.RV64.SYSV`.

The prototype `PCALL` forms use `R10` as the foreign target address and `R11`
as the x86_64 return continuation.  They currently cover the common register
fast path: x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` plus stack
slots `[RSP+8]` and `[RSP+16]` are mapped to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`; `XMM0`-`XMM7` remain aliased to AArch64 `d0`-`d7` or RISC-V
`fa0`-`fa7`; and AArch64 `ret x30` or RISC-V `jalr x0, 0(ra)` returns through
a cookie to the saved x86 continuation.
The `polycall` guest tool exercises this path against loaded foreign ELF64
function payloads rather than inline x86-hosted instruction blobs.

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
exits.  The target ISA makes the common thunk operation a fast hardware `PCALL`;
software descriptors or thunks still handle ABI cases outside the fixed fast
path.  Direct register aliasing is an implementation optimization, not the
external ABI.
