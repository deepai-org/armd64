# Polyglot ISA Contract

This is the working CPU contract for the Bochs prototype and the intended
hardware/FPGA direction. The goal is compatibility with existing precompiled
x86_64, AArch64, and RISC-V userspace objects, including linked cross-ISA
shared libraries. It is not a new compiler-only ABI.

Authoritative userspace constants live in `tools/include/polycpuid.h`.

## What Changes From x86_64

- x86_64 remains the boot, kernel, and privileged-system ISA.
- CPL3 code can switch the instruction frontend to raw AArch64 or raw RISC-V.
- All frontends use the same x86_64 virtual address space, page tables, page
  permissions, cache coherence, and TSO memory model.
- Cross-ISA calls preserve the real source and target ABIs: x86_64 SysV,
  AArch64 AAPCS64, and RISC-V psABI.
- Foreign architectural state is explicit XSAVE-style state, not hidden state
  keyed only by CR3.
- Foreign syscalls, breakpoints, illegal instructions, and unresolved imports
  produce neutral trap packets. Hardware does not implement Linux, libc,
  libgcc, or dynamic-linker policy.

## Hardware Shape

Production hardware should allocate dedicated decoded x86 opcodes for frontend
switches and cross-ISA call gates:

- `PENTER.A64`: enter raw AArch64 fetch.
- `PENTER.RV64`: enter raw RISC-V fetch.
- `PEXIT`: return to x86_64 fetch.
- `PCALL.A64.SYSV`: call an AArch64 AAPCS64 target from x86_64 SysV.
- `PCALL.RV64.SYSV`: call a RISC-V psABI target from x86_64 SysV.
- Descriptor `PCALL` forms for ABI cases that need metadata: aggregate
  returns, stack arguments, variadics, vectors, helper imports, and dynamic
  linker trampolines.

Each transition terminates the current decode block, records precise source and
destination PCs, and restarts fetch in the destination ISA. AArch64 fetch is
4-byte aligned. RISC-V fetch is 2-byte aligned so compressed `C` instructions
remain valid.

The Bochs prototype uses temporary 8-byte opcodes:

```text
0f 24 <selector> 50 4f 4c 59 21
```

The trailer is ASCII `POLY!`. These are decoded as prototype instructions, not
as `UD2` exception envelopes. Final silicon should use real allocated opcode
space.

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
| `0x30`-`0x68` | status, trap-vector, state-key, export/import selectors |

Selectors `0x14`-`0x1f` and `0x21`-`0x2a` are focused ABI fast paths for common
native shapes such as HFA, heterogeneous aggregates, FP64 stack overflow, and
fixed 128-bit vectors.

## Native Foreign Escapes

Raw foreign modes use native instruction encodings for direct transitions:

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `brk #0x7fff` | exit to x86_64 |
| AArch64 | `brk #0x7ffe` | switch to RISC-V |
| AArch64 | `brk #0x7ffd` | call RISC-V target in `x16`, continuation in `x17` |
| AArch64 | `brk #0x7ffc` | call RISC-V with `{u32,float}` bridge |
| AArch64 | `brk #0x7ffb` | call RISC-V with `{float,u32}` bridge |
| AArch64 | `brk #0x7ffa` | call RISC-V with FP64 stack bridge |
| AArch64 | `brk #0x7ff9` | trap return |
| AArch64 | `brk #0x7ff8` | call RISC-V with fixed 128-bit vector bridge |
| RISC-V | `0x0000000b` | exit to x86_64 |
| RISC-V | `0x0000002b` | switch to AArch64 |
| RISC-V | `0x0000005b` | call AArch64 target in `x5`, continuation in `x6` |
| RISC-V | `0x0000107b` | call AArch64 with `{u32,float}` bridge |
| RISC-V | `0x0000207b` | call AArch64 with `{float,u32}` bridge |
| RISC-V | `0x0000307b` | call AArch64 with FP64 stack bridge |
| RISC-V | `0x0000407b` | trap return |
| RISC-V | `0x0000507b` | call AArch64 with fixed 128-bit vector bridge |

Native return instructions can cross frontends when the link register contains
a hardware return cookie:

- AArch64 `ret x30`
- RISC-V `jalr x0, 0(ra)`
- x86_64 `ret` for descriptor-backed x86 helper imports

## ABI Bridge

The external contract is native ABI compatibility. Register aliasing is only an
implementation optimization.

Fixed x86-to-foreign calls use:

