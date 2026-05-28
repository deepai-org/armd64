# Polyglot ISA Contract

This document separates the architectural target from the current Bochs
prototype.  The target is compatibility with existing precompiled AArch64 and
RISC-V code linked into an x86_64 process, not a new compiler-only ABI.

## Hardware Contract

Production hardware should not use `#UD` envelopes for hot operations.  A
silicon or FPGA implementation should expose CPUID-gated x86 instructions for:

- `PENTER.A64`: enter fixed-width AArch64 fetch at the next byte.
- `PENTER.RV64`: enter RISC-V fetch at the next byte, decoding both 16-bit
  compressed and 32-bit base instructions.
- `PEXIT`: return to x86_64 fetch without taking an exception.
- `PCALL.A64.SYSV`: call an AArch64 AAPCS64 target from an x86_64 SysV caller.
- `PCALL.RV64.SYSV`: call a RISC-V psABI target from an x86_64 SysV caller.
- Descriptor-based `PCALL` forms for aggregates, variadics, unwind metadata,
  and other cases that cannot be encoded by one fixed shuffle.

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

Foreign traps are architectural exits, not hardware libcalls, Linux policy, or
any other operating-system policy.  When foreign code executes AArch64 `svc`,
RISC-V `ecall`, AArch64 `brk`, RISC-V `ebreak`, or an unsupported/illegal
foreign instruction, hardware records a trap frame, switches the frontend back
to x86_64 or another configured supervisor frontend, and transfers control to
an implementation-defined trap path.  Firmware, the OS, a loader, or a
userspace runtime decides whether to translate a syscall, deliver a signal,
invoke a thunk, emulate the instruction, or reject the operation.

Foreign memory ordering is specified as x86_64 TSO for the prototype
compatibility target, and leaf `0x40000007` is the runtime-discoverable memory
ABI for that contract.  AArch64 and RISC-V barriers are legal decode points,
and foreign atomic read-modify-write instructions operate through the same
coherent virtual-memory path as ordinary x86_64 memory operations.

Interrupts, faults, and debug exceptions taken during foreign fetch must record
the interrupted foreign mode and PC before entering the x86_64 kernel path.
`IRET`, `SYSRET`, `SYSEXIT`, and signal return must restore the foreign
frontend mode when the saved architectural state requires it.

## Bochs Prototype Contract

Bochs now has a fixed 8-byte x86 opcode-family placeholder for hot frontend
and call operations.  Both long-mode and non-long-mode x86 decode route the
`0f 24` opcode to `BX_IA_POLYMODE`; `#UD` handling is not an alternate dispatch
path for this family:

- `0f 24 00 50 4f 4c 59 21`: prototype `PEXIT`, returning to x86_64 mode.
- `0f 24 01 50 4f 4c 59 21`: prototype `PENTER.A64`, entering raw AArch64
  fetch at the next byte.
- `0f 24 02 50 4f 4c 59 21`: prototype `PENTER.RV64`, entering raw RISC-V
  fetch at the next byte.
- `0f 24 10 50 4f 4c 59 21`: prototype `PCALL.A64.SYSV`.
- `0f 24 11 50 4f 4c 59 21`: prototype `PCALL.RV64.SYSV`.
- `0f 24 12 50 4f 4c 59 21`: prototype `PCALL.A64.SYSV.SRET`.
- `0f 24 13 50 4f 4c 59 21`: prototype `PCALL.RV64.SYSV.SRET`.
- `0f 24 14 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.FPAIR32RET`, packing AArch64 `s0`/`s1` into x86_64
  SysV `XMM0[63:0]` on return.
- `0f 24 15 50 4f 4c 59 21`: prototype
  `PCALL.RV64.SYSV.FPAIR32RET`, packing RISC-V `fa0`/`fa1` into x86_64
  SysV `XMM0[63:0]` on return.
- `0f 24 16 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.FPAIR32ARG`, unpacking x86_64 SysV `XMM0[63:0]` into
  AArch64 `s0`/`s1` and shifting following FP argument lanes up by one
  foreign FP register.
- `0f 24 17 50 4f 4c 59 21`: prototype
  `PCALL.RV64.SYSV.FPAIR32ARG`, unpacking x86_64 SysV `XMM0[63:0]` into
  RISC-V `fa0`/`fa1` and shifting following FP argument lanes up by one
  foreign FP register.
- `0f 24 18 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.HETERO_U64_F64`, mapping an x86_64 SysV
  `{ u64, double }` aggregate argument from `RDI`/`XMM0` to AAPCS64
  `x0`/`x1`, shifting the following integer argument to `x2`, and mapping the
  returned `x0`/`x1` aggregate back to x86_64 `RAX`/`XMM0`.
- `0f 24 19 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.HETERO_F64_U64`, mapping an x86_64 SysV
  `{ double, u64 }` aggregate argument from `XMM0`/`RDI` to AAPCS64
  `x0`/`x1`, shifting the following integer argument to `x2`, and mapping the
  returned `x0`/`x1` aggregate back to x86_64 `XMM0`/`RAX`.
- `0f 24 1a 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.HETERO_U64_F32`, mapping an x86_64 SysV
  `{ u64, float }` aggregate argument from `RDI`/`XMM0[31:0]` to AAPCS64
  `x0`/`x1`, shifting the following integer argument to `x2`, and mapping the
  returned `x0`/`x1` aggregate back to x86_64 `RAX`/`XMM0[31:0]`.
- `0f 24 1b 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.HETERO_F32_U64`, mapping an x86_64 SysV
  `{ float, u64 }` aggregate argument from `XMM0[31:0]`/`RDI` to AAPCS64
  `x0`/`x1`, shifting the following integer argument to `x2`, and mapping the
  returned `x0`/`x1` aggregate back to x86_64 `XMM0[31:0]`/`RAX`.
- `0f 24 1c 50 4f 4c 59 21`: prototype
  `PCALL.RV64.SYSV.COMPACT_U32_F32`, unpacking an x86_64 SysV
  `{ u32, float }` aggregate from packed `RDI` into RISC-V `a0`/`fa0`,
  shifting the following integer argument to `a1`, and repacking the returned
  `a0`/`fa0` lanes into `RAX`.
- `0f 24 1d 50 4f 4c 59 21`: prototype
  `PCALL.RV64.SYSV.COMPACT_F32_U32`, unpacking an x86_64 SysV
  `{ float, u32 }` aggregate from packed `RDI` into RISC-V `fa0`/`a0`,
  shifting the following integer argument to `a1`, and repacking the returned
  `fa0`/`a0` lanes into `RAX`.
- `0f 24 1e 50 4f 4c 59 21`: prototype
  `PCALL.A64.SYSV.FP64STACK`, preserving scalar FP register aliases while
  copying x86_64 SysV FP64 overflow stack arguments from `[RSP+8]` into the
  AArch64 foreign stack window.
- `0f 24 1f 50 4f 4c 59 21`: prototype
  `PCALL.RV64.SYSV.FP64STACK`, preserving scalar FP register aliases while
  mapping up to eight x86_64 SysV FP64 overflow stack arguments from
  `[RSP+8]` onward into RISC-V `a0`-`a7` after `fa0`-`fa7` are consumed.
- `0f 24 20 50 4f 4c 59 21`: prototype `PIRET`, used by
  descriptor-driven foreign-to-x86 import calls to resume the saved foreign
  return PC after an x86 helper returns normally.
- `0f 24 30+id 50 4f 4c 59 21`: syscall status read.  `id=0` returns the
  current mode, `id=1` returns the last foreign syscall number, and `id=2`
  returns the last foreign syscall mode.
- `0f 24 38+id 50 4f 4c 59 21`: break-trap status read.  `id=1` returns the
  last break number and `id=2` returns the last break source mode.
- `0f 24 40+id 50 4f 4c 59 21`: mode/counter status read.  `id=0` returns
  frontend switches, `id=1` returns the current mode, `id=2` returns raw foreign
  instructions, `id=3` returns foreign syscalls, and `id=4` returns foreign
  breakpoint traps.
- `0f 24 50+id 50 4f 4c 59 21`: trap status read.  `id=0` returns the reason,
  `id=1` returns the source mode, `id=2` returns the trap number, `id=3`-`8`
  return trap arguments `0`-`5`, `id=9` returns the trap PC, and `id=10`
  returns the trap selector/immediate where the foreign instruction encoding
  provides one; `id=11` returns the trap resume PC, and `id=12`-`13` return
  trap arguments `6`-`7`.
- `0f 24 60 50 4f 4c 59 21`: trap vector set.  `RAX` is the x86 handler PC or
  zero to disable vector delivery.
- `0f 24 61 50 4f 4c 59 21`: trap vector get.  Returns the current handler PC in
  `RAX`.
- `0f 24 62 50 4f 4c 59 21`: trap return.  Resumes the recorded source frontend
  at the trap resume PC.
- `0f 24 65 50 4f 4c 59 21`: explicit state key set.  `RAX=0` disables the
  explicit key and falls back to the stack-region key.
- `0f 24 66 50 4f 4c 59 21`: explicit state key get.  Returns the current
  explicit key in `RAX`.
- `0f 24 67 50 4f 4c 59 21`: explicit state export.  `RAX` points to a
  4096-byte, 64-byte-aligned `struct poly_xsave_state` buffer.  The CPU writes
  the current keyed prototype state in the fixed leaf `0x40000004` layout.
- `0f 24 68 50 4f 4c 59 21`: explicit state import.  `RAX` points to a
  `struct poly_xsave_state` buffer with magic `0x31594c50`; the state import layout version is `2`.
  The CPU imports the state into the current keyed prototype bank and
  continues execution in x86 mode.  This is a prototype software state operation
  using the same layout as the active poly xstate component.

Bochs decodes the `0f 24` opcode slot through the prototype `BX_IA_POLYMODE`
handler and validates the trailing `POLY!` magic before changing frontend
state.  It is still a prototype allocation, but it no longer depends on the
generic invalid-opcode dispatch path or a `UD2` envelope.

`UD2` is not an alternate polyglot envelope in the current ISA contract;
ordinary `UD2` retains standard invalid-opcode behavior.

The prototype exposes private CPUID leaves when `poly_enabled=1` so runtimes can
discover the experimental hardware contract before emitting poly operations:

