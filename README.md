# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension.  The extension
keeps standard x86_64 execution as the host ISA, then adds CPUID-gated
prototype opcode-family operations that let selected AArch64 and RISC-V
instruction streams run through direct foreign fetch inside the x86_64 process.

This is an active scaffold, not a complete native-speed AArch64/RISC-V CPU.  The
current implementation validates the architecture shape, Linux boot path,
foreign ELF launch path, mixed-ISA transitions, explicit foreign trap records,
and a deterministic compatibility runtime for scaffolded syscall/libcall tests.
It does not yet implement full AArch64 or RISC-V ISA coverage, real foreign
Linux ABI passthrough, or equal-speed execution.

## Current State

- `scripts/boot.sh` downloads an Alpine Linux x86_64 kernel and BusyBox package,
  builds a minimal initramfs, creates a bootable ISO, and boots it in Bochs.
- The Docker image builds the local Bochs fork from
  `bochs-prepoly-src/bochs` and installs it as `bochs-poly`.
- The guest prints `BOOT_OK` on a clean baseline boot.
- The baseline `make boot` path runs `nativecheck.elf`, proving ordinary x86_64
  userspace still runs and the private poly CPUID leaves are hidden when
  `POLY_ENABLED=0`.
- The `make boot-poly-arch-traps` path disables the Bochs compatibility
  dispatcher and verifies that AArch64/RISC-V syscall and breakpoint traps route
  through the architectural trap vector.  Its guest x86 handler translates the
  foreign `getpid` syscall into a real x86 Linux `syscall`, then resumes the
  original raw frontend with `POLY_TRAP_RETURN`; it also runs generated
  AArch64/RISC-V `getpid` ELF payloads through the same disabled-compat path.
- With `POLY_ENABLED=1`, Bochs handles the polyglot userspace opcode-family
  operations and raw foreign fetch in `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`.
- `tools/polyprobe.c` validates raw AArch64 and RISC-V fetch/decode, wide
  register state, shared foreign stack-pointer frame handling, native returns,
  mixed raw instruction streams, repeated mixed-mode switch stress, mixed
  libcalls, mixed syscalls, and status/counter markers.
- `tools/polyapp.c` runs manifest-backed generated foreign ELF64 payloads from
  `tools/polyapps/*.poly` by entering raw foreign mode, executing packed
  32-bit foreign instructions, and escaping back to x86_64.  The manifest path
  accepts variable-size executable segments up to 1 MiB.
- `tools/polyexec.c` runs generated foreign ELF64 payloads directly by path
  using the same raw-mode execution path, preserving executable bytes exactly
  so RISC-V compressed 16-bit code does not have to be repacked as 32-bit
  words. The full boot gate includes a 6-byte compressed RISC-V ELF entry
  segment to cover this path.
- `tools/polycall.c` loads generated foreign ELF64 function payloads and calls
  their entrypoints through the prototype hardware ABI bridge (`PCALL`), so
  the return path uses ordinary AArch64/RISC-V return instructions rather than
  raw escape instructions. It also resolves prototype foreign `JUMP_SLOT`
  function imports to hardware call-descriptor slots; `tools/polycall_x86_helpers.c`
  supplies separately compiled x86_64 helper targets for the foreign-to-x86
  descriptor tests.
- `tools/polythread.c` runs real x86_64 pthreads that repeatedly enter long
  AArch64 and RISC-V `PCALL` loops while keeping hidden foreign integer and FP
  registers live, exercising guest thread-bank isolation and interrupt/`IRET64`
  raw-mode restoration across many foreign transitions.
- `tools/polysignal.c` arms real guest `SIGALRM` delivery while executing long
  raw AArch64 and RISC-V `PCALL` loops with hidden integer and FP foreign
  registers live, checking that the x86_64 signal handler and `rt_sigreturn`
  path resume the interrupted foreign frontend and synthetic bank correctly.
- `tools/polybench.c` executes long raw AArch64 and RISC-V loops inside the
  guest, verifies that raw instruction counters advance across multiple
  fetch/decode bursts, and checks mixed raw AArch64-to-RISC-V and
  RISC-V-to-AArch64 code blobs, including a compressed RISC-V instruction
  before a direct AArch64 switch, that switch, call, nest calls, and carry
  scalar double, mixed integer/FP, two-register integer returns, shared-stack
  values, caller callee-saved integer/FP registers, and syscall trap results
  directly without returning to x86.
- `tools/polybinfmt.sh` can register guest `binfmt_misc` entries so generated
  AArch64 and RISC-V ELF64 payloads execute directly from the x86_64 guest,
  including expected-result checks for compressed RISC-V fixtures.
- `docs/poly-isa.md` defines the silicon-oriented ISA contract: dedicated
  frontend-switch opcodes, XSAVE-visible foreign state, explicit trap exits,
  and native-ABI thunking for precompiled cross-ISA libraries.

## ISA Changes From Standard x86_64

The Bochs fork treats selected userspace x86 byte sequences as polyglot CPU
operations when `POLY_ENABLED=1`.  Normal x86_64 instructions are unchanged.
The preferred prototype hot path decodes a fixed `0f 24 <op> POLY!`
opcode-family placeholder through the `BX_IA_POLYMODE` handler; the runtime
tools use this opcode family for hot frontend switches, `PCALL`, and status
reads.  `UD2` is no longer an alternate polyglot envelope; ordinary `UD2`
retains standard invalid-opcode behavior.
The current handler accepts these operations only from guest userspace.

Preferred 8-byte x86 poly opcode-family operations:

| Operation | Bytes | Effect |
| --- | --- | --- |
| Switch to x86_64 mode | `0f 24 00 50 4f 4c 59 21` | Prototype `PEXIT`: sets current poly mode to x86_64. |
| Switch to raw AArch64 mode | `0f 24 01 50 4f 4c 59 21` | Prototype `PENTER.A64`: enters raw AArch64 direct fetch at the next byte. |
| Switch to raw RISC-V mode | `0f 24 02 50 4f 4c 59 21` | Prototype `PENTER.RV64`: enters raw RISC-V direct fetch at the next byte. |
| x86 SysV call to AArch64 | `0f 24 10 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV`: `R10=foreign target`, `R11=x86 return`, optional `R13=foreign TLS base`; maps x86_64 SysV integer args to AAPCS64 and enters raw AArch64. |
| x86 SysV call to RISC-V | `0f 24 11 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV`: `R10=foreign target`, `R11=x86 return`, optional `R13=foreign TLS base`; maps x86_64 SysV integer args to RISC-V psABI and enters raw RISC-V. |
| x86 SysV sret call to AArch64 | `0f 24 12 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to AAPCS64 `x8`, shifts user args back to `x0`-`x7`, and enters raw AArch64. |
| x86 SysV sret call to RISC-V | `0f 24 13 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to RISC-V `a0`, shifts user args to `a1`-`a7`, and enters raw RISC-V. |
| x86 SysV call to AArch64 with two-float return | `0f 24 14 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FPAIR32RET`: same scalar argument mapping as `PCALL.A64.SYSV`, but packs AAPCS64 `s0`/`s1` into x86_64 SysV `XMM0[63:0]` on return. |
| x86 SysV call to RISC-V with two-float return | `0f 24 15 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FPAIR32RET`: same scalar argument mapping as `PCALL.RV64.SYSV`, but packs RISC-V `fa0`/`fa1` into x86_64 SysV `XMM0[63:0]` on return. |
| x86 SysV call to AArch64 with two-float argument | `0f 24 16 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FPAIR32ARG`: unpacks x86_64 SysV `XMM0[63:0]` into AAPCS64 `s0`/`s1` and shifts following FP argument lanes up by one foreign FP register. |
| x86 SysV call to RISC-V with two-float argument | `0f 24 17 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FPAIR32ARG`: unpacks x86_64 SysV `XMM0[63:0]` into RISC-V `fa0`/`fa1` and shifts following FP argument lanes up by one foreign FP register. |
| x86 SysV call to AArch64 with `{u64,double}` aggregate | `0f 24 18 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_U64_F64`: maps x86_64 SysV `RDI`/`XMM0` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `RAX`/`XMM0`. |
| x86 SysV call to AArch64 with `{double,u64}` aggregate | `0f 24 19 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_F64_U64`: maps x86_64 SysV `XMM0`/`RDI` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `XMM0`/`RAX`. |
| x86 SysV call to AArch64 with `{u64,float}` aggregate | `0f 24 1a 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_U64_F32`: maps x86_64 SysV `RDI`/`XMM0[31:0]` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `RAX`/`XMM0[31:0]`. |
| x86 SysV call to AArch64 with `{float,u64}` aggregate | `0f 24 1b 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_F32_U64`: maps x86_64 SysV `XMM0[31:0]`/`RDI` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `XMM0[31:0]`/`RAX`. |
| x86 SysV call to RISC-V with `{u32,float}` compact aggregate | `0f 24 1c 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.COMPACT_U32_F32`: unpacks the x86_64 SysV packed aggregate from `RDI` into RISC-V `a0`/`fa0`, shifts the following integer argument to `a1`, and repacks the returned `a0`/`fa0` lanes into `RAX`. |
| x86 SysV call to RISC-V with `{float,u32}` compact aggregate | `0f 24 1d 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.COMPACT_F32_U32`: unpacks the x86_64 SysV packed aggregate from `RDI` into RISC-V `fa0`/`a0`, shifts the following integer argument to `a1`, and repacks the returned `fa0`/`a0` lanes into `RAX`. |
| x86 SysV FP-stack call to AArch64 | `0f 24 1e 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FP64STACK`: same scalar FP register aliases as `PCALL.A64.SYSV`, but copies x86_64 SysV FP overflow stack arguments from `[RSP+8]` to the foreign stack window. |
| x86 SysV FP-overflow call to RISC-V | `0f 24 1f 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FP64STACK`: same scalar FP register aliases as `PCALL.RV64.SYSV`, but maps up to eight x86_64 SysV FP overflow stack arguments from `[RSP+8]` onward into RISC-V `a0`-`a7` as required by psABI once `fa0`-`fa7` are consumed. |
| x86 import return | `0f 24 20 50 4f 4c 59 21` | Prototype `PIRET`: resumes the saved foreign return PC after an x86 helper returns normally from a descriptor-driven import call. |
| Syscall status | `0f 24 30+id 50 4f 4c 59 21` | Returns syscall state in `RAX`: `id=0` current mode, `id=1` last foreign syscall number, `id=2` last foreign syscall mode. |
| Libcall status | `0f 24 38+id 50 4f 4c 59 21` | Returns libcall state in `RAX`: `id=1` last libcall number, `id=2` last libcall mode. |
| Switch/status counters | `0f 24 40+id 50 4f 4c 59 21` | Returns mode/counter state in `RAX`: `id=0` switches, `id=1` current mode, `id=2` foreign raw instructions, `id=3` foreign syscalls, `id=4` foreign libcalls. |
| Trap status | `0f 24 50+id 50 4f 4c 59 21` | Returns last foreign trap state in `RAX`: `id=0` reason, `id=1` source mode, `id=2` number, `id=3`-`8` args, `id=9` trap PC, `id=10` trap selector/immediate, `id=11` resume PC. |
| Trap vector | `0f 24 60/61/62 50 4f 4c 59 21` | `0x60` sets the architectural trap vector from `RAX`, `0x61` reads it into `RAX`, and `0x62` resumes the recorded source frontend at the trap resume PC. |

When `POLY_ENABLED=1`, the prototype exposes a private CPUID discovery leaf for
runtime dispatch:

| Leaf | Registers | Meaning |
| --- | --- | --- |
| `0x40000000` | `EAX=0x40000003`, `EBX:EDX:ECX="PolyglotCPU!"` | Advertises the maximum poly CPUID leaf and the 12-byte poly vendor string. |
| `0x40000001` | `EAX=1`, `EBX=mode mask`, `ECX=feature mask`, `EDX=0` | Reports poly CPUID ABI version 1, supported frontend modes, implemented prototype features, and no architectural XSAVE component yet. |
| `0x40000002, subleaf 0` | `EAX[15:0]=0x7fff`, `EAX[31:16]=0x7ffe`, `EBX=0x7ffd`, `ECX=0x0000000b`, `EDX=0x0000002b` | Reports native raw-mode escape/cross-switch encodings: AArch64-to-x86, AArch64-to-RISC-V switch, AArch64-to-RISC-V call, RISC-V-to-x86, and RISC-V-to-AArch64 switch. |
| `0x40000002, subleaf 1` | `EAX=0x0000005b`, `EBX=0x0000107b`, `ECX=0x0000207b`, `EDX=0` | Reports the RISC-V-to-AArch64 native cross-call encoding and compact `{u32,float}`/`{float,u32}` native ABI cross-call variants. |
| `0x40000002, subleaf 2` | `EAX=106`, `EBX=8`, `ECX=16`, `EDX=16` | Reports the first prototype foreign-to-x86 import descriptor slot id, slot count, descriptor byte size, and import-call stride. |
| `0x40000002, subleaf 3` | `EAX=0x7ffa`, `EBX=0x0000307b`, `ECX=0`, `EDX=0` | Reports the neutral FP64 overflow stack-argument cross-call encodings: AArch64 `brk #0x7ffa` to RISC-V and RISC-V custom `0x0000307b` to AArch64. |
| `0x40000003` | `EAX=state flags`, `EBX=23`, `ECX=0`, `EDX=0` | Reports the prototype foreign-state contract: overlapping x86-visible GPR/FP state plus hidden synthetic banks keyed by `CR3`, `FSBASE`, and an 8 MiB stack-region key. `ECX=0`/`EDX=0` means no XCR0 component or XSAVE byte area is assigned yet. |

The current `0x40000001.EBX` mode mask sets bits `0`, `3`, and `4` for x86_64,
raw AArch64, and raw RISC-V.  `0x40000001.ECX` sets bits for raw AArch64, raw
RISC-V, neutral direct switches, native return cookies, x86 SysV `PCALL`,
`PCALL` sret, scalar FP bridging, trap records, user return restoration, x86 TSO
foreign ordering, and per-thread synthetic banks.  Bit `11` advertises the
optional Bochs deterministic syscall/libcall compatibility runtime when
`cpu.poly_compat_traps=1`; it is clear when trap-only architectural exits are
configured.  Bit `12` additionally advertises the prototype x86 poly opcode
family. Bit `13` advertises the two-float aggregate return packing
variants for native ABI `PCALL`; bit `14` advertises two-float aggregate
argument unpacking; bits `15`-`18` advertise the `{u64,double}`,
`{double,u64}`, `{u64,float}`, and `{float,u64}` heterogeneous aggregate
bridges for AArch64 `PCALL`; bits `19`-`20` advertise the RISC-V
`{u32,float}` and `{float,u32}` compact aggregate bridges; bit `21` advertises
the corresponding neutral AArch64<->RISC-V compact aggregate cross-call
bridges; bit `22` advertises runtime-supplied foreign-to-x86 import descriptor
slots; bit `23` advertises FP64 overflow stack-argument `PCALL` variants; bit
`24` advertises neutral AArch64<->RISC-V FP64 overflow stack-argument
cross-call variants; bit `25` advertises the architectural trap vector and
trap-return path.  The same
double-lane bridge forms also cover the ABI-compatible `{u32,double}` and
`{double,u32}` shapes.
`0x40000003.EAX` reports the current state-management contract. Bits `0`-`6`
mean overlapping x86 GPR/FP state, prototype synthetic banks, `CR3` keying,
`FSBASE` keying, stack-region keying, user-return restore support, and x86 TSO
foreign ordering. Bit `7` is intentionally clear until foreign state is exposed
as an architectural XSAVE component.

