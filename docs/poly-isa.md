# Poly ISA Contract

This is the hardware-facing contract exercised by the Bochs prototype. The
goal is compatibility with existing precompiled x86_64, AArch64, and RISC-V
userspace objects, including cross-ISA shared libraries. It is not a new
compiler-only ABI.

Authoritative constants live in `tools/include/polycpuid.h`.

## What Differs From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, atomics, virtual memory, and TSO ordering.
- CPL3 code can switch fetch/decode into raw AArch64 or raw RISC-V.
- Foreign instructions are fetched directly from the same x86_64 virtual
  address space; there is no per-instruction exception envelope.
- Cross-ISA calls preserve native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Foreign architectural state is explicit XSAVE-style state, not hidden
  CR3-scoped emulator state.
- Syscalls, breakpoints, illegal instructions, unsupported operations, and
  unresolved imports become OS-neutral trap records. Hardware does not know
  Linux, libc, libgcc, symbol names, or dynamic-linker policy.

## Frontend Switches

Final silicon/FPGA designs should allocate real decoded x86 opcodes for:

- `PENTER.A64`, `PENTER.RV64`: enter raw foreign fetch.
- `PEXIT`: return to x86_64 fetch.
- `PCALL.A64.SYSV`, `PCALL.RV64.SYSV`: native-ABI calls from x86_64.
- Descriptor `PCALL` forms for stack args, aggregate returns, variadics,
  vectors, callbacks, helper imports, PLT/GOT trampolines, and dynamic linker
  cases.

Every transition ends the current decode block and records precise source and
destination PCs. AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned
so compressed instructions remain valid.

The Bochs prototype uses temporary decoded 8-byte instructions:

```text
0f 24 <selector> 50 4f 4c 59 21
```

The trailer is ASCII `POLY!`. These are prototype opcodes, not `UD2`
exception envelopes.

Important selectors:

| Selector | Meaning |
| ---: | --- |
| `0x00` | `PEXIT` |
| `0x01` | `PENTER.A64` |
| `0x02` | `PENTER.RV64` |
| `0x10`-`0x13` | fixed native-ABI `PCALL` forms |
| `0x20` | `PIRET`, return from x86 helper to saved foreign PC |
| `0x30`-`0x68` | state, trap, import/export, and descriptor selectors |

## Foreign Escapes

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `brk #0x7fff` | exit to x86_64 |
| AArch64 | `brk #0x7ffe` | switch to RISC-V |
| AArch64 | `brk #0x7ffd` | call RISC-V target in `x16`, continuation in `x17` |
| AArch64 | `brk #0x7ff9` | trap return |
| RISC-V | `0x0000000b` | exit to x86_64 |
| RISC-V | `0x0000002b` | switch to AArch64 |
| RISC-V | `0x0000005b` | call AArch64 target in `x5`, continuation in `x6` |
| RISC-V | `0x0000407b` | trap return |

Native return instructions may cross frontends when the link register or stack
return slot contains a hardware return cookie.

## ABI Bridge

The external contract is native ABI compatibility. Register aliasing is only an
implementation optimization.

Fixed x86-to-foreign calls map x86_64 SysV integer arguments from `RDI`, `RSI`,
`RDX`, `RCX`, `R8`, `R9`, plus stack slots, to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`. FP arguments and results map through `XMM0`-`XMM7`, AArch64
`v0`-`v7`, and RISC-V `fa0`-`fa7`. Return lane 0 maps to `RAX`; return lane 1
maps to `RDX`.

Large memory returns follow the target ABI. AArch64 uses `x8`; RISC-V uses
`a0` and shifts user arguments; x86_64 returns the hidden pointer in `RAX`.

Descriptor-backed calls carry metadata for stack arguments, aggregate returns,
variadics, callbacks, helper imports, TLS helpers, PLT/GOT trampolines, lazy
binding, and dynamic-linker policy.

## Traps

Foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved descriptor imports,
unsupported instructions, and illegal instructions produce a `POLYTRAP`
packet.

| Reason | Meaning |
| ---: | --- |
| `0` | no trap |
| `1` | syscall trap |
| `2` | breakpoint trap |
| `3` | unresolved descriptor import |
| `4` | illegal or unsupported foreign instruction |

The packet records source mode, trap number, selector/immediate, trap PC,
resume PC, eight generic argument lanes, and the first eight native foreign ABI argument registers.

If a trap vector is installed, control transfers to that handler. Otherwise
syscall/import traps surface as x86 `#UD`, and breakpoint traps surface as x86
`#BP`. OS or userspace runtime code decides syscall translation, signal
delivery, lazy binding, debugger handling, or failure policy.

## State

Asynchronous events during foreign fetch are precise: hardware saves interrupted
frontend mode and PC, saves enabled poly state through the poly XSAVE component,
enters the standard x86_64 interrupt/fault path, then `IRET64`, `SYSRET`,
`SYSEXIT`, or signal return restores the recorded foreign frontend when needed.

The prototype CPUID contract exposes poly state as XCR0 component `20`.
Component layout version `3` is 4096 bytes and contains the mode header, trap
packet, active transition record, AArch64 GPR/FP state, RISC-V GPR/FP state,
and descriptor import return stack.

The prototype software state import layout version is `3`; it is a Bochs
fallback path, not the silicon context-switch contract.

Private CPUID leaves start at `0x40000000` and currently extend through
`0x40000009`.

## Runtime Boundary

Hardware provides frontend transitions, trap packets, explicit state, and
descriptor call gates. Userspace runtime code provides ELF loading, relocations,
PLT/GOT binding, IFUNC, TLS, dependency policy, generated thunks, syscall
translation for a chosen OS ABI, and libc/libgcc/libatomic helper semantics.

## Validation

Prefer real boot tests:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

The scripts under `tools/contracts/` are coarse consistency checks only.