- `CPUID.EAX=0x40000000`: `EAX=0x40000009`, `EBX:EDX:ECX="PolyglotCPU!"`.
- `CPUID.EAX=0x40000001`: `EAX=1` for the poly CPUID ABI version.
- `0x40000001.EBX`: frontend mode mask.  Bits `0`, `3`, and `4` mean x86_64,
  raw AArch64, and raw RISC-V.
- `0x40000001.ECX`: feature mask.  Bits `0`-`27` mean raw AArch64, raw RISC-V,
  neutral direct switches, native return cookies, x86 SysV `PCALL`, `PCALL`
  sret, scalar FP bridging, trap records, user return restoration, x86 TSO
  foreign ordering, per-thread synthetic banks, reserved bit `11`, the
  prototype x86 poly opcode family, two-float aggregate return packing,
  two-float aggregate argument unpacking,
  `{u64,double}`/`{double,u64}`/`{u64,float}`/`{float,u64}` heterogeneous
  aggregate bridging for native ABI `PCALL`, RISC-V `{u32,float}` and
  `{float,u32}` compact aggregate bridges, neutral AArch64<->RISC-V compact
  aggregate cross-calls, runtime-supplied foreign-to-x86 import descriptor
  slots, FP64 overflow stack-argument `PCALL` variants, neutral
  AArch64<->RISC-V FP64 overflow stack-argument cross-call variants, the
  architectural trap vector/trap-return path, explicit software-selected poly
  state keys, and fixed 128-bit vector ABI bridging for tested AArch64 calls.
  The
  double-lane bridge forms also cover ABI-compatible `{u32,double}` and
  `{double,u32}` shapes.
- `0x40000001.EDX`: architectural XSAVE component id.  The current prototype
  reports component `20`.
- `CPUID.EAX=0x40000002, ECX=0`: native escape encoding discovery.
  `EAX[15:0]=0x7fff` means AArch64 `brk #0x7fff` exits to x86_64;
  `EAX[31:16]=0x7ffe` means AArch64 `brk #0x7ffe` switches to RISC-V;
  `EBX=0x7ffd` means AArch64 `brk #0x7ffd` calls RISC-V;
  `ECX=0x0000000b` means RISC-V custom-0 exits to x86_64; and
  `EDX=0x0000002b` means RISC-V custom-1 switches to AArch64.
- `CPUID.EAX=0x40000002, ECX=1`: `EAX=0x0000005b` reports the RISC-V
  custom-2 AArch64 cross-call encoding, `EBX=0x0000107b` reports the RISC-V
  compact `{u32,float}` AArch64 cross-call encoding, and `ECX=0x0000207b`
  reports the compact `{float,u32}` variant.  Other registers are reserved
  zero.
- `CPUID.EAX=0x40000002, ECX=2`: `EAX=106` reports the first
  foreign-to-x86 import descriptor slot id, `EBX=8` reports the current slot
  count, `ECX=32` reports the descriptor byte size, and `EDX=16` reports the
  import-call stride.
- `CPUID.EAX=0x40000002, ECX=3`: `EAX=0x7ffa` reports the AArch64
  FP64 overflow stack-argument RISC-V cross-call encoding, and `EBX=0x0000307b`
  reports the matching RISC-V AArch64 cross-call encoding.  Other registers
  are reserved zero.
- `CPUID.EAX=0x40000002, ECX=4`: `EAX=0x7ff9` reports the AArch64
  trap-return escape, `EBX=0x0000407b` reports the RISC-V trap-return
  instruction, and `ECX=0x63`/`EDX=0x64` report the x86 trap-vector mode
  set/get opcode selectors.
- `CPUID.EAX=0x40000002, ECX=5`: `EAX=144` reports the foreign import ID
  count, `ECX:EBX=0xffffffffffffe000` reports the import-call address window
  base, and `EDX=16` reports the import-call stride.
  Import ID `2` is reserved for the removed legacy x86-add helper and now
  traps like any unresolved descriptor-backed import; runtime-provided x86
  helper calls start at descriptor slot ID `106`.
- `CPUID.EAX=0x40000003`: prototype foreign-state contract discovery.
  `EAX` reports state flags.  Bits `0`-`6` mean overlapping x86-visible
  GPR/FP state, prototype synthetic banks, `CR3` keying, `FSBASE` keying,
  8 MiB stack-region keying, user-return restoration, and x86 TSO foreign
  ordering.  Bit `7` means non-aliased foreign state is exposed as an
  architectural XSAVE component.  Bit `8` means software can select an explicit
  state key with `0f 24 65 ... POLY!`; a zero explicit key restores the
  stack-region fallback.  Bit `9` means native cross-frontend return state uses
  fixed 32-byte transition records.  Bit `10` means the prototype implements
  explicit state export/import opcodes `0f 24 67/68 ... POLY!` using the fixed
  `struct poly_xsave_state` layout.  Bit `11` means the formal silicon-target
  XSAVE component contract is present in leaf `0x40000004`.  `EBX=23` reports
  the stack region shift.
  Legacy syscall/break status registers, trap-vector policy, recorded
  trap-packet/status state, trap-return save state, and 32-byte transition
  records are part of this keyed prototype state and component layout.
  `ECX=20` reports the XCR0 component id and `EDX=4096` reports the XSAVE byte
  area.
- `CPUID.EAX=0x40000004`: silicon-target XSAVE contract discovery.  `EAX=20`
  is the XCR0 component number, `EBX=4096` is the component byte
  size, `ECX[15:0]=2` is the layout version, `ECX[31:16]=64` is the required
  byte alignment, and `EDX=0x1f` reports flags: user XCR0 component,
  OSXSAVE/XRSTOR required, interrupt-resume state present, trap state present,
  and no hidden foreign register banks permitted.  This is a formal hardware
  ABI definition and is enumerated through standard `CPUID.0xD` when polyglot
  execution is enabled.  Component `20` avoids component `11`, which is already
  assigned to standard x86 CET_U state in current x86 XSAVE maps.  A guest OS
  must still enable `XCR0[20]` before architectural `XSAVE/XRSTOR` context
  switches include this component; the stock Linux guest used by the current
  boot tests enumerates the component but leaves `XCR0[20]` disabled.
- `CPUID.EAX=0x40000005`: architectural trap-packet ABI discovery.  `EAX=2`
  is the trap layout version, `EBX=64` is the byte size of the fixed packet
  header, `ECX=8` is the native ABI argument slot count, and `EDX=0x1f`
  reports flags: vector delivery, no-vector x86 exception delivery,
  trap-return source-state restoration, x86/AArch64/RISC-V handler entry, and
  trap-status opcodes.  This leaf is independent from the active XSAVE leaf so
  software can discover the packet ABI before hardware exposes the full foreign
  register component.
- `CPUID.EAX=0x40000006`: raw-mode interrupt/resume ABI discovery.  `EAX=1`
  is the interrupt ABI version.  `EBX=0x1f` reports flags: raw fetch is
  CPL3-only, asynchronous entry uses a standard x86 interrupt frame, precise
  foreign state is saved before entry, the interrupted foreign PC is precise,
  and events are checked between raw foreign instructions.  `ECX=0x0f`
  reports supported return paths: `IRET64`, `SYSRET`, `SYSEXIT`, and guest
  signal return.  `EDX=0x18` is the covered raw frontend mask: AArch64 bit
  `3` and RISC-V bit `4`.
- `CPUID.EAX=0x40000007`: foreign memory-ordering ABI discovery.  `EAX=1`
  is the memory ABI version.  `EBX=1` selects x86 TSO.  `ECX=0x1f` reports
  flags: raw frontends share the coherent x86 memory subsystem, AArch64
  barriers are ordering-preserving no-ops, RISC-V fences are
  ordering-preserving no-ops, foreign atomics use coherent memory operations,
  and raw frontends do not inject weak AArch64/RISC-V reordering.  `EDX=0x18`
  is the covered raw frontend mask: AArch64 bit `3` and RISC-V bit `4`.
- `CPUID.EAX=0x40000008`: cross-frontend transition ABI discovery.  `EAX=1`
  is the transition ABI version.  `EBX=0x1ff` reports flags: x86 transitions
  use decoded poly opcodes, raw frontends use native escape instructions, every
  transition flushes the active frontend and ends the current decode block,
  source and destination PCs are precise, raw fetch is fixed-width/aligned,
  AArch64/RISC-V can switch or call each other without returning through x86,
  native returns use hardware cookies, and trap return is architectural.
  `ECX[15:0]=4` is the AArch64 raw alignment, `ECX[31:16]=2` is the RISC-V
  raw alignment, and `EDX=0x19` is the covered frontend mask: x86 bit `0`,
  AArch64 bit `3`, and RISC-V bit `4`.
- `CPUID.EAX=0x40000009`: native ABI bridge/runtime descriptor discovery.
  `EAX=1` is the ABI bridge version.  `EBX=0xfff` reports flags: x86_64 SysV
  can enter AAPCS64 and RISC-V psABI, hidden-sret forms are described, scalar
  FP aliases are described, focused aggregate bridge forms are described, FP64
  overflow stack-argument forms are described, imports use descriptor-backed
  foreign-to-x86 calls, TLS base handoff is explicit, descriptor tables are
  runtime-supplied, fixed CPU helper fallbacks are not part of the ABI, and x86
  helper targets return through ordinary `ret`, with fixed 128-bit vector
  register arguments/returns advertised for supported native ABI paths.
  `ECX[7:0]=8` is the native
  GPR argument lane count, `ECX[15:8]=8` is the scalar FP argument lane count,
  `ECX[31:16]=16` is the foreign stack alignment, `EDX[15:0]=32` is the
  descriptor byte size, and `EDX[31:16]=16` is the descriptor call stride.
  The `polycall` loader and `polybench` descriptor benchmarks validate this
  leaf before they install descriptor-backed foreign-to-x86 import tables.

The leaf `0x40000004` XSAVE component layout is fixed-size and little-endian:

| Offset | Size | Contents |
| --- | ---: | --- |
| `0x000` | `0x040` | Header: magic/version/size, current frontend mode, flags, foreign PC, foreign TLS base, trap-vector PC, and trap-vector mode. |
| `0x040` | `0x040` | Last architectural trap packet header: reason, source mode, trap number, selector/immediate, trap PC, and resume PC. |
| `0x080` | `0x040` | Last trap argument registers `arg0`-`arg7`. |
| `0x0c0` | `0x040` | Active cross-frontend transition record and reserved expansion. |
| `0x100` | `0x100` | AArch64 integer state: `x0`-`x30` plus `sp`, each 64-bit. |
| `0x200` | `0x200` | AArch64 SIMD/FP state: `v0`-`v31`, each 128-bit. |
| `0x400` | `0x080` | AArch64 `NZCV`, `FPCR`, `FPSR`, reservation metadata, and reserved expansion. |
| `0x480` | `0x100` | RISC-V integer state: `x0`-`x31`, each 64-bit; `x0` is saved as zero and ignored on restore. |
| `0x580` | `0x200` | RISC-V FP state: `f0`-`f31`, each stored in a 128-bit slot for future FLEN growth. |
| `0x780` | `0x080` | RISC-V `fcsr`, reservation metadata, and reserved expansion. |
| `0x800` | `0x800` | Reserved, zero on save and ignored on restore until a future layout version assigns it. |

`tools/polycpuid.h` defines the matching C ABI types.  The layout is guarded by
compile-time assertions for every offset above, the total 4096-byte component
size, the 64-byte trap packet, and the 32-byte transition frame.  Runtime probe
expectations for leaves `0x40000004` and `0x40000005` are derived from those
types so a future structure drift fails the build or probe instead of silently
changing the advertised hardware contract.

This component contains only polyglot-extension state.  Existing x86 GPR,
RFLAGS, RIP, XMM/YMM/ZMM, x87, PKRU, and similar state remain in their normal
x86 architectural save locations.  Hardware must update this component before
delivering asynchronous events from raw foreign fetch and must restore the
recorded frontend mode/PC on `IRET`, `SYSRET`, `SYSEXIT`, or signal-return
equivalent when returning to the interrupted user address.  Leaf `0x40000006`
is the runtime-discoverable interrupt ABI for those entry and return semantics.

Raw foreign modes also have native frontend-switch encodings so x86 is not the
only routing hub:

- AArch64 `brk #0x7fff`: exit raw AArch64 and resume x86_64 decode.
- AArch64 `brk #0x7ffe`: switch directly from raw AArch64 to raw RISC-V at
  the next byte.
- AArch64 `brk #0x7ffd`: call a raw RISC-V target held in `x16`, saving an
  AArch64 continuation from `x17`.
- AArch64 `brk #0x7ffc`: call a raw RISC-V target with a compact
  `{u32,float}` native ABI bridge between packed `x0` and split `a0`/`fa0`.
- AArch64 `brk #0x7ffb`: call a raw RISC-V target with a compact
  `{float,u32}` native ABI bridge between packed `x0` and split `fa0`/`a0`.
- AArch64 `brk #0x7ffa`: call a raw RISC-V target with an FP64 overflow
  stack-argument bridge, mapping eight AArch64 stack double slots into RISC-V
  `a0`-`a7` after the shared `d0`-`d7`/`fa0`-`fa7` lanes are consumed.
- RISC-V custom-0 `0x0000000b`: exit raw RISC-V and resume x86_64 decode.
- RISC-V custom-1 `0x0000002b`: switch directly from raw RISC-V to raw
  AArch64 at the next byte.
- RISC-V custom-2 `0x0000005b`: call a raw AArch64 target held in `x5`,
  saving a RISC-V continuation from `x6`.
- RISC-V custom `0x0000107b`: call a raw AArch64 target with a compact
  `{u32,float}` native ABI bridge between split `a0`/`fa0` and packed `x0`.
- RISC-V custom `0x0000207b`: call a raw AArch64 target with a compact
  `{float,u32}` native ABI bridge between split `fa0`/`a0` and packed `x0`.
- RISC-V custom `0x0000307b`: call a raw AArch64 target with an FP64 overflow
  stack-argument bridge, mapping RISC-V `a0`-`a7` into eight AArch64 stack
  double slots after the shared `fa0`-`fa7`/`d0`-`d7` lanes are consumed.

These native switches preserve the shared low integer register aliases, so
`x0`/`a0`/`RAX` can carry a value through AArch64-to-RISC-V or
RISC-V-to-AArch64 code without an x86 trampoline.  The shared scalar FP aliases
similarly carry `d0`-`d7`/`fa0`-`fa7` through neutral cross-calls.
The native cross-call forms additionally set the callee's native link register
to a hardware return cookie, so AArch64 `ret` or RISC-V `jalr x0, 0(ra)`
restores the caller frontend mode and continuation without an x86 rendezvous.
The mixed raw probes also cover a 16-bit compressed RISC-V instruction followed
by a direct switch to AArch64, proving the neutral path does not require
32-bit-only RISC-V code before leaving the RISC-V frontend.  The Bochs
prototype backs this with a small bounded cross-return stack. Direct RISC-V
ELF execution preserves raw executable bytes and accepts 2-byte-aligned entry
segments, so compressed-code compatibility is not dependent on packing
halfwords into synthetic 32-bit loader words; the gate includes a 6-byte
compressed RISC-V ELF entry segment to verify the non-4-byte-sized case, plus
a 6-byte mixed compressed/32-bit entry that returns through `jalr` to a
halfword-aligned escape.
`polybench`
covers scalar double FP cross-calls, eight-register double FP argument pressure
across `d0`-`d7`/`fa0`-`fa7`, FP64 overflow stack-argument cross-calls that sum
sixteen double arguments in both directions, mixed integer/FP cross-calls,
two-register integer returns, compact `{u32,float}`/`{float,u32}` native ABI
cross-call bridging, shared-stack cross-calls, caller callee-saved integer/FP
register preservation, syscall trap routing inside neutral callees, and a
nested AArch64 -> RISC-V -> AArch64 call chain.  The stack probes use the
caller's real user `sp`: the caller allocates and restores an aligned slot,
while the opposite foreign frontend reads it through ordinary native stack
addressing.  The callee-saved probes keep AArch64 `x19` or RISC-V `s0` live
across a neutral call into the opposite frontend, and the saved-FP probes keep
AArch64 `d8` or RISC-V `fs0` live while the opposite frontend uses its own
callee-saved FP register.  The pair-return probes verify the second integer
result lane maps across `x1`/`a1`.  The syscall
probes execute native AArch64 `svc` or RISC-V `ecall` inside the neutral
callee and return the deterministic syscall result through the caller's native
result register.  The break probes execute native AArch64 `brk #1` or
RISC-V `ebreak` strlen traps inside the neutral callee and return through the
same hardware cookie path.  Descriptor-import probes execute ordinary AArch64
`blr` or RISC-V `jalr` calls to `strlen`, `strnlen`, `memset`, `memcpy`, and
three-argument `memcmp` import descriptors inside the neutral callee, preserve
the callee's native link register, mutate a shared x86 buffer through the
foreign memory path, and return through the same cross-frontend cookie path.
`polyprobe` also installs a guest x86 trap-vector handler for its raw syscall
and breakpoint probes, so the low-level status/counter gate can run without
Bochs synthesizing Linux or libc behavior.
The prototype saves that stack, the `PCALL` return cookie, and x86-import return
state with the same synthetic bank as the non-aliased foreign registers.  The
bank key is guest `CR3`, user `FSBASE`, and either an explicit userspace poly
state key or an 8 MiB-aligned user stack-region fallback key.  Runtime or OS
code should set an explicit key for deterministic per-thread banks; the
fallback keeps common pthread stacks isolated even when static TLS does not
give each guest thread a distinct `FSBASE`.  The `polythread` guest test
exercises this with real x86_64 pthreads repeatedly entering AArch64 and
RISC-V `PCALL` paths.  The `polysignal` guest test arms real `SIGALRM`
delivery during long raw AArch64 and RISC-V `PCALL` loops, verifying that the
x86_64 signal handler and `rt_sigreturn` path resume the interrupted foreign
frontend.  The Bochs prototype now records raw-mode interrupt state before
x86_64 long-mode interrupt delivery and restores the recorded foreign frontend
after `IRET64`, `SYSRET`, `SYSEXIT`, or Linux signal return reaches the
interrupted user RIP.  The same interrupt-resume state is part of the poly
xstate component layout.