Foreign execution always uses raw direct fetch.  Bochs enters raw mode through
the x86_64 poly opcode, bypasses x86 decode, and fetches foreign
instructions directly from `RIP`: fixed 32-bit instructions for AArch64 and
mixed 16/32-bit instructions for RISC-V.  AArch64 `brk #0x7fff` and RISC-V
custom-0 instruction `0x0000000b` escape back to x86_64 at the next byte.
AArch64 `brk #0x7ffe` switches directly to raw RISC-V, and RISC-V custom-1
instruction `0x0000002b` switches directly to raw AArch64, preserving the
shared low integer result/argument register state instead of routing through
x86.  AArch64 `brk #0x7ffd` is a prototype neutral call gate to a RISC-V target
address in `x16` with an AArch64 return PC in `x17`; RISC-V custom-2
`0x0000005b` is the reverse call gate to an AArch64 target in `x5` with a
RISC-V return PC in `x6`.  AArch64 `brk #0x7ffc`/`brk #0x7ffb` and RISC-V
custom opcodes `0x0000107b`/`0x0000207b` are compact `{u32,float}` and
`{float,u32}` native ABI variants for direct AArch64<->RISC-V calls.  AArch64
`brk #0x7ffa` and RISC-V custom opcode `0x0000307b` are FP64 overflow
stack-argument variants; they preserve the shared `d0`-`d7`/`fa0`-`fa7`
aliases while mapping the eight extra double lanes between AArch64 stack slots
and RISC-V `a0`-`a7`.  The callee returns with its ordinary native return
instruction to a hardware cookie, which restores the caller frontend mode and
maps the shared integer and scalar FP argument/result registers back.  Raw
cross-call returns use a small bounded hardware-style return stack in the
prototype, so nested AArch64-to-RISC-V-to-AArch64 calls can unwind through
ordinary native returns without an x86 trampoline.  Raw
foreign fetch is only active at guest CPL3; kernel, interrupt, and exception
paths continue through normal x86_64 decode even if the current userspace poly
mode is raw AArch64 or raw RISC-V.  When a long-mode interrupt hits raw
foreign fetch, the prototype records the interrupted foreign frontend mode and
RIP in the synthetic bank, lets the x86_64 kernel run as x86, and restores raw
mode after `IRET64` returns to the recorded user RIP.  Raw fetch is bound to
the guest `CR3`, user `FSBASE`, and stack-region bank key, so unrelated
userspace tasks and common pthread stacks do not inherit raw decoding after a
scheduler switch or a fault in the raw-mode task.
The current raw run loop batches up to 64 raw foreign instructions before
returning to the outer Bochs event loop, while still checking async events and
mode exits between individual raw instructions.
Raw native returns stay native: AArch64 `ret` branches to `x30`/the selected
link register and RISC-V `ret` is handled as ordinary `jalr x0, 0(ra)`.  Raw
loader tests may still synthesize an explicit return landing pad by setting
AArch64 `x30` or RISC-V `ra` to a following x86 escape instruction.  The newer
prototype `PCALL` forms set a hardware-style return cookie in `x30` or `ra`;
ordinary foreign `ret` to that cookie switches back to x86_64 at the `R11`
return address.

The hybrid CPU currently defines foreign-mode memory ordering as x86_64 TSO.
AArch64 `dmb`, `dsb`, and `isb` barriers and RISC-V `fence` and `fence.i`
instructions are decoded as ordering-preserving no-ops instead of introducing
weaker AArch64/RISC-V reordering inside Bochs.
Foreign atomic operations use the same Bochs virtual-memory path as ordinary
foreign loads and stores.  The current prototype covers compiler-emitted
AArch64 exclusive and LSE atomics, GCC AArch64 outline atomic helper imports,
RISC-V A-extension word/dword AMOs including signed and unsigned min/max, and
the LR.W/SC.W loops GCC emits for RISC-V byte and halfword atomics, including
compiler-emitted subtract and NAND loops.

The current register bridge aliases the overlapping caller-visible integer ABI:

- x86_64 `RAX` carries the foreign return value and maps to AArch64 `x0` or
  RISC-V `a0`.
