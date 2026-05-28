# Polyglot ISA Contract

This document describes the polyglot CPU contract implemented by the Bochs
prototype and the intended hardware/FPGA direction. The goal is compatibility
with existing x86_64, AArch64, and RISC-V userspace objects, not a new
compiler-only ABI.

The authoritative numeric constants used by userspace probes live in
`tools/polycpuid.h`.

## Design Goals

- x86_64 remains the boot and privileged-system ISA.
- User mode can switch into raw AArch64 or raw RISC-V fetch without a
  per-instruction trap envelope.
- Foreign code shares the x86_64 virtual address space, page tables,
  permissions, and coherent memory system.
- Cross-ISA calls preserve native ABI semantics: x86_64 SysV, AArch64
  AAPCS64, and RISC-V psABI.
- AArch64 and RISC-V can switch or call each other directly; x86_64 is not a
  mandatory routing hub after boot.
- Foreign traps are OS-neutral architectural exits. The CPU does not implement
  Linux, libc, libgcc, or dynamic-linker policy in hardware.
- Foreign architectural state is explicit XSAVE-style state, not hidden
  emulator state keyed only by CR3.

## Hardware Model

Production hardware should expose dedicated decoded x86 opcodes for:

- `PENTER.A64`: switch the frontend to raw AArch64 fetch.
- `PENTER.RV64`: switch the frontend to raw RISC-V fetch.
- `PEXIT`: switch the frontend back to x86_64 fetch.
- `PCALL.A64.SYSV`: call an AArch64 AAPCS64 target from an x86_64 SysV caller.
- `PCALL.RV64.SYSV`: call a RISC-V psABI target from an x86_64 SysV caller.
- Descriptor `PCALL` forms for aggregate, variadic, vector, import, and stack
  cases outside the fixed scalar fast path.

Every frontend transition ends the current decode block, flushes frontend
state, records a precise source and destination PC, and starts fetching in the
destination ISA. Raw AArch64 fetch is 4-byte aligned. Raw RISC-V fetch is
2-byte aligned so compressed `C` instructions are valid.

The Bochs prototype uses temporary `0f 24 ... "POLY!"` encodings. These are
not final x86 opcode allocations, but they are decoded as real instructions in
the prototype rather than routed through `UD2` exception handling.

## Prototype Opcodes

All prototype x86 opcodes are 8 bytes:

```text
0f 24 <selector> 50 4f 4c 59 21
```

The trailing bytes are the ASCII guard `POLY!`.

Core selectors:

| Selector | Meaning |
| ---: | --- |
| `0x00` | `PEXIT` to x86_64 |
| `0x01` | `PENTER.A64` |
| `0x02` | `PENTER.RV64` |
| `0x10` | `PCALL.A64.SYSV` |
| `0x11` | `PCALL.RV64.SYSV` |
| `0x12` | `PCALL.A64.SYSV.SRET` |
| `0x13` | `PCALL.RV64.SYSV.SRET` |
| `0x20` | `PIRET`, return from x86 helper to saved foreign PC |
| `0x30`-`0x37` | syscall/status reads |
| `0x38`-`0x3f` | breakpoint/status reads |
| `0x40`-`0x47` | mode and performance counter reads |
| `0x50`-`0x5d` | trap packet/status reads |
| `0x60` | trap vector set |
| `0x61` | trap vector get |
| `0x62` | trap return |
| `0x63` | trap vector frontend-mode set |
| `0x64` | trap vector frontend-mode get |
| `0x65` | explicit prototype state key set |
| `0x66` | explicit prototype state key get |
| `0x67` | explicit prototype state export |
| `0x68` | explicit prototype state import |

Selectors `0x14`-`0x1f` and `0x21`-`0x2a` are focused native ABI bridge
forms for cases such as two-float aggregate packing, heterogeneous aggregates,
FP64 overflow stack arguments, fixed 128-bit vectors, and AArch64 HFA
arguments/returns. They are prototype fast paths for common native ABI shapes;
they are not a new universal ABI.

The explicit state import layout version is `3`; the matching selector is
`0x68`.

## Native Foreign Escapes