The prototype `PCALL` forms use `R10` as the foreign target address and `R11`
as the x86_64 return continuation; `R13` optionally carries the foreign TLS
block base used by AArch64 `TPIDR_EL0` and RISC-V TLS descriptors.  They
currently cover the common register
fast path: x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` plus stack
slots `[RSP+8]` and `[RSP+16]` are mapped to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`; the foreign stack pointer is a separate 16-byte-aligned window below
the x86 frame with stack arguments copied from `[RSP+24]` onward so the first
foreign stack-passed argument is at `[sp]`;
`XMM0`-`XMM7` remain aliased to AArch64 `v0`-`v7` or RISC-V `fa0`-`fa7`,
covering scalar FP arguments/returns, tested AArch64 fixed 128-bit SIMD
arguments/returns through `XMM0`/`XMM1` and `v0`/`v1`, and two-register
homogeneous double
aggregate arguments and returns through `XMM0`/`XMM1`, plus two-`float`
homogeneous aggregate returns packed into `XMM0[63:0]` and two-`float`
homogeneous aggregate arguments unpacked from `XMM0[63:0]`, including mixed
integer/FP signatures where GPR and XMM lanes are consumed in one native call.
AArch64 additionally has focused bridges for `{u64,double}`, `{double,u64}`,
`{u64,float}`, and `{float,u64}` heterogeneous aggregates whose x86 lanes are
split across GPR and XMM registers but whose AAPCS64 lanes live in
integer-register bit lanes; the double-lane bridge forms also cover
`{u32,double}` and `{double,u32}` native ABI layouts because the narrower
integer payloads stay in the same argument/result lanes.  RISC-V additionally
has focused compact aggregate bridges for `{u32,float}` and `{float,u32}`,
where x86 SysV and AAPCS64 keep the aggregate packed in one GPR but RISC-V
psABI splits it across `a0` and `fa0`; the native AArch64<->RISC-V cross-call
opcodes include the same compact bridge so neutral calls do not have to route
through x86;
and
AArch64 `ret x30` or RISC-V `jalr x0, 0(ra)` returns through a cookie to the
saved x86 continuation, maps AArch64 `x0` or RISC-V `a0` back to x86 `RAX`,
maps AArch64 `x1` or RISC-V `a1` back to x86 `RDX` for ordinary two-word
integer aggregate returns, and restores the x86 stack pointer.
The `SRET` variants cover larger memory-return aggregates: AArch64 receives
the hidden result pointer in `x8` while user arguments remain in `x0`-`x7`;
RISC-V receives the hidden result pointer in `a0` and user arguments shift to
`a1`-`a7`, with the remaining arguments copied into the foreign stack window.
On return, the bridge restores x86 `RSP` and returns the hidden result pointer
in `RAX`, matching the x86_64 SysV memory-return convention.  The AArch64 raw
decoder treats register 31 as `SP` for add/sub-immediate instructions, matching
ordinary compiler stack-frame setup and teardown.
The AArch64 raw decoder also supports `adrp` page-relative PC materialization,
NZCV-backed `adds`/`subs` plus `b.cond` for ordinary condition-code branches,
64-bit `stp`/`ldp` pair load-store forms for normal frame save/restore, and
register-offset plus pre/post-indexed `ldr`/`str` forms for compiler-emitted
indexed and pointer-walking memory access. It also covers bitfield move aliases
such as `bfxil` and `bfi`, which common compilers use when repacking small
aggregate lanes, plus scalar AdvSIMD `ushr d` for packed-lane extraction.
Native `bl`/`blr` link-register calls handle local helper calls inside foreign
code.  The RISC-V raw decoder aliases `x2/sp` to the shared stack pointer, so
`addi sp, sp, imm` plus `ld`/`sd` stack accesses covers the same ordinary psABI
stack-frame pattern.
The `polycall` guest tool maps foreign ELF64 `PT_LOAD` segments into a single
in-memory image before entering raw mode, so page-relative code can address
separate text/data load segments rather than only inline instruction blobs. The
gate covers both AArch64 `adrp`/`ldr` and RISC-V `auipc`/`ld` split-load
payloads. `polycall` also accepts simple `ET_DYN` images with
`R_AARCH64_NONE`/`R_RISCV_NONE` no-op relocations,
`R_AARCH64_RELATIVE` or `R_RISCV_RELATIVE` relocations from `RELA` tables,
`REL` tables with in-place addends, packed `DT_RELR`
direct and bitmap relative relocation tables,
`R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolver relocations, and same-image symbolic 64-bit dynamic relocations
(`R_AARCH64_ABS64` or `R_RISCV_64`), applying them with the actual runtime load
bias before `PCALL`. The loader also accepts `R_AARCH64_COPY`/`R_RISCV_COPY`
copy relocations for non-PIE foreign executables that reference data exported by
same-directory foreign shared libraries. Symbolic relocation metadata and `path#symbol` entrypoint
lookup are read from `DT_SYMTAB`/`DT_STRTAB` in the loaded dynamic image.
`DT_HASH` is used to bound the sectionless dynamic symbol table, and
`DT_GNU_HASH` is supported for common GNU-hash-only shared objects.
The standalone `polyexec` path uses the same post-relocation memory protection
baseline for foreign application images: `PT_LOAD` pages are protected from
ELF `PF_R`/`PF_W`/`PF_X` flags, and `PT_GNU_RELRO` pages are marked read-only
before entering the foreign entrypoint. It also applies same-image symbolic
64-bit dynamic relocations for `R_AARCH64_ABS64`, `R_AARCH64_GLOB_DAT`,
`R_AARCH64_JUMP_SLOT`, `R_RISCV_64`, and `R_RISCV_JUMP_SLOT`, including
PLT relocation tables described by `DT_JMPREL`/`DT_PLTRELSZ`, and runs
same-image `R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolvers through the
foreign frontend before applying final page protections, while still
rejecting unresolved external dynamic symbols in the standalone application
path.
The binfmt wrapper still uses `polyexec` for standalone foreign images, but
routes dependency-backed shared-object entrypoints through `polycall` so
`DT_NEEDED` dependency loading, PLT/GOT binding, TLS, IFUNC, and constructor
paths are exercised by the binfmt boot matrix without embedding a second full
dynamic linker in `polyexec`.
`DT_VERSYM`/`DT_VERNEED`/`DT_VERDEF` symbol-version metadata is honored when
binding undefined relocations to dependency exports, so a relocation requiring
`foo@OLD` from a specific needed library does not silently bind to a default
`foo@@NEW` export or to the same version name from a different dependency.
Dependency binding also requires exportable ELF binding and visibility:
`STB_GLOBAL`/`STB_WEAK`/`STB_GNU_UNIQUE` plus
`STV_DEFAULT`/`STV_PROTECTED`.
Section tables are kept as a fallback for synthetic test payloads. The gate
uses compiler-produced AArch64 and RISC-V shared objects
(`aarch64-pcall-real.so#poly_entry`, `riscv-pcall-real.so#poly_entry`,
`riscv-pcall-real-rv64gc.so#poly_entry`,
`aarch64-pcall-gnu-hash-real.so#poly_entry`,
`riscv-pcall-gnu-hash-real.so#poly_entry`,
`aarch64-pcall-state.so#poly_entry`, and
`riscv-pcall-state.so#poly_entry`) plus compiler-produced imported-function
objects (`aarch64-pcall-import-real.so#poly_entry` and
`riscv-pcall-import-real.so#poly_entry`,
`riscv-pcall-import-real-rv64gc.so#poly_entry`), compiler-produced imported-object
objects (`aarch64-pcall-import-value-real.so#poly_entry` and
`riscv-pcall-import-value-real.so#poly_entry`), compiler-produced weak undefined
import objects (`aarch64-pcall-weak-import-real.so#poly_entry` and
`riscv-pcall-weak-import-real.so#poly_entry`), compiler-produced GNU-unique
dependency object-symbol pairs (`aarch64-pcall-gnu-unique-real.so#poly_entry`
with `libpolyunique-aarch64.so`, and
`riscv-pcall-gnu-unique-real.so#poly_entry` with
`libpolyunique-riscv.so`), compiler-produced default-preemptible and
`-Bsymbolic` dependency binding pairs
(`aarch64-pcall-symbolic-preempt-real.so#poly_entry`,
`aarch64-pcall-symbolic-bind-real.so#poly_entry`,
`aarch64-pcall-symbolic-protected-real.so#poly_entry`,
`riscv-pcall-symbolic-preempt-real.so#poly_entry`,
`riscv-pcall-symbolic-bind-real.so#poly_entry`, and
`riscv-pcall-symbolic-protected-real.so#poly_entry`), compiler-produced symbolic IFUNC
objects (`aarch64-pcall-ifunc-real.so#poly_entry` and
`riscv-pcall-ifunc-real.so#poly_entry`), compiler-produced `DT_NEEDED`
shared-library pairs (`aarch64-pcall-needed-real.so#poly_entry` with
`libpolyneeded-aarch64.so`, and `riscv-pcall-needed-real.so#poly_entry` with
`libpolyneeded-riscv.so`), compiler-produced dependency-to-root export pairs
where a needed DSO binds an undefined relocation back to the root loaded object
(`aarch64-pcall-root-export-real.so#poly_entry` with
`libpolyrootdep-aarch64.so`, and
`riscv-pcall-root-export-real.so#poly_entry` with
`libpolyrootdep-riscv.so`), compiler-produced dependency-to-root TLS pairs
where a needed DSO binds an undefined TLS relocation back to the root loaded
object (`aarch64-pcall-root-tls-real.so#poly_entry` with
`libpolyroottls-aarch64.so`, and
`riscv-pcall-root-tls-real.so#poly_entry` with
`libpolyroottls-riscv.so`), compiler-produced dependency-to-root IFUNC pairs
where a needed DSO binds an undefined relocation back to a root IFUNC resolver
(`aarch64-pcall-root-ifunc-real.so#poly_entry` with
`libpolyrootifunc-aarch64.so`, and
`riscv-pcall-root-ifunc-real.so#poly_entry` with
`libpolyrootifunc-riscv.so`), compiler-produced `DT_NEEDED` dependency-TLS pairs
(`aarch64-pcall-needed-tls-real.so#poly_entry` with
`libpolyneededtls-aarch64.so`, and `riscv-pcall-needed-tls-real.so#poly_entry`
with `libpolyneededtls-riscv.so`), compiler-produced `DT_NEEDED`
versioned-symbol pairs (`aarch64-pcall-versioned-real.so#poly_entry` with
`libpolyversioned-aarch64.so`, and
`riscv-pcall-versioned-real.so#poly_entry` with
`libpolyversioned-riscv.so`), compiler-produced `DT_NEEDED`
dependency-IFUNC pairs (`aarch64-pcall-needed-ifunc-real.so#poly_entry` with
`libpolyneededifunc-aarch64.so`, and
`riscv-pcall-needed-ifunc-real.so#poly_entry` with
`libpolyneededifunc-riscv.so`), compiler-produced `DT_NEEDED`
dependency-`DT_INIT`/`DT_FINI` pairs
(`aarch64-pcall-needed-dt-init-real.so#poly_entry` with
`libpolyneededdtinit-aarch64.so`, and
`riscv-pcall-needed-dt-init-real.so#poly_entry` with
`libpolyneededdtinit-riscv.so`), compiler-produced copy-relocation executables
(`aarch64-pcall-copy-reloc.elf#poly_entry` and
`riscv-pcall-copy-reloc.elf#poly_entry`), compiler-produced relocated
function-pointer objects (`aarch64-pcall-funcptr-real.so#poly_entry` and
`riscv-pcall-funcptr-real.so#poly_entry`), compiler-produced two-word aggregate
return objects (`aarch64-pcall-pair-real.so#poly_entry` and
`riscv-pcall-pair-real.so#poly_entry`), compiler-produced hidden-sret
aggregate return objects (`aarch64-pcall-sret-real.so#poly_entry` and
`riscv-pcall-sret-real.so#poly_entry`), compiler-produced TLS objects
(`aarch64-pcall-tls-real.so#poly_entry` and
`riscv-pcall-tls-real.so#poly_entry`) plus initial-exec TLS objects
(`aarch64-pcall-tls-ie-real.so#poly_entry` and
`riscv-pcall-tls-ie-real.so#poly_entry`), compiler-produced
constructor/destructor objects (`aarch64-pcall-ctor-real.so#poly_entry`,
`riscv-pcall-ctor-real.so#poly_entry`,
`aarch64-pcall-fini-real.so#poly_entry`,
`riscv-pcall-fini-real.so#poly_entry`,
`aarch64-pcall-dt-init-real.so#poly_entry`, and
`riscv-pcall-dt-init-real.so#poly_entry`), compiler-produced conditional objects
(`aarch64-pcall-cond-real.so#poly_entry` and
`riscv-pcall-cond-real.so#poly_entry`), compiler-produced compare-and-branch
objects (`aarch64-pcall-cbz-real.so#poly_entry` and
`riscv-pcall-cbz-real.so#poly_entry`), compiler-produced bit-test branch
objects (`aarch64-pcall-bitbranch-real.so#poly_entry` and
`riscv-pcall-bitbranch-real.so#poly_entry`), compiler-produced signed-extension
objects (`aarch64-pcall-signed-ext-real.so#poly_entry` and
`riscv-pcall-signed-ext-real.so#poly_entry`), compiler-produced signed-load
objects (`aarch64-pcall-signed-load-real.so#poly_entry` and
`riscv-pcall-signed-load-real.so#poly_entry`), compiler-produced integer-division
objects (`aarch64-pcall-int-div-real.so#poly_entry` and
`riscv-pcall-int-div-real.so#poly_entry`), compiler-produced integer multiply-add
objects (`aarch64-pcall-int-madd-real.so#poly_entry` and
`riscv-pcall-int-madd-real.so#poly_entry`), compiler-produced integer high-multiply
objects (`aarch64-pcall-int-highmul-real.so#poly_entry` and
`riscv-pcall-int-highmul-real.so#poly_entry`), compiler-produced 128-bit
division/modulo helper objects (`aarch64-pcall-int128-helpers-real.so#poly_entry`
and `riscv-pcall-int128-helpers-real.so#poly_entry`), compiler-produced 128-bit
integer/double conversion helper objects
(`aarch64-pcall-int128-fp-helpers-real.so#poly_entry` and
`riscv-pcall-int128-fp-helpers-real.so#poly_entry`), compiler-produced 128-bit
integer/float conversion helper objects
(`aarch64-pcall-int128-float-helpers-real.so#poly_entry` and
`riscv-pcall-int128-float-helpers-real.so#poly_entry`), compiler-produced libgcc
bit helper objects (`aarch64-pcall-bit-helpers-real.so#poly_entry` and
`riscv-pcall-bit-helpers-real.so#poly_entry`), compiler-produced quad-precision
`long double` helper objects (`aarch64-pcall-longdouble-helpers-real.so#poly_entry`,
`riscv-pcall-longdouble-helpers-real.so#poly_entry`,
`aarch64-pcall-longdouble-signed-helpers-real.so#poly_entry`, and
`riscv-pcall-longdouble-signed-helpers-real.so#poly_entry`), compiler-produced
quad-precision comparison/conversion helper objects
(`aarch64-pcall-longdouble-compare-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-compare-helpers-real.so#poly_entry`), compiler-produced
quad-precision 32-bit integer conversion helper objects
(`aarch64-pcall-longdouble-int32-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-int32-helpers-real.so#poly_entry`), compiler-produced integer carry-chain
objects (`aarch64-pcall-int-carry-real.so#poly_entry` and
`riscv-pcall-int-carry-real.so#poly_entry`), compiler-produced integer variable-shift
objects (`aarch64-pcall-int-varshift-real.so#poly_entry` and
`riscv-pcall-int-varshift-real.so#poly_entry`), compiler-produced integer logical
objects (`aarch64-pcall-int-logic-real.so#poly_entry` and
`riscv-pcall-int-logic-real.so#poly_entry`), compiler-produced integer bit-operation
objects (`aarch64-pcall-int-bitops-real.so#poly_entry` and
`riscv-pcall-int-bitops-real.so#poly_entry`), compiler-produced integer rotate/extract
objects (`aarch64-pcall-int-rotate-real.so#poly_entry` and
`riscv-pcall-int-rotate-real.so#poly_entry`), compiler-produced integer conditional
compare objects (`aarch64-pcall-int-ccmp-real.so#poly_entry` and
`riscv-pcall-int-ccmp-real.so#poly_entry`), AArch64 post-index memory object
(`aarch64-pcall-postindex-mem.so#poly_entry`), AArch64 atomic objects covering
exclusive LL/SC, default GCC outline helpers, LSE instructions, and 16-byte
libatomic helper imports
(`aarch64-pcall-atomic.so#poly_entry`,
`aarch64-pcall-atomic-outline.so#poly_entry`, and
`aarch64-pcall-atomic-lse.so#poly_entry`), RISC-V atomic object with AMO and
16-byte libatomic helper import coverage
(`riscv-pcall-atomic.so#poly_entry`), compiler-produced unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-produced indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-produced callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), and compiler-produced scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`) plus sixteen-double FP stack-argument objects
(`aarch64-pcall-fp64-stack-real.so#poly_entry` and
`riscv-pcall-fp64-stack-real.so#poly_entry`) plus compiler-produced homogeneous
double aggregate return objects (`aarch64-pcall-fpair-real.so#poly_entry` and
`riscv-pcall-fpair-real.so#poly_entry`), homogeneous float aggregate return
objects (`aarch64-pcall-fpair32-real.so#poly_entry` and
`riscv-pcall-fpair32-real.so#poly_entry`), homogeneous double aggregate argument
objects (`aarch64-pcall-fpair-arg-real.so#poly_entry` and
`riscv-pcall-fpair-arg-real.so#poly_entry`), homogeneous float aggregate
argument objects (`aarch64-pcall-fpair32-arg-real.so#poly_entry` and
`riscv-pcall-fpair32-arg-real.so#poly_entry`), mixed integer/FP argument objects
(`aarch64-pcall-mixed-args-real.so#poly_entry` and
`riscv-pcall-mixed-args-real.so#poly_entry`), heterogeneous aggregate objects
(`aarch64-pcall-hetero-real.so#poly_entry` and
`riscv-pcall-hetero-real.so#poly_entry`,
`aarch64-pcall-hetero-rev-real.so#poly_entry`, and
`riscv-pcall-hetero-rev-real.so#poly_entry`,
`aarch64-pcall-hetero32-real.so#poly_entry`,
`riscv-pcall-hetero32-real.so#poly_entry`,
`aarch64-pcall-hetero32-rev-real.so#poly_entry`, and
`riscv-pcall-hetero32-rev-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-real.so#poly_entry`,
`riscv-pcall-hetero-u32-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-rev-real.so#poly_entry`, and
`riscv-pcall-hetero-u32-rev-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-f32-real.so#poly_entry`,
`riscv-pcall-hetero-u32-f32-real.so#poly_entry`,
`aarch64-pcall-hetero-f32-u32-real.so#poly_entry`, and
`riscv-pcall-hetero-f32-u32-real.so#poly_entry`), and scalar double/float FP
import objects (`aarch64-pcall-fp64-import-real.so#poly_entry`,
`riscv-pcall-fp64-import-real.so#poly_entry`,
`aarch64-pcall-fp32-import-real.so#poly_entry`, and
`riscv-pcall-fp32-import-real.so#poly_entry`), compiler-produced scalar double
FP callee-saved objects (`aarch64-pcall-fp64-callee-real.so#poly_entry` and
`riscv-pcall-fp64-callee-real.so#poly_entry`), compiler-produced scalar float
FP callee-saved objects (`aarch64-pcall-fp32-callee-real.so#poly_entry` and
`riscv-pcall-fp32-callee-real.so#poly_entry`), compiler-produced scalar double
FP conditional objects (`aarch64-pcall-fp64-cond-real.so#poly_entry` and
`riscv-pcall-fp64-cond-real.so#poly_entry`), compiler-produced scalar double
FP division objects (`aarch64-pcall-fp64-div-real.so#poly_entry` and
`riscv-pcall-fp64-div-real.so#poly_entry`), compiler-produced scalar double FP
unary/zero-compare objects (`aarch64-pcall-fp64-unary-real.so#poly_entry` and
`riscv-pcall-fp64-unary-real.so#poly_entry`), compiler-produced scalar double FP
indexed-memory objects (`aarch64-pcall-fp64-indexed-mem-real.so#poly_entry` and
`riscv-pcall-fp64-indexed-mem-real.so#poly_entry`), compiler-produced scalar
double FP conversion objects (`aarch64-pcall-fp64-convert-real.so#poly_entry` and
`riscv-pcall-fp64-convert-real.so#poly_entry`) plus signed conversion objects
(`aarch64-pcall-fp64-signed-convert-real.so#poly_entry` and
`riscv-pcall-fp64-signed-convert-real.so#poly_entry`) and 32-bit conversion
objects (`aarch64-pcall-fp64-i32-convert-real.so#poly_entry`,
`riscv-pcall-fp64-i32-convert-real.so#poly_entry`,
`aarch64-pcall-fp64-u32-convert-real.so#poly_entry`, and
`riscv-pcall-fp64-u32-convert-real.so#poly_entry`) plus scalar float/double
conversion objects (`aarch64-pcall-fp-mixed-convert-real.so#poly_entry` and
`riscv-pcall-fp-mixed-convert-real.so#poly_entry`), and compiler-produced scalar
float FP objects (`aarch64-pcall-fp32-real.so#poly_entry` and
`riscv-pcall-fp32-real.so#poly_entry`) plus compiler-produced scalar float FP
memory objects (`aarch64-pcall-fp32-mem-real.so#poly_entry` and
`riscv-pcall-fp32-mem-real.so#poly_entry`). Nonzero `poly_entry` symbol
offsets for the dynamic-relocation probes ensure symbol resolution, not the ELF
entrypoint, selects the target. The stateful `.so` probes exercise
compiler-emitted access to writable static data in a separate RW `PT_LOAD`. The
FP `.so` probes call ordinary native ABI functions with float/double arguments
in `s0`/`d0`-`s2`/`d2` or `fa0`-`fa2` and verify the FP return through
`s0`/`d0`/`fa0` aliased to x86 `XMM0`. The FP memory probes exercise compiler
emitted global FP loads and stack FP spill/reload forms. The FP conditional
probes exercise compiler-emitted AArch64 `fcmpe` plus condition branches and
RISC-V `flt.d` plus `fmv.d` select paths. The FP division probes exercise
compiler-emitted AArch64 `fdiv` and RISC-V `fdiv.d` paths. The FP unary probes
exercise compiler-emitted AArch64 `fneg`, AArch64 zero-immediate `fcmpe`, and
RISC-V `fmv.d.x` plus `fneg.d` paths. The FP absolute-value probes exercise
compiler-emitted AArch64 `fabs.s`/`fabs.d` and RISC-V `fabs.s`/`fabs.d`
pseudo-instructions through `fsgnjx`. The FP square-root probes exercise
compiler-emitted AArch64 `fsqrt.s`/`fsqrt.d` and RISC-V `fsqrt.s`/`fsqrt.d`.
The FP fused multiply-add probes exercise compiler-emitted AArch64
`fmadd.s`/`fmadd.d` and RISC-V `fmadd.s`/`fmadd.d`. The fused
multiply-add variant probes exercise compiler-emitted AArch64
`fmsub.s`/`fmsub.d`, `fnmadd.s`/`fnmadd.d`, and `fnmsub.s`/`fnmsub.d`, plus
RISC-V `fmsub.s`/`fmsub.d`, `fnmsub.s`/`fnmsub.d`, and
`fnmadd.s`/`fnmadd.d`. The FP min/max probes exercise compiler-emitted
AArch64 `fminnm.s`/`fminnm.d` plus `fmaxnm.s`/`fmaxnm.d`, and RISC-V
`fmin.s`/`fmin.d` plus `fmax.s`/`fmax.d`. The FP select probes exercise
compiler-emitted AArch64 `fcsel.s`/`fcsel.d` and RISC-V compare/branch plus
`fmv.s`/`fmv.d` select sequences.
The FP indexed-memory probes exercise
compiler-emitted AArch64 scalar FP register-offset `ldr`/`str` forms and
RISC-V shift/add plus `fld`/`fsd` indexed sequences. The FP conversion probes
exercise compiler-emitted AArch64 `fcvtzu` and RISC-V `fcvt.lu.d` for positive
finite double-to-unsigned-integer conversion plus AArch64 `fcvtzs` and RISC-V
`fcvt.l.d` for finite double-to-signed-integer conversion. The 32-bit FP
conversion probes exercise AArch64 `fcvtzs`/`fcvtzu` `w` destinations and
RISC-V `fcvt.w.d`/`fcvt.wu.d`. The scalar float/double conversion probes
exercise AArch64 `fcvt s,d`/`fcvt d,s` and RISC-V `fcvt.s.d`/`fcvt.d.s`.
The AArch64 FP integer-result probes exercise scalar FP/GPR `fmov` bit moves
and finite scalar float `fcvtzs`/`fcvtzu` conversions to `w`/`x` destinations.
The integer-to-FP conversion probes exercise compiler-emitted AArch64
`scvtf`/`ucvtf` from GPR and scalar FP/SIMD integer sources plus RISC-V
`fcvt.s.w`, `fcvt.d.l`, and `fcvt.d.lu`.
The FP import probes exercise real
PLT/GOT calls to
`poly_import_fp64_add` and `poly_import_fp32_add` and verify
descriptor-dispatched FP arguments and return values. The x86 import probes
exercise real PLT/GOT calls to `poly_import_x86_sum8` and verify that
AArch64/RISC-V seventh and eighth integer arguments land in standard x86 SysV
stack-argument slots.  The companion post-import probes return from the x86
helper back into foreign code and fold in a ninth foreign stack argument before
the outer `PCALL` returns. The x86 sum8 helper performs a nested compiled x86
helper call before returning, so the descriptor path is exercised across
ordinary x86 call/return activity inside the imported target. The
`poly_import_x86_fp64_sum8` probes call an x86 SysV helper with eight double
arguments, covering all `XMM0-XMM7`/`v0-v7`/`fa0-fa7` scalar FP argument
aliases. The `poly_import_x86_mixed_u64_fp64` probes call an x86 helper with
alternating integer and double arguments, covering the native ABIs' independent
GPR and FP argument counters. The imported-object
probes exercise real compiler-emitted GOT loads of
undefined `poly_import_value`. The function-pointer probes exercise compiler
emitted same-image data relocations to local function symbols plus native
indirect calls through `blr` or `jalr`. The needed-library probes exercise a
main foreign shared object calling an intermediate `DT_NEEDED` library, which
then calls a transitive same-ISA leaf dependency through ordinary dynamic
relocation metadata; the intermediate constructor also calls the leaf dependency
so the test covers dependency-before-dependent constructor ordering. The same
needed-library probes also expose destructor-written dependency state after
dependency teardown, including the transitive leaf destructor effect. The leaf
dependency also exports a data object consumed by the intermediate dependency,
covering cross-library object-symbol relocation. The
constructor probes execute compiler-emitted `DT_INIT_ARRAY` entries and
linker-selected `DT_INIT` functions before the requested foreign entrypoint.
The destructor probes execute compiler-emitted `DT_FINI_ARRAY` entries and
linker-selected `DT_FINI` functions during foreign-object teardown and verify
their effect on foreign static state. The RELRO probes exercise
compiler-produced `PT_GNU_RELRO` ranges containing relocated read-only data;
the loader applies relocations, applies PT_LOAD permissions, marks RELRO ranges
read-only, and then enters foreign constructors or entrypoints. The TLS probes exercise compiler-emitted AArch64 TLSDESC with
`mrs tpidr_el0`, AArch64 traditional
`R_AARCH64_TLS_DTPMOD64`/`R_AARCH64_TLS_DTPREL64` plus `__tls_get_addr`, and
RISC-V `__tls_get_addr` against a copied `PT_TLS` initial image supplied
through the `PCALL` TLS-base register, plus initial-exec
`R_AARCH64_TLS_TPREL64` and `R_RISCV_TLS_TPREL64` accesses against the same
TLS base, including undefined `STT_TLS` relocations bound to `DT_NEEDED`
dependencies. The conditional probes exercise compiler-emitted AArch64
logical-immediate `tst`, `csel`, and conditional-select variants
`csinc`/`csinv`/`csneg`, plus RISC-V branch/select patterns. The
compare-and-branch probes exercise compiler-emitted AArch64 `cbz`/`cbnz` on
non-`x0` registers and RISC-V ordinary branch forms. The bit-test branch probes
exercise compiler-emitted AArch64 `tbz`/`tbnz` plus matching RISC-V branch
sequences. The unsigned-bitfield
probes exercise compiler-emitted AArch64 `lsl`/`ubfx`/`bfxil`/`bfi` aliases and RISC-V
shift/mask sequences. The signed-extension probes exercise compiler-emitted
AArch64 `sxth`, `sbfx`, plus `add ... asr/sxtb/sxth/sxtw` and
RISC-V signed byte/halfword/word arithmetic. The integer-division probes
exercise compiler-emitted AArch64 `udiv`/`sdiv` plus RISC-V
`divu`/`div`/`divuw`/`divw`. The integer multiply-add probes exercise
compiler-emitted AArch64 `madd`/`msub` plus RISC-V `mul`/`add`/`sub`
sequences. The integer high-multiply probes exercise compiler-emitted AArch64
`umulh`/`smulh` plus RISC-V `mulhu`/`mulh` sequences. The integer carry-chain
probes exercise compiler-emitted AArch64 `adds`/`adc`/`subs`/`sbc` plus
RISC-V `add`/`sltu` sequences. The integer variable-shift probes exercise
compiler-emitted AArch64 `lsl`/`lsr`/`asr` register forms plus RISC-V
`sll`/`srl`/`sra` sequences. The integer logical probes exercise
compiler-emitted AArch64 `orn`/`bic`/`eon` and shifted-register `tst` plus
RISC-V logical and branch sequences. The integer bit-operation probes exercise
compiler-emitted AArch64 `rbit`/`rev16`/`rev`/`clz`/`cls` for ordinary bit
builtins plus RISC-V shift/branch fallback sequences. The integer rotate/extract
probes exercise compiler-emitted AArch64 `extr`/`ror` aliases and logical
`ror` operands plus RISC-V shift/or fallback sequences. The integer conditional
compare probes exercise compiler-emitted AArch64 `ccmp` for chained conditions
plus RISC-V branch fallback sequences. The AArch64 post-index memory probe
exercises single-register `ldr`/`str` post-index writeback forms. The AArch64
atomic probes exercise compiler-emitted exclusive `ldxr`/`ldaxr` plus
`stxr`/`stlxr`, `clrex`, compiler-emitted NAND LL/SC loops, 8-, 16-, 32-, and
64-bit LSE `ldadd`, `swp`, `ldclr`, `ldeor`, `ldset`, signed/unsigned min/max,
and `cas`, and GCC outline atomic helper imports for default compiler output. The RISC-V atomic
probe exercises
compiler-emitted `amoadd`, `amoswap`, `amoand`, `amoxor`, and `amoor` word and
dword forms, inline signed/unsigned `amomin`/`amomax` word and dword forms,
plus `lr.d`/`sc.d` and byte/halfword LR.W/SC.W loops from C
`__atomic` builtins, including subtract and NAND loops. The integer indexed-memory probes
exercise compiler-emitted AArch64 register-offset `ldr`/`str` forms and RISC-V
shift/add indexed load-store sequences. The scalar FP callee-saved probes
exercise compiler-emitted AArch64 `stp`/`ldp` of `d8` and later, scalar float
use of saved `s8` and later lanes, plus RISC-V `fsd`/`fld` of `fs0` and later
across imported calls. The compressed-word
probe exercises RISC-V `c.lw`/`c.sw` and `c.lwsp`/`c.swsp` forms. The
compressed-ALU probe exercises RISC-V `c.addiw`, `c.srli`, `c.srai`, `c.andi`,
`c.sub`, `c.xor`, `c.or`, `c.and`, `c.subw`, and `c.addw`. The compressed-FP
probe exercises RISC-V `c.fld`/`c.fsd` and `c.fldsp`/`c.fsdsp` double-FP
memory forms. The FP integer-result probes exercise RISC-V `fmv.x.w`,
`fmv.x.d`, `fclass.s`, `fclass.d`, and `fcvt.w.s`. It also
includes sectionless `dyntab` probes that exercise only
`PT_DYNAMIC` symbol metadata. PLT-style dynamic relocation tables are accepted
through `DT_JMPREL`/`DT_PLTRELSZ`, with either `DT_PLTREL=RELA` or
`DT_PLTREL=REL`, including `R_AARCH64_JUMP_SLOT` and `R_RISCV_JUMP_SLOT`
entries for defined symbols.
Packed relative relocation tables are accepted through
`DT_RELR`/`DT_RELRSZ`/`DT_RELRENT`, including direct entries and bitmap entries.
Traditional addend-in-place relative relocation tables are accepted through
`DT_REL`/`DT_RELSZ`/`DT_RELENT`.
Dynamic `R_AARCH64_NONE` and `R_RISCV_NONE` entries are accepted as no-ops.
IFUNC-style resolver relocations are accepted through
`R_AARCH64_IRELATIVE` and `R_RISCV_IRELATIVE`, and symbolic relocations whose
dynamic symbol type is `STT_GNU_IFUNC`, including IFUNC symbols exported by
loaded `DT_NEEDED` dependencies; the resolver runs in the foreign frontend after
ordinary relocations are applied, and its return value is written to the
relocation target.
Copy relocations are accepted through `R_AARCH64_COPY` and `R_RISCV_COPY`; the
loader resolves the exported object in loaded dependency scope and copies the
symbol contents into the executable image before foreign constructors or
entrypoints run.
Undefined object-symbol relocations can bind to process-provided imports; the
gate covers `poly_import_value` through an undefined dynamic symbol relocation.
Undefined weak object/function relocations resolve to zero, matching ordinary
ELF optional-symbol semantics; the weak-import probes cover AArch64 weak
`GLOB_DAT`/`JUMP_SLOT` and RISC-V weak symbolic relocations.
Dependency relocations for default-visibility defined symbols follow ordinary
ELF interposition unless the dependency advertises `DT_SYMBOLIC` or
`DF_SYMBOLIC` through `DT_FLAGS`; those `-Bsymbolic` objects bind their own
defined symbols locally. `STV_PROTECTED` definitions are exported but also bind
locally for the defining dependency's own relocations.
Same-directory `DT_NEEDED` dependencies are loaded as foreign shared libraries,
and undefined function or object-symbol relocations in the requesting object can
bind directly to dependency text/data without routing through an x86 import
descriptor. GNU symbol versions are matched during dependency binding, with
unversioned requests preferring default exports and explicit version requests
requiring both the matching dependency version definition and the matching
`DT_VERNEED` provider filename/SONAME. Non-exported dependency symbols are not
eligible for external binding even if they appear in dynamic symbol metadata.
Undefined relocations in dependencies may also bind to exportable symbols in
the root loaded foreign object, matching common callback/hook patterns in
ordinary dynamic linking. Root-exported `STT_GNU_IFUNC` symbols are resolved
by calling the root resolver in foreign mode before patching dependency
relocations.
Weak undefined foreign relocations first try the root/dependency scope and only
resolve to zero when no loaded object exports the requested symbol.
When multiple dependency libraries export the same function/object symbol, the
foreign loader ranks direct `DT_NEEDED` dependencies ahead of transitive
dependencies so a later direct library can interpose over an earlier indirect
provider.
Already loaded dependencies are reused for later ordinary SONAME-style
`DT_NEEDED` requests with the same SONAME, even when a later RUNPATH resolves
the same SONAME to a different path.
Dependencies recursively load their own same-directory `DT_NEEDED` entries, so
an intermediate foreign library can call a second foreign library through its
ordinary PLT/GOT relocations. Dependency library dynamic relocations are applied
before those dependency calls execute.
The harness builds direct plus transitive dependency sets with five loaded
foreign libraries, covering dependency trees beyond the original four-library
prototype cap, and direct dependency fans larger than eight libraries.
`DT_RUNPATH` is preferred over `DT_RPATH` when present, and `$ORIGIN/...`,
`${ORIGIN}/...`, `$LIB`/`${LIB}` token expansion to `lib`, and
`$PLATFORM`/`${PLATFORM}` expansion to the foreign frontend platform name
(`aarch64` or `riscv`), relative entries, and absolute dependency directories
are used to resolve dependency libraries before a missing needed library fails
to load. Colon-separated RUNPATH entries continue searching after a missing
directory. Old-style RPATH-only objects built without new dtags are covered,
including inherited RPATH lookup for transitive foreign dependencies whose
direct parent has no local RUNPATH. Inherited `$ORIGIN` entries expand against
the object that declared the RPATH, not the transitive dependency currently
being loaded. An object's local RUNPATH is not inherited by its children and
also stops inherited RPATH from leaking through that object. `POLY_LD_LIBRARY_PATH`
is searched for foreign dependencies before declared RUNPATH/RPATH directories
using the same `$ORIGIN`, `$LIB`, and `$PLATFORM` dynamic-string token expansion, with boot
coverage proving environment-path fallback after a missing directory and
environment-path precedence over a conflicting declared RUNPATH library.
If `POLY_LD_LIBRARY_PATH` is unset, the prototype falls back to
`LD_LIBRARY_PATH` for compatibility with older tests.
Absolute `DT_NEEDED` path strings are accepted directly by the user-space
loader, and `DT_NEEDED` strings may use dynamic-string tokens such as
`$ORIGIN/...`, `$ORIGIN/${LIB}/...`, and `$ORIGIN/${PLATFORM}/...`.
Declared RUNPATH/RPATH directories are searched before the loader's
compatibility fallback to the caller's directory, so a colocated same-SONAME
object does not override explicit dynamic-linker metadata.
Dependency `DT_INIT` and `DT_INIT_ARRAY` constructors run before entering
dependent foreign code, root PIE executables run `DT_PREINIT_ARRAY` entries
before ordinary constructors, and dependencies tagged with `DF_1_INITFIRST`
run their constructors before ordinary dependency constructors. Dependency
`DT_FINI_ARRAY` and `DT_FINI` destructors run during teardown. The
`depfini:` harness mode verifies dependency finalizers by calling an exported
dependency result symbol after dependency teardown and before unmapping.
Root and dependency `PT_GNU_RELRO` ranges are protected read-only after
ordinary, copy, and IFUNC relocations have been written and before any
foreign constructor, preinit callback, or entrypoint executes.
Root and dependency `PT_LOAD` pages are also protected according to their ELF
`PF_R`/`PF_W`/`PF_X` flags at that same point, with RELRO protection applied
after the broader load-segment permissions so RELRO is not reopened by a
writable data segment.
Dependency `PT_TLS` images are laid out in the same software-allocated TLS
block as the entry object, with each dependency's TLS relocations resolved to a
distinct offset under the `PCALL` TLS-base register. Dependency TLS
relocations may also bind to exportable `STT_TLS` symbols in the root loaded
foreign object, so dependency callbacks can share root TLS state.
Imported function symbols can bind to prototype hardware call-descriptor
slots. AArch64 `blr` or RISC-V `jalr` to a descriptor address maps the native
foreign argument registers through an x86/runtime import target, writes the
native foreign return register, and resumes at the foreign link address. The
gate covers deterministic `poly_import_add` and `poly_import_mul` descriptors,
proving that distinct undefined function symbols can dispatch through separate
descriptor slots. The compiler-produced import objects exercise real
PLT/GOT-backed `JUMP_SLOT` calls to `poly_import_add`: AArch64 PLT code may
branch with `br` after the caller's `bl` saved the continuation in `x30`, and
RISC-V PLT code may use 32-bit `jalr`, compressed `c.jalr`, or compressed
`c.jr` tail-call transfers while preserving the caller continuation in `ra`.
The descriptor path also accepts common libc
symbol names `strlen`, `strcmp`, `strncmp`, `memcpy`, `memmove`, `memset`,
`memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`, `strncpy`,
`strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`, `stpcpy`,
`stpncpy`, `mempcpy`, `memrchr`, `memmem`, `rawmemchr`, `strchrnul`,
`strcasecmp`, `strncasecmp`, `strcasestr`, `bcmp`, `bcopy`, `bzero`,
`index`, `rindex`, and `puts`, so
compiler-produced foreign objects can call those routines through ordinary
PLT/GOT entries without using synthetic breakpoint helpers. The neutral
cross-call gate also covers direct descriptor `strlen`, `strnlen`, `memset`,
`memcpy`, and three-argument `memcmp` calls inside foreign callees, proving
descriptor imports are not limited to x86-entered `PCALL` payloads. The gate
also covers `poly_import_x86_add`, `poly_import_x86_mul`,
`poly_import_x86_sum6`, `poly_import_x86_sum8`,
`poly_import_x86_fp64_add`, `poly_import_x86_fp64_sum8`,
`poly_import_x86_mixed_u64_fp64`, and `poly_import_x86_fp32_add`, where descriptor
slots select real x86_64 helper targets from a runtime-supplied table, map the
first six native foreign integer arguments to x86_64 SysV `RDI`, `RSI`, `RDX`,
`RCX`, `R8`, and `R9`, place seventh and eighth integer arguments in the
standard x86 stack-argument slots when needed, reuse the shared
`XMM0-XMM7`/`v0-v7`/`fa0-fa7` FP register aliases for scalar FP arguments and
returns, including a full eight-register double-argument x86 helper and an
alternating integer/double x86 helper that consumes both native argument
classes independently, synthesize an x86 return address to a nearby dedicated `0f 24` `PIRET`
landing pad, let the helper use an ordinary `ret`, and then resume the saved
AArch64/RISC-V return PC with the x86 `RAX` result mapped back to the native
foreign integer return register.  The `polycall` descriptor table currently
targets `noinline` x86_64 C helpers linked from
`tools/polycall_x86_helpers.c`, which verifies the call gate against a
separately compiled x86 helper object instead of trampoline-local handwritten
byte helpers. The eight-argument target includes a nested compiled x86 helper
call, so ordinary x86 calls inside the imported target are covered as well.
A raw x86 function address is still not itself a valid AArch64 or RISC-V branch
target; production hardware needs either this kind of architectural call gate
or an OS/runtime descriptor that names the x86 callable target and ABI metadata.
When a runtime supplies a descriptor entry for an import ID, the Bochs CPU uses
that descriptor as the import target.  `polycall` uses this path for
`poly_import_add`/`poly_import_mul`,
`poly_import_fp64_add`/`poly_import_fp32_add`, libc string/memory imports such as
`strlen`, `memcpy`, `memcmp`, and `puts`, and also for environment, allocation,
teardown, stack-failure, aux-vector/page-size,
errno, C++ guard imports such as `__cxa_guard_acquire`,
`__cxa_guard_release`, and `__cxa_guard_abort`, and process-query imports such
as `getenv`, `malloc`, `atexit`, `__stack_chk_fail`, `getauxval`,
`__errno_location`, and `getpid`, for
AArch64 TLSDESC, AArch64 traditional `__tls_get_addr`, and RISC-V
`__tls_get_addr` TLS accessors, and for scalar
libgcc 128-bit div/mod helpers `__udivti3`,
`__umodti3`, `__divti3`, and `__modti3`, scalar int128/float conversion
helpers `__fixdfti`, `__fixunsdfti`, `__floattidf`, `__floatuntidf`,
`__fixsfti`, `__fixunssfti`, `__floattisf`, and `__floatuntisf`, and bit
helpers `__clzdi2`, `__ctzdi2`, `__paritydi2`, and `__popcountdi2`, plus
16-byte libatomic helpers `__atomic_compare_exchange_16`,
`__atomic_load_16`, and `__atomic_store_16`, and quad-precision libgcc
helpers `__addtf3`, `__subtf3`, `__multf3`, `__divtf3`, `__floatunditf`,
`__fixunstfdi`, `__floatditf`, `__floatsitf`, `__floatunsitf`, `__fixtfdi`,
`__fixtfsi`, `__fixunstfsi`, `__eqtf2`, `__lttf2`, `__letf2`, `__gttf2`,
`__getf2`, `__netf2`, `__unordtf2`, `__extendsftf2`, `__extenddftf2`,
`__trunctfsf2`, and `__trunctfdf2`. Every valid import-window slot is
descriptor-backed or trap-backed: if the runtime supplies a descriptor, the CPU
uses the generic cross-frontend x86 call gate; otherwise it records an
architectural import trap with the fixed native ABI argument lanes. The CPU no
longer classifies individual libc, TLS, process-query, libatomic, or libgcc
helper IDs to decide whether a software descriptor is mandatory, and it does
not synthesize fixed fallback results.
The CPU contract is the descriptor call gate and ABI register mapping, not the
semantics of those library functions. Runtime descriptor targets are
responsible for ABI-specific details such as AAPCS64 even-register alignment
holes before 16-byte integer arguments; the CPU import gate always captures and
maps the fixed native argument lanes. RISC-V quad-precision helper descriptors
rebuild `__float128` operands from fixed `a0/a1` and `a2/a3` GPR lane pairs in
runtime x86 wrappers rather than requiring CPU-side XMM argument packing for
specific helper IDs. Each 32-byte x86 import descriptor is
`{target, trampoline, flags, reserved}`.  Flag bit `0` requests that the
seventh and eighth native GPR argument lanes be written to x86 stack-argument
slots, bit `1` maps an x86 `RDX:RAX` 128-bit integer return back to the native
foreign return register pair, and bit `2` maps an x86 `XMM0` binary128 return
back to AArch64 `v0` or RISC-V `a0/a1`. These flags are runtime metadata, not
CPU-side helper-ID policy.
The same descriptor path currently provides prototype imports for common GCC
dynamic TLS accessors (`R_AARCH64_TLSDESC`, AArch64
`R_AARCH64_TLS_DTPMOD64`/`R_AARCH64_TLS_DTPREL64` plus `__tls_get_addr`, and
RISC-V `__tls_get_addr`) through runtime x86 helper descriptors that consume
the `PCALL` TLS-base register.
Initial-exec `R_AARCH64_TLS_TPREL64`/`R_RISCV_TLS_TPREL64` relocations bind
directly to that same TLS base, including dependency-exported `STT_TLS`
symbols. The descriptor path also covers common GCC
AArch64 outline atomic helpers: 8-, 16-, 32-, and 64-bit `ldadd`, `swp`,
`ldclr`, `ldeor`, `ldset`, and `cas` with `relax`, `acq`, `rel`, and `acq_rel`
suffixes.  The suffixes alias to the same operation descriptors because the
prototype defines foreign atomic memory ordering in terms of the x86-TSO
execution model.  These are compatibility descriptors for observed compiler
output, not CPU-recognized helper names and not a general libgcc or libc
implementation.  Raw LSE additionally
covers signed/unsigned min/max RMW opcodes.