- x86_64 `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to AArch64 `x1`-`x6`
  and RISC-V `a1`-`a6`.
- x86_64 `RSP` maps to RISC-V `sp` and to AArch64 `SP` where the AArch64
  encoding defines register 31 as stack pointer.  AArch64 operands that define
  register 31 as zero register still read as `XZR/WZR`.
- x86_64 `XMM0`-`XMM7` low lanes map to AArch64 scalar `s0`/`d0`-`s7`/`d7`
  and RISC-V `fa0`-`fa7` for the currently decoded scalar float/double FP
  subset.
- Bochs tracks the remaining foreign integer registers in synthetic banks keyed
  by guest `CR3` and user `FSBASE`: AArch64 `x7`-`x30` plus syscall scratch
  `x8`, and RISC-V non-aliased registers including `a7`.

Precompiled cross-ISA linking is expected to use native ABI contracts, not a
custom compiler ABI.  The prototype `PCALL.A64.SYSV` and `PCALL.RV64.SYSV`
forms move the common thunk work into the emulated ISA: they map x86_64 SysV
integer arguments into AAPCS64 `x0`-`x7` or RISC-V psABI `a0`-`a7`; x86 args
1-6 come from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9`, while args 7-8 come
from the SysV stack slots at `[RSP+8]` and `[RSP+16]`.  During the foreign call,
the hardware bridge saves the x86 `RSP`, exposes a separate 16-byte-aligned
foreign `SP` window below the x86 frame, and copies integer overflow stack
arguments from `[RSP+24]` onward so the first foreign stack argument is visible
at `[sp]`; AArch64 FP64 stack-argument variants copy from `[RSP+8]` because
x86_64 SysV uses the overflow stack only after `XMM0`-`XMM7` are consumed, while
the RISC-V variant maps those overflow FP lanes into `a0`-`a7` per psABI.  x86
`RSP` is restored when the native foreign return hits the return cookie.  The
bridge preserves the shared `XMM0`-`XMM7` FP argument/return aliases, including
two-register homogeneous double aggregate arguments and returns through
`XMM0`/`XMM1`, two-`float` homogeneous aggregate returns packed into
`XMM0[63:0]`, two-`float` homogeneous aggregate arguments unpacked from
`XMM0[63:0]` into two foreign FP argument registers, including mixed integer/FP signatures where GPR and XMM lanes
are consumed in one native call.  AArch64 also has focused bridges for common
heterogeneous `{u64,double}`, `{double,u64}`, `{u64,float}`, and
`{float,u64}` aggregate shapes, moving FP lanes between x86 `XMM0` and
AAPCS64 integer-register bit lanes.  The `{u64,double}` and `{double,u64}`
bridge forms also cover the corresponding `{u32,double}` and `{double,u32}`
native ABI layouts because x86 SysV, AAPCS64, and RISC-V psABI keep the
integer lane in the same logical argument/result position with narrower
low-bit payloads.  RISC-V also has focused compact aggregate bridges for
`{u32,float}` and `{float,u32}`, where x86 SysV and AAPCS64 use one packed GPR
but RISC-V psABI splits the same value across `a0` and `fa0`.  The neutral
AArch64<->RISC-V cross-call opcodes have matching compact aggregate variants,
so those packed/split mappings do not need an x86 rendezvous.  The bridge sets `x30` or `ra` to a return cookie, enters raw fetch at the
`R10` target, carries an optional foreign TLS block base in x86 `R13`, maps
AArch64 `x0` or RISC-V `a0` back to x86 `RAX`, and maps
AArch64 `x1` or RISC-V `a1` back to x86 `RDX` for ordinary two-word integer
aggregate returns.  The
prototype `SRET` variants cover larger memory-return aggregates by mapping the
x86 hidden result pointer to AArch64 `x8` or RISC-V `a0` and shifting the
remaining user arguments according to the target ABI.  `polycall`
verifies this against loaded foreign ELF64 function payloads
(`aarch64-pcall-sum.elf`, `riscv-pcall-sum.elf`, `aarch64-pcall-sum8.elf`,
`riscv-pcall-sum8.elf`, `aarch64-pcall-sum9.elf`,
`riscv-pcall-sum9.elf`, and compiler-produced AArch64/RISC-V shared objects
(`aarch64-pcall-real.so#poly_entry`, `riscv-pcall-real.so#poly_entry`,
`riscv-pcall-real-rv64gc.so#poly_entry`,
`aarch64-pcall-gnu-hash-real.so#poly_entry`,
`riscv-pcall-gnu-hash-real.so#poly_entry`,
`aarch64-pcall-state.so#poly_entry`, and
`riscv-pcall-state.so#poly_entry`) plus compiler-built imported-function
objects (`aarch64-pcall-import-real.so#poly_entry` and
`riscv-pcall-import-real.so#poly_entry`,
`riscv-pcall-import-real-rv64gc.so#poly_entry`), compiler-built libc-style imported
function objects (`aarch64-pcall-libc-import-real.so#poly_entry` and
`riscv-pcall-libc-import-real.so#poly_entry`) that call `strlen`, `strcmp`, `strncmp`, `memcpy`,
`memmove`, `memset`, `memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`,
`strncpy`, `strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`,
`stpcpy`, `stpncpy`, `mempcpy`, `memrchr`, `memmem`, `rawmemchr`,
`strchrnul`, `strcasecmp`, `strncasecmp`, `strcasestr`, `bcmp`, `bcopy`, `bzero`, `index`, and `rindex` through
PLT/GOT descriptors, compiler-built imported-object
objects (`aarch64-pcall-import-value-real.so#poly_entry` and
`riscv-pcall-import-value-real.so#poly_entry`), compiler-built weak undefined
import objects (`aarch64-pcall-weak-import-real.so#poly_entry` and
`riscv-pcall-weak-import-real.so#poly_entry`), compiler-built stack-protected
objects (`aarch64-pcall-stack-protector-real.so#poly_entry` and
`riscv-pcall-stack-protector-real.so#poly_entry`), compiler-built errno-access
objects (`aarch64-pcall-errno-real.so#poly_entry` and
`riscv-pcall-errno-real.so#poly_entry`), compiler-built aux-vector dispatch
objects (`aarch64-pcall-getauxval-real.so#poly_entry` and
`riscv-pcall-getauxval-real.so#poly_entry`), compiler-built page-size query
objects (`aarch64-pcall-getpagesize-real.so#poly_entry` and
`riscv-pcall-getpagesize-real.so#poly_entry`) plus compiler-built `sysconf`
page-size query objects (`aarch64-pcall-sysconf-real.so#poly_entry` and
`riscv-pcall-sysconf-real.so#poly_entry`) and compiler-built environment-query
objects (`aarch64-pcall-env-real.so#poly_entry` and
`riscv-pcall-env-real.so#poly_entry`), compiler-built allocator objects
(`aarch64-pcall-alloc-real.so#poly_entry` and
`riscv-pcall-alloc-real.so#poly_entry`) plus allocator-backed string-copy
objects (`aarch64-pcall-strdup-real.so#poly_entry` and
`riscv-pcall-strdup-real.so#poly_entry`) and aligned-allocation objects
(`aarch64-pcall-aligned-alloc-real.so#poly_entry` and
`riscv-pcall-aligned-alloc-real.so#poly_entry`) plus teardown-registration
objects (`aarch64-pcall-atexit-real.so#poly_entry` and
`riscv-pcall-atexit-real.so#poly_entry`) and process-query objects
(`aarch64-pcall-process-real.so#poly_entry` and
`riscv-pcall-process-real.so#poly_entry`), compiler-built `DT_NEEDED`
shared-library pairs (`aarch64-pcall-needed-real.so#poly_entry` with
`libpolyneeded-aarch64.so` and its leaf dependency
`libpolyneeded-leaf-aarch64.so`, and
`riscv-pcall-needed-real.so#poly_entry` with `libpolyneeded-riscv.so` and its
leaf dependency `libpolyneeded-leaf-riscv.so`), compiler-built relocated
function-pointer objects (`aarch64-pcall-funcptr-real.so#poly_entry` and
`riscv-pcall-funcptr-real.so#poly_entry`), compiler-built two-word aggregate
return objects (`aarch64-pcall-pair-real.so#poly_entry` and
`riscv-pcall-pair-real.so#poly_entry`), compiler-built hidden-sret aggregate
return objects (`aarch64-pcall-sret-real.so#poly_entry` and
`riscv-pcall-sret-real.so#poly_entry`), compiler-built constructor/destructor
objects (`aarch64-pcall-ctor-real.so#poly_entry`,
`riscv-pcall-ctor-real.so#poly_entry`,
`aarch64-pcall-fini-real.so#poly_entry`, and
`riscv-pcall-fini-real.so#poly_entry`), compiler-built TLS objects
(`aarch64-pcall-tls-real.so#poly_entry` and
`riscv-pcall-tls-real.so#poly_entry`) plus initial-exec TLS objects
(`aarch64-pcall-tls-ie-real.so#poly_entry` and
`riscv-pcall-tls-ie-real.so#poly_entry`), compiler-built conditional objects
(`aarch64-pcall-cond-real.so#poly_entry` and
`riscv-pcall-cond-real.so#poly_entry`), compiler-built compare-and-branch
objects (`aarch64-pcall-cbz-real.so#poly_entry` and
`riscv-pcall-cbz-real.so#poly_entry`), compiler-built bit-test branch
objects (`aarch64-pcall-bitbranch-real.so#poly_entry` and
`riscv-pcall-bitbranch-real.so#poly_entry`), compiler-built signed-extension
objects (`aarch64-pcall-signed-ext-real.so#poly_entry` and
`riscv-pcall-signed-ext-real.so#poly_entry`), compiler-built signed-load
objects (`aarch64-pcall-signed-load-real.so#poly_entry` and
`riscv-pcall-signed-load-real.so#poly_entry`), compiler-built integer-division
objects (`aarch64-pcall-int-div-real.so#poly_entry` and
`riscv-pcall-int-div-real.so#poly_entry`), compiler-built integer multiply-add
objects (`aarch64-pcall-int-madd-real.so#poly_entry` and
`riscv-pcall-int-madd-real.so#poly_entry`), compiler-built integer high-multiply
objects (`aarch64-pcall-int-highmul-real.so#poly_entry` and
`riscv-pcall-int-highmul-real.so#poly_entry`), compiler-built 128-bit
division/modulo helper objects (`aarch64-pcall-int128-helpers-real.so#poly_entry`
and `riscv-pcall-int128-helpers-real.so#poly_entry`), compiler-built 128-bit
integer/double conversion helper objects
(`aarch64-pcall-int128-fp-helpers-real.so#poly_entry` and
`riscv-pcall-int128-fp-helpers-real.so#poly_entry`), compiler-built 128-bit
integer/float conversion helper objects
(`aarch64-pcall-int128-float-helpers-real.so#poly_entry` and
`riscv-pcall-int128-float-helpers-real.so#poly_entry`), compiler-built libgcc
bit helper objects (`aarch64-pcall-bit-helpers-real.so#poly_entry` and
`riscv-pcall-bit-helpers-real.so#poly_entry`), compiler-built quad-precision
`long double` helper objects (`aarch64-pcall-longdouble-helpers-real.so#poly_entry`,
`riscv-pcall-longdouble-helpers-real.so#poly_entry`,
`aarch64-pcall-longdouble-signed-helpers-real.so#poly_entry`, and
`riscv-pcall-longdouble-signed-helpers-real.so#poly_entry`), compiler-built
quad-precision comparison/conversion helper objects
(`aarch64-pcall-longdouble-compare-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-compare-helpers-real.so#poly_entry`), compiler-built
quad-precision 32-bit integer conversion helper objects
(`aarch64-pcall-longdouble-int32-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-int32-helpers-real.so#poly_entry`), compiler-built integer carry-chain
objects (`aarch64-pcall-int-carry-real.so#poly_entry` and
`riscv-pcall-int-carry-real.so#poly_entry`), compiler-built integer variable-shift
objects (`aarch64-pcall-int-varshift-real.so#poly_entry` and
`riscv-pcall-int-varshift-real.so#poly_entry`), compiler-built integer logical
objects (`aarch64-pcall-int-logic-real.so#poly_entry` and
`riscv-pcall-int-logic-real.so#poly_entry`), compiler-built integer bit-operation
objects (`aarch64-pcall-int-bitops-real.so#poly_entry` and
`riscv-pcall-int-bitops-real.so#poly_entry`), compiler-built integer rotate/extract
objects (`aarch64-pcall-int-rotate-real.so#poly_entry` and
`riscv-pcall-int-rotate-real.so#poly_entry`), compiler-built integer conditional
compare objects (`aarch64-pcall-int-ccmp-real.so#poly_entry` and
`riscv-pcall-int-ccmp-real.so#poly_entry`), AArch64 post-index memory
object (`aarch64-pcall-postindex-mem.so#poly_entry`), AArch64 atomic objects
covering exclusive LL/SC, default GCC outline helpers, LSE instructions, and
16-byte libatomic helper imports
(`aarch64-pcall-atomic.so#poly_entry`,
`aarch64-pcall-atomic-outline.so#poly_entry`, and
`aarch64-pcall-atomic-lse.so#poly_entry`), RISC-V atomic object with AMO and
16-byte libatomic helper import coverage
(`riscv-pcall-atomic.so#poly_entry`), compiler-built unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-built indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-built callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), compiler-built scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`) plus sixteen-double FP stack-argument objects
(`aarch64-pcall-fp64-stack-real.so#poly_entry` and
`riscv-pcall-fp64-stack-real.so#poly_entry`), compiler-built homogeneous double
aggregate return objects (`aarch64-pcall-fpair-real.so#poly_entry` and
`riscv-pcall-fpair-real.so#poly_entry`), compiler-built homogeneous float
aggregate return objects (`aarch64-pcall-fpair32-real.so#poly_entry` and
`riscv-pcall-fpair32-real.so#poly_entry`), compiler-built homogeneous double
aggregate argument objects (`aarch64-pcall-fpair-arg-real.so#poly_entry` and
`riscv-pcall-fpair-arg-real.so#poly_entry`), compiler-built homogeneous float
aggregate argument objects (`aarch64-pcall-fpair32-arg-real.so#poly_entry` and
`riscv-pcall-fpair32-arg-real.so#poly_entry`), compiler-built mixed integer/FP
argument objects (`aarch64-pcall-mixed-args-real.so#poly_entry` and
`riscv-pcall-mixed-args-real.so#poly_entry`), compiler-built heterogeneous
aggregate objects (`aarch64-pcall-hetero-real.so#poly_entry` and
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
`riscv-pcall-hetero-f32-u32-real.so#poly_entry`), compiler-built scalar double and
float FP import objects (`aarch64-pcall-fp64-import-real.so#poly_entry`,
`riscv-pcall-fp64-import-real.so#poly_entry`,
`aarch64-pcall-fp32-import-real.so#poly_entry`, and
`riscv-pcall-fp32-import-real.so#poly_entry`), compiler-built scalar double FP
callee-saved objects (`aarch64-pcall-fp64-callee-real.so#poly_entry` and
`riscv-pcall-fp64-callee-real.so#poly_entry`), compiler-built scalar float FP
callee-saved objects (`aarch64-pcall-fp32-callee-real.so#poly_entry` and
`riscv-pcall-fp32-callee-real.so#poly_entry`), compiler-built scalar double FP
conditional objects (`aarch64-pcall-fp64-cond-real.so#poly_entry` and
`riscv-pcall-fp64-cond-real.so#poly_entry`), compiler-built scalar double FP
division objects (`aarch64-pcall-fp64-div-real.so#poly_entry` and
`riscv-pcall-fp64-div-real.so#poly_entry`), compiler-built scalar double FP
unary/zero-compare objects (`aarch64-pcall-fp64-unary-real.so#poly_entry` and
`riscv-pcall-fp64-unary-real.so#poly_entry`), compiler-built scalar double FP
absolute-value objects (`aarch64-pcall-fp64-abs-real.so#poly_entry` and
`riscv-pcall-fp64-abs-real.so#poly_entry`), compiler-built scalar double FP
square-root objects (`aarch64-pcall-fp64-sqrt-real.so#poly_entry` and
`riscv-pcall-fp64-sqrt-real.so#poly_entry`), compiler-built scalar double FP
fused multiply-add objects (`aarch64-pcall-fp64-fma-real.so#poly_entry` and
`riscv-pcall-fp64-fma-real.so#poly_entry`) plus fused multiply-add variant
objects (`aarch64-pcall-fp64-fma-variants-real.so#poly_entry` and
`riscv-pcall-fp64-fma-variants-real.so#poly_entry`), compiler-built scalar double
FP min/max objects (`aarch64-pcall-fp64-minmax-real.so#poly_entry` and
`riscv-pcall-fp64-minmax-real.so#poly_entry`), compiler-built scalar double FP
select objects (`aarch64-pcall-fp64-select-real.so#poly_entry` and
`riscv-pcall-fp64-select-real.so#poly_entry`), compiler-built scalar double FP
indexed-memory objects (`aarch64-pcall-fp64-indexed-mem-real.so#poly_entry` and
`riscv-pcall-fp64-indexed-mem-real.so#poly_entry`), compiler-built scalar double
FP conversion objects (`aarch64-pcall-fp64-convert-real.so#poly_entry` and
`riscv-pcall-fp64-convert-real.so#poly_entry`) plus signed conversion objects
(`aarch64-pcall-fp64-signed-convert-real.so#poly_entry` and
`riscv-pcall-fp64-signed-convert-real.so#poly_entry`) and 32-bit conversion
objects (`aarch64-pcall-fp64-i32-convert-real.so#poly_entry`,
`riscv-pcall-fp64-i32-convert-real.so#poly_entry`,
`aarch64-pcall-fp64-u32-convert-real.so#poly_entry`, and
`riscv-pcall-fp64-u32-convert-real.so#poly_entry`) plus scalar float/double
conversion objects (`aarch64-pcall-fp-mixed-convert-real.so#poly_entry` and
`riscv-pcall-fp-mixed-convert-real.so#poly_entry`), compiler-built scalar float FP
objects (`aarch64-pcall-fp32-real.so#poly_entry` and
`riscv-pcall-fp32-real.so#poly_entry`), compiler-built scalar float FP
absolute-value objects (`aarch64-pcall-fp32-abs-real.so#poly_entry` and
`riscv-pcall-fp32-abs-real.so#poly_entry`), compiler-built scalar float FP
square-root objects (`aarch64-pcall-fp32-sqrt-real.so#poly_entry` and
`riscv-pcall-fp32-sqrt-real.so#poly_entry`), compiler-built scalar float FP
fused multiply-add objects (`aarch64-pcall-fp32-fma-real.so#poly_entry` and
`riscv-pcall-fp32-fma-real.so#poly_entry`) plus fused multiply-add variant
objects (`aarch64-pcall-fp32-fma-variants-real.so#poly_entry` and
`riscv-pcall-fp32-fma-variants-real.so#poly_entry`), compiler-built scalar float
FP min/max objects (`aarch64-pcall-fp32-minmax-real.so#poly_entry` and
`riscv-pcall-fp32-minmax-real.so#poly_entry`), compiler-built scalar float FP
select objects (`aarch64-pcall-fp32-select-real.so#poly_entry` and
`riscv-pcall-fp32-select-real.so#poly_entry`), compiler-built scalar float FP
memory objects (`aarch64-pcall-fp32-mem-real.so#poly_entry` and
`riscv-pcall-fp32-mem-real.so#poly_entry`), and compiler-shaped stack-frame payloads
(`aarch64-pcall-frame.elf`, `aarch64-pcall-native-frame.elf`,
`aarch64-pcall-bl.elf`, `aarch64-pcall-adrp.elf`, `aarch64-pcall-cond.elf`,
`aarch64-pcall-split-load.elf`, `aarch64-pcall-dynrel.elf`,
`aarch64-pcall-rel.elf`, `aarch64-pcall-relr.elf`,
`aarch64-pcall-relr-bitmap.elf`,
`aarch64-pcall-irelative.elf`, `aarch64-pcall-dynsym.elf`,
`aarch64-pcall-dyntab.elf`,
`aarch64-pcall-dyntab-entry.elf`, `riscv-pcall-frame.elf`,
`riscv-pcall-split-load.elf`, `riscv-pcall-dynrel.elf`,
`riscv-pcall-rel.elf`, `riscv-pcall-relr.elf`,
`riscv-pcall-relr-bitmap.elf`,
`riscv-pcall-irelative.elf`, `riscv-pcall-dynsym.elf`,
`riscv-pcall-dyntab.elf`, and
`riscv-pcall-dyntab-entry.elf`) that use native
`SP` adjustment, stack load/store, pair frame save/restore, local AArch64 `bl`
calls, AArch64 `adrp` page-relative data addressing, NZCV-backed AArch64
conditional branches, RISC-V `auipc` page-relative data addressing, real
AArch64 and RISC-V `ET_DYN` `.so` objects with dynamic symbol lookup, writable
static data, split ELF `PT_LOAD` text/data layout, simple `ET_DYN` relative
relocations through both `RELA` addends and `REL` in-place addends, packed
`DT_RELR` direct and bitmap relative relocation tables,
`R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolver relocations, same-image
symbolic 64-bit dynamic relocations and exported entrypoints through both
section-backed and `PT_DYNAMIC` symbol metadata, including SysV `DT_HASH` and
GNU `DT_GNU_HASH` symbol counts plus `DT_JMPREL`/`JUMP_SLOT` PLT relocations
using both `DT_PLTREL=RELA` and `DT_PLTREL=REL` tables for sectionless dynamic
objects, scalar double FP arguments and returns through
the native FP register ABI, scalar double FP function imports through PLT/GOT call
descriptors, real compiler-emitted GOT loads for undefined object-symbol
imports, weak undefined function/object relocations resolving to zero,
same-directory `DT_NEEDED` foreign shared-library dependencies with direct
binding of undefined function and object-symbol relocations to dependency
text/data, weak undefined relocations binding to dependency exports when
present, direct-dependency symbol interposition ahead of transitive dependencies
when symbols collide, recursive loading of dependency libraries' own `DT_NEEDED`
entries, and dependency sets larger than the original four-library prototype
limit. Foreign `DT_RUNPATH`/`DT_RPATH` entries support `$ORIGIN/...`
dependency subdirectories,
plus dependency library dynamic relocations before those dependency calls
execute,
dependency `DT_INIT_ARRAY` constructor execution before entering dependent
foreign code and dependency `DT_FINI_ARRAY` teardown afterward with
destructor-visible dependency state checks,
compiler-emitted same-image function-pointer relocations and indirect native
calls, `DT_INIT_ARRAY` constructor execution before foreign entrypoints,
`DT_FINI_ARRAY` destructor execution during teardown,
compiler-emitted AArch64 TLSDESC and RISC-V `__tls_get_addr` access to
`PT_TLS` initial images through the `PCALL` TLS-base register, plus
initial-exec `R_AARCH64_TLS_TPREL64` and `R_RISCV_TLS_TPREL64` accesses,
compiler-emitted conditional select variants and branch patterns
(`aarch64-pcall-select-variants-real.so`,
`riscv-pcall-select-variants-real.so`, `aarch64-pcall-import.elf`,
`riscv-pcall-import.elf`), prototype imported
function call gates (`aarch64-pcall-import-func.elf`,
`aarch64-pcall-import-mul.elf`, `aarch64-pcall-import-x86.elf`,
`riscv-pcall-import-func.elf`, `riscv-pcall-import-cjalr.elf`,
`riscv-pcall-import-cjr.elf`, `riscv-pcall-import-mul.elf`,
`riscv-pcall-import-x86.elf`, `aarch64-pcall-import-x86-mul.elf`, and
`riscv-pcall-import-x86-mul.elf`) plus real compiler-emitted PLT/GOT calls to
foreign-to-x86 helper descriptors including eight-argument x86 SysV
stack-argument calls (`aarch64-pcall-x86-sum8-import-real.so#poly_entry` and
`riscv-pcall-x86-sum8-import-real.so#poly_entry`) plus non-tail
post-import continuations that fold in a ninth foreign stack argument
(`aarch64-pcall-x86-sum8-post-import-real.so#poly_entry` and
`riscv-pcall-x86-sum8-post-import-real.so#poly_entry`), and
`poly_import_add`, `strlen`, `strcmp`, `strncmp`, `memcpy`, `memmove`, `memset`,
`memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`, `strncpy`,
`strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`, `stpcpy`,
`stpncpy`, `mempcpy`, `memrchr`, `memmem`, `rawmemchr`, `strchrnul`, `bcmp`,
`bcopy`, `bzero`, `strcasecmp`, `strncasecmp`, `strcasestr`, `index`, `rindex`,
`__stack_chk_guard`, `__stack_chk_fail`, `__errno_location`, `getauxval`, `getpagesize`, `sysconf`, `getenv`, `secure_getenv`, `malloc`, `calloc`, `realloc`, `free`, `strdup`, `strndup`, `posix_memalign`, `aligned_alloc`, `memalign`, `atexit`, `__cxa_atexit`, `__cxa_finalize`, `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`, `getegid`, and `gettid`, and teardown
before returning.
The `poly_import_x86_add`, `poly_import_x86_mul`,
`poly_import_x86_sum6`, `poly_import_x86_sum8`,
`poly_import_x86_fp64_add`, `poly_import_x86_fp64_sum8`,
`poly_import_x86_mixed_u64_fp64`, and `poly_import_x86_fp32_add` descriptors enter
real x86_64 helpers through a runtime-supplied descriptor table, map the first
six native foreign integer arguments to x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`,
`R8`, and `R9`, place seventh and eighth integer arguments in the standard x86
stack-argument slots when needed, reuse the shared
`XMM0-XMM7`/`v0-v7`/`fa0-fa7` FP register aliases for scalar FP arguments and
returns, including an eight-double x86 SysV import that exercises every scalar
FP argument register and a mixed integer/double import that exercises the
independent SysV GPR and XMM argument counters, synthesize an x86 return address to the dedicated `0f 24` `PIRET`,
accept each helper's ordinary `ret`, and map the x86 `RAX` result back to
AArch64 `x0` or RISC-V `a0` for integer returns.  The current
`polycall` harness points these descriptors at `noinline` x86_64 C functions
linked from `tools/polycall_x86_helpers.c`, so the path exercises a separately
compiled x86 helper object with compiler-generated function bodies rather than
handwritten helper bytecode in the call trampoline. The eight-argument x86
helper also performs a nested compiled x86 helper call before returning, so the
descriptor path covers ordinary x86 call/return activity inside the imported
target.
The same descriptor mechanism currently resolves AArch64 TLSDESC and RISC-V
`__tls_get_addr` TLS accesses for self-contained foreign shared objects, and
common GCC AArch64 outline
atomic helper imports used by default compiler output:
8-, 16-, 32-, and 64-bit `ldadd`, `swp`, `ldclr`, `ldeor`, `ldset`, and `cas`
helpers with `relax`, `acq`, `rel`, and `acq_rel` suffixes.  The loader
aliases these suffixes to the same operation descriptor because the prototype
defines foreign atomic memory ordering in terms of the x86-TSO execution
model.
More complex ABI cases such as arbitrary external import target descriptors,
additional heterogeneous register aggregates, variadic calls, dynamic-loader TLS module
allocation across multiple DSOs, unwind, and exceptions still need full
descriptor-driven or software thunk support.  Direct register
aliases are an implementation optimization only where they match the native ABI
contract; they are not the external compatibility contract.

Cross-ISA returns are expected to use native return instructions.  AArch64
libraries return with `ret` through `x30`, and RISC-V libraries return with
`jalr x0, 0(ra)`.  For `PCALL`, the return cookie routes that native return
back to the saved x86_64 continuation without executing a foreign breakpoint or
custom escape instruction.
`polycall` accepts `foreign.elf#symbol` requests so tests can target a named
exported function instead of only the ELF entrypoint; the dynamic-relocation
probes export `poly_entry` away from offset zero to exercise that path.

