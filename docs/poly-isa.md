# Poly ISA Contract

This is the CPU contract for the Bochs prototype and the intended
hardware/FPGA direction. The goal is compatibility with existing precompiled
x86_64, AArch64, and RISC-V userspace objects, including cross-ISA shared
libraries. This is not a new compiler-only ABI.

Authoritative userspace constants live in `tools/include/polycpuid.h`.

## Difference From x86_64

- x86_64 remains the boot, kernel, interrupt, fault, page-table, and privileged
  ISA.
- CPL3 code can switch the instruction frontend to raw AArch64 or raw RISC-V.
- Foreign instructions are fetched directly. There is no per-instruction
  exception envelope.
- All frontends share the same x86_64 virtual address space, permissions,
  cache coherence, and TSO memory model.
- Cross-ISA calls preserve real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Foreign state is explicit XSAVE-style architectural state. Hidden CR3-only
  emulator state is not the hardware contract.
- Foreign syscalls, breakpoints, illegal instructions, unsupported operations,
  and unresolved imports produce neutral trap records. Hardware does not know
  Linux, libc, libgcc, symbol names, or dynamic-linker policy.

## Frontend Transitions

Production hardware should allocate real decoded x86 opcodes for:

- `PENTER.A64`: enter raw AArch64 fetch.
- `PENTER.RV64`: enter raw RISC-V fetch.
- `PEXIT`: return to x86_64 fetch.
- `PCALL.A64.SYSV`: call an AArch64 AAPCS64 target from x86_64 SysV.
- `PCALL.RV64.SYSV`: call a RISC-V psABI target from x86_64 SysV.
- Descriptor `PCALL` forms for stack arguments, aggregate returns, variadics,
  fixed vectors, helper imports, PLT/GOT trampolines, and dynamic-linker cases.

Every transition terminates the current decode block, records precise source
and destination PCs, and restarts fetch in the destination frontend. AArch64
fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned so compressed
instructions remain valid.

The Bochs prototype uses temporary 8-byte decoded instructions:

```text
0f 24 <selector> 50 4f 4c 59 21
```

The trailer is ASCII `POLY!`. These are prototype instructions, not `UD2`
exception envelopes. Final hardware should use allocated opcode space.

Core selectors:

| Selector | Meaning |
| ---: | --- |
| `0x00` | `PEXIT` |
| `0x01` | `PENTER.A64` |
| `0x02` | `PENTER.RV64` |
| `0x10` | `PCALL.A64.SYSV` |
| `0x11` | `PCALL.RV64.SYSV` |
| `0x12` | `PCALL.A64.SYSV.SRET` |
| `0x13` | `PCALL.RV64.SYSV.SRET` |
| `0x20` | `PIRET`, return from x86 helper to saved foreign PC |
| `0x30`-`0x68` | state, trap-vector, export/import, and descriptor selectors |

## Native Foreign Escapes

Raw foreign modes use native encodings for direct exits and cross-frontend
transitions:

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

Additional reserved escape encodings handle ABI-specific bridges for common
aggregate and vector shapes. Native return instructions may also cross
frontends when the link register or stack return slot contains a hardware
return cookie.

## ABI Bridge

The external contract is native ABI compatibility. Register aliasing is only an
implementation optimization.

Fixed x86-to-foreign calls use:

- target address in `R10`;
- x86 continuation in `R11`;
- optional foreign TLS base in `R13`;
- x86_64 SysV integer arguments from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`,
  plus stack slots, mapped to AArch64 `x0`-`x7` or RISC-V `a0`-`a7`;
- FP arguments/results through `XMM0`-`XMM7`, AArch64 `v0`-`v7`, and RISC-V
  `fa0`-`fa7`;
- return lane 0 to `RAX` and return lane 1 to `RDX`.

Large memory returns follow the target ABI: AArch64 uses `x8`, RISC-V uses
`a0` and shifts user arguments, and x86_64 returns the hidden pointer in `RAX`.

Descriptor-backed calls carry the metadata needed for libc/libgcc wrappers,
TLS helpers, PLT/GOT trampolines, variadics, aggregate returns, stack
arguments, callback imports, and lazy binding. The CPU provides the fast call
gate and precise state transition; userspace supplies symbol policy.

## Traps and OS Policy

Foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved descriptor imports,
unsupported instructions, and illegal instructions produce a `POLYTRAP`
packet.

Trap reasons:

| Reason | Meaning |
| ---: | --- |
| `0` | no trap |
| `1` | syscall trap |
| `2` | breakpoint trap |
| `3` | unresolved descriptor import |
| `4` | illegal or unsupported foreign instruction |

The packet records source mode, trap number, selector/immediate, trap PC,
resume PC, eight generic argument lanes, and the first eight native foreign ABI
argument registers.

If a trap vector is installed, control transfers to that handler. Otherwise
syscall/import traps surface as x86 `#UD`, and breakpoint traps surface as x86
`#BP`. The OS or userspace runtime decides whether the trap means Linux syscall
translation, signal delivery, lazy binding, debugger handling, or failure.

## Interrupts and State

Asynchronous events during foreign fetch are precise:

- hardware saves interrupted frontend mode and PC;
- enabled poly state is saved through the poly XSAVE component;
- control enters the standard x86_64 interrupt/fault path;
- `IRET64`, `SYSRET`, `SYSEXIT`, or signal return restores the recorded
  foreign frontend when the saved state requires it.

The prototype CPUID contract exposes poly state as XCR0 component `20`.
Component layout version `3` is 4096 bytes and contains the mode header, trap
packet, active transition record, AArch64 GPR/FP state, RISC-V GPR/FP state,
and descriptor import return stack. Normal x86 GPR, RIP, RFLAGS, x87,
XMM/YMM/ZMM, PKRU, and similar state remain in their standard x86 save
locations.

Private CPUID leaves start at `0x40000000` and currently extend through
`0x40000009`. They advertise vendor, ABI version, frontend support, native
escape encodings, trap layout, interrupt/resume behavior, memory ordering,
transition ABI, descriptor ABI, and the XSAVE component. Exact constants are in
`tools/include/polycpuid.h`.

The prototype software state import/export fallback state import layout version is `3`; it is not the silicon context-switch contract.

## Runtime Responsibilities

Hardware provides frontend transitions, trap packets, explicit state, and
descriptor call gates. Userspace runtime code provides:

- foreign ELF parsing and segment mapping;
- dynamic relocations, PLT/GOT binding, IFUNC, TLS, copy relocs, and symbol
  versioning;
- `DT_NEEDED`, RPATH/RUNPATH, SONAME, preload, and dependency policy;
- generated ABI thunks and cross-ISA trampolines;
- syscall translation for a chosen OS ABI;
- libc, libgcc, libatomic, TLS, and process-query helper semantics.

Validation should use real boot tests first:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`