The prototype records a unified `POLYTRAP` state before leaving raw execution:

- Reason `0`: no trap.
- Reason `1`: foreign syscall trap (`svc` or `ecall`).
- Reason `2`: foreign breakpoint trap (`brk` or `ebreak`).
- Reason `3`: unresolved descriptor-backed foreign import.
- Reason `4`: unsupported or illegal raw foreign instruction.
- Mode records the raw source mode: `3` for AArch64, `4` for RISC-V.
- Number records the syscall number register for syscall traps (`x8` for
  AArch64, `a7` for RISC-V) or the breakpoint immediate/id for breakpoint
  traps. For import traps, it records the unresolved import descriptor id. For
  illegal-instruction traps, it records the raw 16-bit or 32-bit instruction
  word zero-extended to 32 bits.
- Selector records the raw trap selector/immediate where the instruction
  encoding provides one, for example AArch64 `svc #imm` or `brk #imm`; RISC-V
  `ecall`/`ebreak` record selector `0` because their service id comes from
  register state. Import traps record selector `0`. Illegal-instruction traps
  record the raw instruction length in bytes.
- Arguments record the first eight native foreign ABI argument registers.
- PC records the foreign instruction address that raised the trap. For import
  traps, it records the unresolved descriptor target address.
- Resume PC records the next foreign instruction address for trap return. For
  import traps, it records the native link-register return address.