Foreign traps are now recorded as explicit, operating-system-neutral
architectural exits before any compatibility behavior runs.  AArch64 `svc` and
RISC-V `ecall` record reason `1`; AArch64 `brk` and RISC-V `ebreak` record
reason `2`.  The record includes source mode, trap number, six ABI arguments,
the foreign PC, the resume PC, and the raw trap selector/immediate when the
foreign instruction encoding carries one.  In hardware or FPGA this packet is
the boundary:
firmware, the OS, or a userspace runtime routes it.  The current Bochs
dispatcher is only a compatibility service layered after the packet is captured.

When `cpu.poly_compat_traps=0`, x86 software can install an architectural trap
vector with `0f 24 60 ... POLY!` using `RAX=handler_pc`.  Foreign traps then
leave the raw frontend and enter x86 at that handler with `RAX=reason`,
`RBX=source mode`, `RCX=trap number`, `RDX=trap PC`, `RSI=selector`, and
`RDI=arg0`.  The handler can read the full packet with trap-status opcodes,
apply OS/runtime policy in software, place the result in the shared result
register, and execute `0f 24 62 ... POLY!` to resume the recorded source
frontend at the trap resume PC.

## Supported Foreign Subset

The direct-fetch AArch64 path covers the generated/probed subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `adr`, `adrp`, `movz`, `movn`,
`movk`, `add`/`sub` immediate forms including `SP`, flag-setting `adds`/`subs`
immediate and shifted-register forms, shifted-register
`add`/`sub`/`adc`/`sbc`/`mul`/`madd`/`msub`/`umulh`/`smulh`/`udiv`/`sdiv`/`and`/`bic`/`orr`/`orn`/`eor`/`eon`/`ands`/`bics`, variable register shifts
`lsl`/`lsr`/`asr`/`ror`, extended-register `add`/`sub` forms,
unsigned bitfield aliases `uxtb`/`uxth`/`lsl`/`lsr`/`ubfx`,
signed bitfield aliases `sxtb`/`sxth`/`sxtw`/`asr`/`sbfx`,
bitfield move aliases `bfxil`/`bfi`,
one-source bit operations `rbit`/`rev16`/`rev32`/`rev`/`clz`/`cls`,
extract/rotate `extr` and logical shifted-register `ror` operands,
scalar AdvSIMD `ushr d`,
unconditional branch and call `b`/`bl`,
condition-code branch `b.cond`, register branch and call `br`/`blr`,
`cbz`/`cbnz`, bit-test branch `tbz`/`tbnz`,
conditional compare `ccmp`/`ccmn`,
conditional select `csel`/`csinc`/`csinv`/`csneg`,
logical-immediate `and`/`orr`/`eor`
and `tst`/`ands`, native `ret`, `dmb`/`dsb`/`isb`, scalar double
and float `fadd`/`fsub`/`fmul`/`fdiv`, fused multiply-add `fmadd`,
`fmsub`, `fnmadd`, and `fnmsub`, register `fmov`, unary
`fneg`/`fabs`/`fsqrt`, normal-number min/max `fminnm`/`fmaxnm`,
`fcmp`/`fcmpe` including zero-immediate compare, scalar FP `fcsel`,
32-bit and 64-bit signed and
unsigned FP-to-integer conversion for finite scalar float and double values,
scalar float/double `fcvt` narrowing and widening, signed and unsigned
integer-to-FP `scvtf`/`ucvtf` conversions from GPR and scalar FP/SIMD integer
sources, scalar FP/GPR bit moves, scalar FP pair loads/stores and scalar FP `ldr`/`str`,
generic byte/halfword/word/dword load-store forms plus scalar FP load-store
forms including register-offset indexed addressing, 64-bit
`stp`/`ldp` pair load-store forms, unscaled `ldur`/`stur` integer and scalar FP
forms plus single-register pre/post-indexed load-store writeback forms,
exclusive atomics `ldxr`/`ldaxr`/`stxr`/`stlxr` and `clrex`, LSE atomic
read-modify-write forms `swp`, `ldadd`, `ldclr`, `ldeor`, `ldset`,
`ldsmin`, `ldsmax`, `ldumin`, `ldumax`, and `cas` for decoded 8-, 16-, 32-,
and 64-bit compiler-emitted encodings, `svc`, and `brk`.

