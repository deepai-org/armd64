# Poly ISA Contract

Hardware-facing contract for running existing precompiled x86_64, AArch64, and
RISC-V userspace objects in one x86_64 virtual address space. This is native ABI
compatibility, not a new compiler-only ABI. Authoritative constants live in
`tools/include/polycpuid.h`.

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, atomics, virtual memory, and TSO ordering.
- CPL3 code can switch the frontend into raw AArch64 or raw RISC-V fetch.
- Foreign instructions are fetched directly. There is no per-instruction `#UD`
  envelope.
- Cross-ISA calls preserve x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI.
- Foreign register state is explicit XSAVE-style architectural state.
- Foreign syscalls, breakpoints, illegal instructions, unsupported operations,
  and unresolved imports become OS-neutral trap records.

## Frontend Transitions

Silicon or FPGA implementations should allocate real decoded x86 opcodes for:

- `PENTER.A64`, `PENTER.RV64`: enter raw foreign fetch.
- `PEXIT`: return to x86_64 fetch.
- `PCALL.A64.SYSV`, `PCALL.RV64.SYSV`: fixed native-ABI calls from x86_64.
- Descriptor `PCALL`: stack args, aggregate returns, variadics, vectors,
  callbacks, helper imports, PLT/GOT trampolines, and lazy binding.

Every transition ends the current decode block and records precise source and
destination PCs. AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned
so compressed instructions remain valid.

The Bochs prototype currently uses temporary decoded x86 opcodes:

```text
0f 24 <selector> 50 4f 4c 59 21
```

The trailer is ASCII `POLY!`. Selectors include `0x00` `PEXIT`, `0x01`
`PENTER.A64`, `0x02` `PENTER.RV64`, `0x10`-`0x13` fixed native-ABI `PCALL`,
`0x20` `PIRET`, and `0x30`-`0x68` state, trap, import/export, and descriptor
operations.

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

The externally visible rule is native ABI compatibility. Register aliasing is
only an implementation optimization.

Fixed x86-to-foreign calls map x86_64 SysV integer arguments from `RDI`, `RSI`,
`RDX`, `RCX`, `R8`, `R9`, plus stack slots, to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`. FP args/results map through `XMM0`-`XMM7`, AArch64 `v0`-`v7`, and
RISC-V `fa0`-`fa7`. Return lane 0 maps to `RAX`; return lane 1 maps to `RDX`.

Large memory returns follow the target ABI. AArch64 uses `x8`; RISC-V uses
`a0` and shifts user arguments; x86_64 returns the hidden pointer in `RAX`.
Descriptor calls carry the metadata needed for nontrivial ABI cases.

## Traps

Foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved descriptor imports,
unsupported instructions, and illegal instructions produce a `POLYTRAP` packet.
Reasons are `0` none, `1` syscall, `2` breakpoint, `3` unresolved import, and
`4` illegal or unsupported instruction.

The packet records source mode, trap number, selector/immediate, trap PC,
resume PC, eight generic argument lanes, and the first eight native foreign ABI argument registers.

If a trap vector is installed, control transfers there. Otherwise syscall and
import traps surface as x86 `#UD`, and breakpoint traps surface as x86 `#BP`.
OS or userspace runtime code decides syscall translation, signal delivery, lazy
binding, debugger handling, or failure policy.

## State And Interrupts

Asynchronous events during foreign fetch are precise: hardware saves interrupted
frontend mode and PC, saves enabled poly state through the poly XSAVE component,
enters the standard x86_64 interrupt/fault path, then `IRET64`, `SYSRET`,
`SYSEXIT`, or signal return restores the recorded foreign frontend when needed.

The prototype CPUID contract exposes poly state as XCR0 component `20`.
Component layout version `3` is 4096 bytes and contains the mode header, trap
packet, active transition record, AArch64 GPR/FP state, RISC-V GPR/FP state,
and descriptor import return stack.

Private CPUID leaves start at `0x40000000` and currently extend through
`0x40000009`. The prototype software state import layout version is `3`; it is
a Bochs fallback path, not the silicon context-switch contract.

## Runtime Boundary

Hardware provides frontend transitions, trap packets, explicit state, and
descriptor call gates. Userspace runtime code provides ELF loading,
relocations, PLT/GOT binding, IFUNC, TLS, dependency policy, generated thunks,
syscall translation for a chosen OS ABI, and libc/libgcc/libatomic helper
semantics.

## Validation

Prefer real boot tests:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

Scripts under `scripts/checks/` are coarse consistency smoke tests only.