Raw foreign modes use native encodings for direct frontend transitions:

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `brk #0x7fff` | exit to x86_64 |
| AArch64 | `brk #0x7ffe` | switch to RISC-V |
| AArch64 | `brk #0x7ffd` | call RISC-V target in `x16`, continuation in `x17` |
| AArch64 | `brk #0x7ffc` | call RISC-V with `{u32,float}` bridge |
| AArch64 | `brk #0x7ffb` | call RISC-V with `{float,u32}` bridge |
| AArch64 | `brk #0x7ffa` | call RISC-V with FP64 overflow stack bridge |
| AArch64 | `brk #0x7ff9` | trap return |
| AArch64 | `brk #0x7ff8` | call RISC-V with fixed 128-bit vector bridge |
| RISC-V | `0x0000000b` | exit to x86_64 |
| RISC-V | `0x0000002b` | switch to AArch64 |
| RISC-V | `0x0000005b` | call AArch64 target in `x5`, continuation in `x6` |
| RISC-V | `0x0000107b` | call AArch64 with `{u32,float}` bridge |
| RISC-V | `0x0000207b` | call AArch64 with `{float,u32}` bridge |
| RISC-V | `0x0000307b` | call AArch64 with FP64 overflow stack bridge |
| RISC-V | `0x0000407b` | trap return |
| RISC-V | `0x0000507b` | call AArch64 with fixed 128-bit vector bridge |

Native return instructions are valid cross-ISA returns when the link register
contains a hardware return cookie:

- AArch64 `ret x30`
- RISC-V `jalr x0, 0(ra)`
- x86_64 `ret` for descriptor-backed x86 helper imports

## Register and ABI Bridge

The external contract is native ABI compatibility. Register aliasing is an
implementation optimization, not the ABI.

Fast x86-to-foreign calls use:

- x86 target address in `R10`.
- x86 continuation in `R11`.
- optional foreign TLS base in `R13`.
- x86_64 SysV integer arguments from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`,
  plus stack slots, mapped to AArch64 `x0`-`x7` or RISC-V `a0`-`a7`.
- scalar FP arguments/results through `XMM0`-`XMM7`, AArch64 `v0`-`v7`, and
  RISC-V `fa0`-`fa7`.
- AArch64/RISC-V return lane 0 mapped to x86 `RAX`; return lane 1 mapped to
  x86 `RDX` for ordinary two-word integer returns.

Large memory-return objects use ABI-specific hidden result pointers:

- AArch64 receives the hidden result pointer in `x8`.
- RISC-V receives it in `a0`, shifting user arguments to `a1` and later.
- x86_64 receives the returned hidden pointer in `RAX`.

Descriptor-backed imports capture the first eight native foreign ABI argument
registers plus selected stack metadata. Runtime descriptors decide how those
lanes map to x86 helpers, libc/libgcc wrappers, TLS helpers, or dynamic-linker
stubs. The CPU contract is the call gate and state transition, not the helper
semantics.

## Trap Model

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

The trap packet records:

- source mode: `3` for AArch64 or `4` for RISC-V;
- trap number: syscall register, breakpoint immediate/id, import id, or raw
  illegal instruction word;
- selector/immediate where the ISA encoding provides one;
- trap PC and resume PC;
- the first eight native foreign ABI argument registers.

If a trap vector is installed, control transfers to the configured frontend
handler. Otherwise syscall/import traps surface as x86 `#UD`, and breakpoint
traps surface as x86 `#BP`. Software decides whether a trap means Linux syscall
translation, signal delivery, dynamic-linker binding, debugger handling, or
failure.

## Interrupt and Exception Contract

Asynchronous events during raw foreign fetch are precise. Hardware must:

- save the interrupted foreign frontend mode and PC;
- save foreign architectural state to the poly XSAVE component when enabled;
- enter the x86_64 interrupt/fault path with a standard x86 frame;
- restore the recorded foreign frontend on `IRET64`, `SYSRET`, `SYSEXIT`, or
  signal return when the saved state requires it.

Raw foreign execution is CPL3-only in the prototype. The x86_64 kernel remains
responsible for privilege transitions, page faults, scheduling, and signal
delivery.

## XSAVE State

Foreign architectural state is exposed as CPUID/XCR0 component `20` in the
prototype contract. The component is 4096 bytes, 64-byte aligned, little-endian,
and layout version `3`.