The direct-fetch RISC-V path covers the generated/probed RV64 subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `lui`, `auipc`, OP-IMM
`addi`, `xori`, `ori`, `andi`, `slli`, `srli`, and `srai`, RV64 word
arithmetic, compare/register-shift and division/remainder forms,
register-register `add`, `sub`, `mul`, `mulh`, `mulhsu`, `mulhu`, `xor`, `and`, and `or`,
`beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu`, `jal`, generic `jalr`, selected
byte/halfword/word/dword load-store forms, A-extension `lr`/`sc`, AMO
word/dword forms including signed/unsigned min/max, and compiler-emitted
byte/halfword LR.W/SC.W atomic loops including subtract and NAND,
`fence`, `fence.i`, `ecall`,
`ebreak`, custom-0 escape, and scalar double `fadd.d`/`fsub.d`/`fmul.d` over
`fa0`-`fa7`, plus scalar float `fadd.s`/`fsub.s`/`fmul.s` and scalar FP
division, square root, and fused multiply-add
`fdiv.s`/`fdiv.d`/`fsqrt.s`/`fsqrt.d`/`fmadd.s`/`fmadd.d`,
`fmsub.s`/`fmsub.d`, `fnmsub.s`/`fnmsub.d`, and
`fnmadd.s`/`fnmadd.d` on the same mapped
FP argument registers, FP min/max `fmin.s`/`fmin.d`/`fmax.s`/`fmax.d`,
FP compare `feq`/`flt`/`fle`, FP sign-injection
`fsgnj`/`fsgnjn`/`fsgnjx` including `fneg`/`fabs`/`fmv`, integer-to-FP bit moves
`fmv.w.x`/`fmv.d.x`, FP-to-integer bit moves `fmv.x.w`/`fmv.x.d`,
FP class tests `fclass.s`/`fclass.d`, signed and unsigned integer-to-FP
`fcvt.s/d.{w,wu,l,lu}`, signed and unsigned FP-to-integer conversions from
scalar float/double, scalar float/double `fcvt.s.d`/`fcvt.d.s`, and FP
`flw`/`fld`/`fsw`/`fsd` memory forms.  It also
decodes a first RV64C compatibility subset for common
compressed integer code: `c.addi4spn`, `c.ld`, `c.sd`, `c.addi`, `c.li`,
`c.lui`, `c.addi16sp`, `c.j`, `c.beqz`, `c.bnez`, `c.slli`, `c.lw`/`c.sw`,
`c.ldsp`/`c.lwsp`, `c.addiw`, `c.srli`/`c.srai`, `c.andi`,
`c.sub`/`c.xor`/`c.or`/`c.and`, `c.subw`/`c.addw`, `c.mv`, `c.jr`/`c.ret`,
`c.ebreak`, `c.jalr`, `c.add`, `c.sdsp`/`c.swsp`, and compressed double-FP
memory forms `c.fld`/`c.fsd` plus `c.fldsp`/`c.fsdsp`.
`polybench` also validates efficient neutral mixed-raw paths: direct
AArch64-to-RISC-V and RISC-V-to-AArch64 frontend switches, plus cross-ISA calls
where the caller enters the other foreign frontend and the callee returns with
ordinary native `ret`/`jalr` through a hardware cookie without routing through
x86.  The gate also covers scalar double FP cross-calls in both directions,
eight-register double FP argument pressure across `d0`-`d7`/`fa0`-`fa7`,
FP64 overflow stack-argument cross-calls that sum sixteen double arguments in
both directions, and mixed integer/FP cross-calls where an integer argument is
converted and folded into the FP return value across the opposite foreign frontend.  Shared-stack
neutral cross-call probes verify an AArch64 or RISC-V caller can allocate an
aligned stack slot, the opposite frontend can load it through its native `sp`,
and the caller can restore the same user stack before returning to x86.
Callee-saved probes verify AArch64 `x19` and RISC-V `s0` remain live across a
neutral call into the opposite frontend.  Pair-return probes verify `x0`/`x1`
and `a0`/`a1` both map back to the caller so ordinary two-word integer returns
survive a neutral cross-call.  Syscall probes verify a callee can execute a
native `svc` or `ecall`, return through the neutral hardware cookie, and leave
the syscall result mapped into the caller's result register.  Libcall probes
verify a callee can execute a native AArch64 `brk #1` or RISC-V `ebreak`
strlen trap inside a neutral cross-call while preserving the x86_64 first
argument pointer and returning the result through the caller's native result
register.  Descriptor-import probes also verify a neutral callee can call
hardware-style `strlen`, `strnlen`, `memset`, `memcpy`, and three-argument
`memcmp` import descriptors with ordinary AArch64 `blr` or RISC-V `jalr`,
restore its native link register, mutate a shared x86 buffer through the
foreign memory path, and return through the same cross-frontend hardware cookie.  The gate also covers
a nested AArch64 -> RISC-V -> AArch64 call chain.
Synthetic AArch64/RISC-V register banks, the current poly mode, and hidden
hardware-style continuation state for `PCALL`, x86 import returns, and neutral
foreign cross-calls are lazily saved and restored per guest `CR3`, user
`FSBASE`, and an 8 MiB-aligned user stack-region key in the Bochs prototype.
A normal x86_64 Linux process switch does not share foreign registers or
continuation cookies with another address space, and common pthread stacks get
separate synthetic banks even when static TLS does not give each guest thread a
distinct `FSBASE`. The low overlapping return/scratch values still use the
current x86 register bridge; this is not yet a full XSAVE-backed foreign
register ABI. The current interrupt prototype covers ordinary long-mode
`IRET64`, `SYSRET`, `SYSEXIT`, and Linux signal-return paths into raw
userspace; the final ISA still needs an explicit, architectural XSAVE-visible
foreign state component.