Bochs no longer treats Linux syscalls or libc helpers as architectural CPU
behavior.  Foreign traps always use the OS-neutral architectural path.  The
contract is the precise trap exit plus explicit state that software can save,
restore, inspect, and route.

If an architectural trap vector was installed with `0f 24 60 ... POLY!`
(`RAX=handler_pc`), control transfers to that handler.  `0f 24 63 ... POLY!`
selects the handler frontend with `RAX=mode`; x86_64 is the default.  For an
x86 handler, delivery uses `RAX=reason`, `RBX=source mode`, `RCX=trap number`,
`RDX=trap PC`, `RSI=selector`, `RDI=arg0`, `R8`-`R12` for trap arguments
`1`-`5`, and `R13`-`R14` for trap arguments `6`-`7`; the same fields remain
available through trap-status opcodes for
debugging and late inspection.  For an AArch64 handler, delivery uses
`x0=reason`, `x1=source mode`, `x2=trap number`, `x3=trap PC`, `x4=selector`,
and `x5`-`x12` for trap arguments `0`-`7`.  For a RISC-V handler, delivery uses
`a0=reason`, `a1=source mode`, `a2=trap number`, `a3=trap PC`,
`a4=selector`, `a5`-`a7` for trap arguments `0`-`2`, and `t0`-`t2` for trap
arguments `3`-`5`, plus `t3`-`t4` for trap arguments `6`-`7`.  The handler
can inspect the full packet with trap-status
opcodes, place a result in the shared result register, and execute the native
trap-return instruction for its frontend: `0f 24 62 ... POLY!` from x86,
AArch64 `brk #0x7ff9`, or RISC-V custom `0x0000407b`.  Trap return restores
the source frontend's integer and scalar/vector FP state and commits only the
handler result register back to the source result register.  `0f 24 61 ...
POLY!` reads the current trap vector into `RAX`.
In the Bochs prototype, the installed trap vector, legacy syscall/break status
registers, recorded trap-packet/status state, trap-return save state, and
32-byte transition records are
keyed with the userspace poly state.  The key is guest `CR3`, user `FSBASE`,
and either an explicit software-selected state key or the 8 MiB stack-region
fallback key when the explicit key is zero.  A different guest address space
starts with no stale trap vector, no stale syscall/break status, no stale
trap packet, and no stale trap-return frame.  Software can also explicitly
export/import this keyed state with `0f 24 67/68 ... POLY!` and the fixed
4096-byte `struct poly_xsave_state` layout. CPUID state bit `11` advertises the
formal `0x40000004` hardware layout, and CPUID state bit `7` advertises the
matching XCR0/XSAVE-visible component.  Full OS context-switch coverage starts
when the guest OS opts into `XCR0[20]`; until then, the prototype's explicit
state-key and software save/restore paths remain the tested mechanism.
If no vector is installed, syscall and import traps surface as x86 `#UD`;
breakpoint traps surface as x86 `#BP`.  This keeps the CPU model OS-neutral:
software, not the CPU, decides whether a trap means Linux syscall translation,
a debugger breakpoint, a dynamic-linker binding, or something else.
`nativecheck.elf` verifies the corresponding guest signals: `SIGILL` for
foreign syscall/import traps and `SIGTRAP` for foreign breakpoint traps.

Breakpoint traps use the source frontend's native ABI argument registers:
AArch64 `x0`-`x5` or RISC-V `a0`-`a5`.  Runtime helper ids, dynamic-linker
bindings, and OS syscall translation are guest policy layered above this packet.

## Compatibility Rule

Precompiled cross-ISA libraries remain native ABI objects.  Boundary thunks are
responsible for mapping x86_64 SysV arguments to AAPCS64 or RISC-V psABI
registers, setting a native return target, entering raw fetch, and handling trap
exits.  The target ISA makes the common thunk operation a fast hardware `PCALL`;
software descriptors or thunks still handle ABI cases outside the fixed fast
path.  Direct register aliasing is an implementation optimization, not the
external ABI.