| Offset | Size | Contents |
| ---: | ---: | --- |
| `0x000` | `0x040` | header, current mode, flags, foreign PC, TLS base, trap vector |
| `0x040` | `0x040` | last trap packet header |
| `0x080` | `0x040` | trap argument lanes `0`-`7` |
| `0x0c0` | `0x040` | active 32-byte transition record and expansion |
| `0x100` | `0x100` | AArch64 `x0`-`x30` plus `sp` |
| `0x200` | `0x200` | AArch64 `v0`-`v31`, 128 bits each |
| `0x400` | `0x080` | AArch64 `NZCV`, `FPCR`, `FPSR`, reservation metadata |
| `0x480` | `0x100` | RISC-V `x0`-`x31`; `x0` restores as zero |
| `0x580` | `0x200` | RISC-V `f0`-`f31`, stored in 128-bit slots |
| `0x780` | `0x080` | RISC-V `fcsr`, reservation metadata |
| `0x800` | `0x500` | descriptor import return stack |
| `0xd00` | `0x300` | reserved, zero on save and ignored on restore |

This component contains only polyglot-extension state. Normal x86 GPR, RIP,
RFLAGS, x87, XMM/YMM/ZMM, PKRU, and similar state stay in their standard x86
save locations.

The current stock Linux guest enumerates this component but does not enable
`XCR0[20]`, so the prototype also supports explicit software state keys and
export/import operations. That fallback is a prototype mechanism, not the
silicon contract.

## CPUID Leaves

Private leaves start at `0x40000000` and currently extend through
`0x40000009`.

| Leaf | Purpose |
| ---: | --- |
| `0x40000000` | vendor/max leaf, vendor string `PolyglotCPU!` |
| `0x40000001` | ABI version, frontend mode mask, feature mask, XSAVE component id |
| `0x40000002` | native escape and descriptor-window discovery |
| `0x40000003` | prototype foreign-state keying and save/restore flags |
| `0x40000004` | silicon-target XSAVE component layout |
| `0x40000005` | trap packet ABI |
| `0x40000006` | interrupt/resume ABI |
| `0x40000007` | foreign memory-ordering ABI |
| `0x40000008` | cross-frontend transition ABI |
| `0x40000009` | native ABI bridge and descriptor ABI |

## Memory Model

The compatibility memory model is x86_64 TSO:

- all frontends share the coherent x86 virtual-memory path;
- AArch64 barriers are accepted as ordering-preserving no-ops under TSO;
- RISC-V fences are accepted as ordering-preserving no-ops under TSO;
- foreign atomics use coherent read-modify-write operations;
- the CPU does not inject weak AArch64/RISC-V reordering.

This choice favors correctness and interop with an x86_64 OS over faithfully
emulating weaker memory models inside one mixed-ISA process.

## Dynamic Linking and Runtime Policy

The ISA provides transitions, trap packets, explicit state, and descriptor
call gates. User-space runtime code provides policy:

- parsing foreign ELF64 images;
- mapping `PT_LOAD`, `PT_DYNAMIC`, `PT_TLS`, `PT_GNU_RELRO`;
- applying relative, symbolic, PLT/GOT, IFUNC, copy, TLS, and versioned
  relocations;
- loading `DT_NEEDED` dependencies and honoring RUNPATH/RPATH/SONAME rules;
- generating cross-ISA PLT/GOT trampolines;
- installing descriptor-backed imports for x86 helpers, libc, libgcc,
  libatomic, TLS helpers, and process/runtime queries;
- translating OS syscalls when a userspace runtime chooses to do so.

Hardware must not special-case libc names, Linux syscall numbers, libgcc helper
ids, or dynamic-linker symbol names.

## Prototype Limitations

- The x86 opcode allocation is temporary.
- The stock guest kernel does not yet enable `XCR0[20]`; explicit state keys
  are used as a tested prototype fallback.
- Focused aggregate/vector `PCALL` selectors cover observed common ABI shapes,
  not every possible C/C++ ABI case.
- `tools/contracts/` contains older regex checks; real boot tests are the
  preferred validation path.

## Compatibility Rule

Precompiled cross-ISA libraries remain native ABI objects. Fast hardware
transitions reduce thunk cost, but software descriptors or generated thunks are
still required for ABI cases that cannot be represented by a fixed hardware
shuffle. The target is minimal thunking, not zero metadata.