- x86 target address in `R10`;
- x86 continuation in `R11`;
- optional foreign TLS base in `R13`;
- x86_64 SysV integer arguments from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, `R9`
  plus stack slots, mapped to AArch64 `x0`-`x7` or RISC-V `a0`-`a7`;
- FP arguments/results through `XMM0`-`XMM7`, AArch64 `v0`-`v7`, and RISC-V
  `fa0`-`fa7`;
- return lane 0 to x86 `RAX`; return lane 1 to x86 `RDX`.

Large memory returns follow the target ABI:

- AArch64 receives the hidden result pointer in `x8`.
- RISC-V receives it in `a0`, shifting user arguments to `a1` and later.
- x86_64 receives the returned hidden pointer in `RAX`.

Descriptor-backed calls capture native ABI argument lanes plus stack metadata.
The runtime descriptor decides the exact shuffle for libc/libgcc wrappers, TLS
helpers, PLT/GOT trampolines, variadics, aggregates, and callback imports. The
CPU contract is the fast call gate and state transition, not symbol semantics.

## Traps

Foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved descriptor imports,
unsupported instructions, and illegal instructions produce a `POLYTRAP` packet.

Trap reasons:

| Reason | Meaning |
| ---: | --- |
| `0` | no trap |
| `1` | syscall trap |
| `2` | breakpoint trap |
| `3` | unresolved descriptor import |
| `4` | illegal or unsupported foreign instruction |

The packet records source mode, trap number, selector/immediate, trap PC,
resume PC, all eight POLYTRAP argument lanes, and the first eight native foreign ABI argument registers.

If a trap vector is installed, control transfers to that frontend handler.
Otherwise syscall/import traps surface as x86 `#UD`, and breakpoint traps
surface as x86 `#BP`. Userspace or the OS decides whether the trap means Linux
syscall translation, signal delivery, lazy binding, debugger handling, or
failure.

## Interrupts and Exceptions

Asynchronous events during foreign fetch are precise:

- hardware saves interrupted frontend mode and PC;
- enabled poly state is saved through the poly XSAVE component;
- control enters the standard x86_64 interrupt/fault path;
- `IRET64`, `SYSRET`, `SYSEXIT`, or signal return restores the recorded
  foreign frontend when the saved state requires it.

Raw foreign execution is CPL3-only in the prototype. The x86_64 kernel remains
responsible for privilege transitions, page faults, scheduling, and signals.

## XSAVE State

The prototype CPUID contract exposes the poly state as XCR0 component `20`.
The component is 4096 bytes, 64-byte aligned, little-endian, and layout version
`3`.

| Offset | Contents |
| ---: | --- |
| `0x000` | header, mode, flags, foreign PC, TLS base, trap vector |
| `0x040` | last trap packet header |
| `0x080` | trap argument lanes `0`-`7` |
| `0x0c0` | active 32-byte transition record |
| `0x100` | AArch64 `x0`-`x30` plus `sp` |
| `0x200` | AArch64 `v0`-`v31`, 128 bits each |
| `0x400` | AArch64 `NZCV`, `FPCR`, `FPSR`, reservation metadata |
| `0x480` | RISC-V `x0`-`x31`; `x0` restores as zero |
| `0x580` | RISC-V `f0`-`f31`, stored in 128-bit slots |
| `0x780` | RISC-V `fcsr`, reservation metadata |
| `0x800` | descriptor import return stack |
| `0xd00` | reserved, zero on save and ignored on restore |

Normal x86 GPR, RIP, RFLAGS, x87, XMM/YMM/ZMM, PKRU, and similar state remain
in their standard x86 save locations.

The stock Linux guest does not yet enable `XCR0[20]`, so the prototype also has
explicit software state-key and export/import operations. That fallback is a
prototype mechanism, not the silicon contract.

The explicit state import layout version is `3`.

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

## Runtime Policy

Hardware provides frontend transitions, trap packets, explicit state, and
descriptor call gates. Userspace runtime code provides:

- foreign ELF parsing and segment mapping;
- dynamic relocations, PLT/GOT binding, IFUNC, TLS, copy relocs, and symbol
  versioning;
- `DT_NEEDED`, RPATH/RUNPATH, SONAME, preload, and dependency policy;
- generated ABI thunks and cross-ISA trampolines;
- syscall translation when targeting a specific OS ABI;
- libc, libgcc, libatomic, TLS, and process-query helper semantics.

Hardware must not special-case libc names, Linux syscall numbers, libgcc helper
ids, or dynamic-linker symbol names.

## Validation

Prefer real boot tests over regex contract checks:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

`tools/contracts/` is kept runnable for coarse consistency checks, but it is
not the main validation path.