After the OS-neutral trap packet is recorded, the Bochs prototype can run a
test-only compatibility service for selected foreign Linux syscall numbers.
This service is controlled by `cpu.poly_compat_traps`/`POLY_COMPAT_TRAPS` and
defaults on for the existing regression suite.  When disabled, the prototype
records the packet, leaves raw mode, and either enters the configured
architectural trap vector or raises an x86 `#UD` if no vector is installed.
The service is not part of the CPU contract; it stands in for firmware, kernel,
loader, or userspace-runtime routing that a real implementation would provide.
The current service recognizes:

- Scalar/process syscalls: `fcntl`, `ioctl`, `faccessat`, `set_tid_address`,
  `futex`, `set_robust_list`, `get_robust_list`, `kill`, `tkill`, `tgkill`, `sigaltstack`,
  `rt_sigaction`, `rt_sigprocmask`, `capget`,
  `capset`, `personality`, `waitid`, `wait4`, `setpriority`, `getpriority`, `setpgid`,
  `setsid`, `umask`, `prctl`, `setregid`, `setgid`, `setreuid`, `setuid`,
  `setresuid`, `getresuid`, `setresgid`, `getresgid`, `setfsuid`,
  `setfsgid`, `getgroups`, `setgroups`, `getpid`, `getppid`, `getuid`,
  `geteuid`, `getgid`, `getegid`, `gettid`, `getpgid(0)`, `getsid(0)`,
  `rseq`, `exit`, and `exit_group`.
- File-style syscalls: `getcwd`, `eventfd2`, `inotify_init1`,
  `inotify_add_watch`, `inotify_rm_watch`, `dup3`, `pipe2`,
  `timer_create`, `timer_gettime`, `timer_getoverrun`, `timer_settime`,
  `timer_delete`, `timerfd_create`, `timerfd_settime`, `timerfd_gettime`, `read`, `readv`,
  `write`, `writev`, `pread64`, `pwrite64`, `preadv`, `pwritev`, `fsync`,
  `fdatasync`, `sync_file_range`, `fadvise64`, `statfs`, `fstatfs`,
  `truncate`, `ftruncate`, `fallocate`, `chdir`, `fchdir`, `fchmod`,
  `fchmodat`, `fchownat`, `fchown`, `setxattr`, `lsetxattr`, `fsetxattr`,
  `getxattr`, `lgetxattr`, `fgetxattr`, `listxattr`, `llistxattr`,
  `flistxattr`, `removexattr`, `lremovexattr`, `fremovexattr`,
  `ioprio_set`, `ioprio_get`, `flock`, `mknodat`, `mkdirat`, `unlinkat`,
  `symlinkat`, `linkat`, `renameat`, `renameat2`, `chroot`, `umount2`,
  `mount`, `pivot_root`, `open_tree`, `move_mount`, `fsopen`, `fsconfig`,
  `fsmount`, `fspick`, `mount_setattr`, `pselect6`, `ppoll`,
  `epoll_create1`, `epoll_ctl`, `epoll_pwait`, `openat`, `readlinkat`,
  `newfstatat`, `fstat`, `statx`, `close`, `getdents64`, `lseek`, and
  `uname`.
- Network-style syscalls: `socket`, `socketpair`, `bind`, `listen`,
  `accept`, `connect`, `getsockname`, `getpeername`, `sendto`, `recvfrom`,
  `setsockopt`, `getsockopt`, `shutdown`, and `accept4`.
- Memory/time-style syscalls: `nanosleep`, `getitimer`, `setitimer`,
  `clock_gettime`, `clock_getres`, `clock_nanosleep`, `times`,
  `sched_setparam`, `sched_setscheduler`,
  `sched_getscheduler`, `sched_getparam`, `sched_setaffinity`,
  `sched_getaffinity`, `sched_yield`, `sched_get_priority_max`,
  `sched_get_priority_min`, `getrusage`,
  `getrlimit`, `setrlimit`, `getcpu`, `gettimeofday`, `sysinfo`,
  `prlimit64`, `getrandom`, `brk`,
  `munmap`, `mremap`, `mprotect`, `madvise`, `mlock`, `munlock`,
  `mlockall`, `munlockall`, `mlock2`, `close_range`, `membarrier`, and
  `mmap`.

The compatibility dispatcher also has deterministic unavailable probes that
return Linux `-ENOSYS` for syscalls commonly probed by runtimes and libraries:
`clone`, `execve`, `get_mempolicy`, `set_mempolicy`, `migrate_pages`,
`move_pages`, `seccomp`, `bpf`, `userfaultfd`, `pkey_mprotect`, `pkey_alloc`, `pkey_free`,
`pidfd_send_signal`, `io_uring_setup`, `io_uring_enter`,
`io_uring_register`, `pidfd_open`, `clone3`, `openat2`, `pidfd_getfd`,
`process_madvise`, `landlock_create_ruleset`, `landlock_add_rule`,
`landlock_restrict_self`, `process_mrelease`, `futex_waitv`, and
`set_mempolicy_home_node`.

The compatibility syscall service carries six foreign Linux ABI arguments for
both foreign architectures; current `mmap6` payloads verify argument registers
beyond `arg2` reach the service after the architectural trap packet is recorded.

The Bochs compatibility runtime also handles selected breakpoint traps as
deterministic scaffold library calls after recording an OS-neutral break-trap
packet.  This is not the hardware ISA contract; real precompiled-code interop
should use ordinary dynamic-linker bindings, hardware-assisted `PCALL`
descriptors, software thunks, or OS/runtime trap routing.

- AArch64 uses `brk #id`.
- RISC-V uses `a7=id; ebreak`.
- Supported ids are `1=strlen`, `2=memfill`, `3=memcmp`, and `4=memcpy`.
- `RDI`/AArch64 `x1`/RISC-V `a1` is the implicit left/destination pointer;
  explicit libcall operands use `RSI,RDX` via AArch64 `x2,x3` or RISC-V
  `a2,a3`.

## Validation Gates

Build the Docker image:

```bash
make image
```

Run the baseline x86_64 Linux boot:

```bash
make boot
```

Run the poly probe and manifest-backed generated foreign payloads:

```bash
make boot-poly
```

Run the OS-neutral architectural trap-vector gate with Bochs syscall/libcall
compatibility disabled:

```bash
make boot-poly-arch-traps
```

Run the fuller gate, including direct foreign ELF execution and guest
`binfmt_misc` registration:

```bash
make boot-poly-full
```

Expected success markers include:

- `BOOT_OK`
- `NATIVE_POLY_TRAP_VECTOR_OK`
- `POLY_PROBE_OK`
- `POLYAPP_OK`
- `POLYEXEC_OK`
- `POLYCALL_OK`
- `POLYTHREAD_OK`
- `POLYBENCH_OK`
- `POLYBINFMT_OK`

## Known Gaps

- Foreign execution is still a Bochs prototype, not full native hardware
  decode.  The current hot path has a CPUID-gated `0f 24 ... POLY!`
  opcode-family placeholder decoded through `BX_IA_POLYMODE`.
- AArch64 and RISC-V ISA support is limited to the tested generated subset.
- Syscall and breakpoint traps are recorded explicitly as OS-neutral packets,
  but the optional compatibility runtime still returns deterministic scaffold
  results rather than complete host or guest Linux ABI behavior.
- Equal-speed or minimal-slowdown execution is a design target.  The current
  implementation demonstrates raw direct-fetch execution and multi-burst raw
  loops, but not full equal-speed execution across complete ISAs.
- Foreign ELF support is limited to the generated static payload shape used by
  `tools/mkpolyelf.c`, `tools/polyapp.c`, and `tools/polyexec.c`, but both
  `polyapp` and `polyexec` load variable-size executable segments up to 1 MiB.
  `polyexec` preserves raw executable bytes, accepting 4-byte-aligned AArch64
  and 2-byte-aligned RISC-V entry segments for compressed-code compatibility;
  `riscv-compressed-half.elf` and `riscv-compressed-jalr.elf` verify RISC-V
  entry sizes that are not multiples of 4 bytes, including both compressed
  `c.jr` and 32-bit `jalr` returns to a halfword-aligned escape.
