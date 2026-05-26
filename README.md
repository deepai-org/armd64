# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension.  The extension
keeps standard x86_64 execution as the host ISA, then adds synthetic userspace
mode-switch envelopes that let selected AArch64 and RISC-V instruction streams
run through direct foreign fetch inside the x86_64 process.

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
- With `POLY_ENABLED=1`, Bochs handles the polyglot userspace mode/status
  envelopes and raw foreign fetch in `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`.
- `tools/polyprobe.c` validates raw AArch64 and RISC-V fetch/decode, wide
  register state, native returns, mixed raw instruction streams, repeated
  mixed-mode switch stress, mixed libcalls, mixed syscalls, and status/counter
  markers.
- `tools/polyapp.c` runs manifest-backed generated foreign ELF64 payloads from
  `tools/polyapps/*.poly` by entering raw foreign mode, executing packed
  32-bit foreign instructions, and escaping back to x86_64.  The manifest path
  accepts variable-size executable segments up to 1 MiB.
- `tools/polyexec.c` runs generated foreign ELF64 payloads directly by path
  using the same raw-mode execution path, including executable segments larger
  than one raw burst.
- `tools/polycall.c` loads generated foreign ELF64 function payloads and calls
  their entrypoints through the prototype hardware ABI bridge (`PCALL`), so
  the return path uses ordinary AArch64/RISC-V return instructions rather than
  raw escape instructions. It also resolves prototype foreign `JUMP_SLOT`
  function imports to hardware call-descriptor slots.
- `tools/polythread.c` runs real x86_64 pthreads that repeatedly enter long
  AArch64 and RISC-V `PCALL` loops, exercising guest thread-bank isolation and
  interrupt/`IRET64` raw-mode restoration across many foreign transitions.
- `tools/polysignal.c` arms real guest `SIGALRM` delivery while executing long
  raw AArch64 and RISC-V `PCALL` loops, checking that the x86_64 signal handler
  and `rt_sigreturn` path resume the interrupted foreign frontend correctly.
- `tools/polybench.c` executes long raw AArch64 and RISC-V loops inside the
  guest, verifies that raw instruction counters advance across multiple
  fetch/decode bursts, and checks mixed raw AArch64-to-RISC-V and
  RISC-V-to-AArch64 code blobs that switch, call, nest calls, and carry scalar
  double, mixed integer/FP, two-register integer returns, shared-stack values,
  caller callee-saved registers, and syscall trap results directly without
  returning to x86.
- `tools/polybinfmt.sh` can register guest `binfmt_misc` entries so generated
  AArch64 and RISC-V ELF64 payloads execute directly from the x86_64 guest.
- `docs/poly-isa.md` defines the silicon-oriented ISA contract: dedicated
  frontend-switch opcodes, XSAVE-visible foreign state, explicit trap exits,
  and native-ABI thunking for precompiled cross-ISA libraries.

## ISA Changes From Standard x86_64

The Bochs fork treats selected userspace `#UD` byte sequences as polyglot CPU
operations when `POLY_ENABLED=1`.  Normal x86_64 instructions are unchanged.
The extension is intentionally encoded as invalid x86 byte sequences so an
unmodified CPU still traps.  The current handler accepts these envelopes only
from guest userspace.

All x86-visible poly operations are wrapped in fixed 8-byte envelopes:

| Operation | Bytes | Effect |
| --- | --- | --- |
| Switch to x86_64 mode | `64 0f 0b 58 4d 4f 44 45` | Sets current poly mode to x86_64. |
| Switch to raw AArch64 mode | `65 0f 0b 52 41 57 36 34` | Sets current poly mode to raw AArch64; following bytes are fetched as fixed 32-bit AArch64 instructions. |
| Switch to raw RISC-V mode | `66 0f 0b 52 41 57 52 56` | Sets current poly mode to raw RISC-V; following bytes are fetched as mixed 16/32-bit RISC-V instructions. |
| x86 SysV call to AArch64 | `40 0f 0b 50 43 41 36 34` | Prototype `PCALL.A64.SYSV`: `R10=foreign target`, `R11=x86 return`; maps x86_64 SysV integer args to AAPCS64 and enters raw AArch64. |
| x86 SysV call to RISC-V | `40 0f 0b 50 43 52 56 36` | Prototype `PCALL.RV64.SYSV`: `R10=foreign target`, `R11=x86 return`; maps x86_64 SysV integer args to RISC-V psABI and enters raw RISC-V. |
| x86 SysV sret call to AArch64 | `42 0f 0b 50 53 41 36 34` | Prototype `PCALL.A64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to AAPCS64 `x8`, shifts user args back to `x0`-`x7`, and enters raw AArch64. |
| x86 SysV sret call to RISC-V | `42 0f 0b 50 53 52 56 36` | Prototype `PCALL.RV64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to RISC-V `a0`, shifts user args to `a1`-`a7`, and enters raw RISC-V. |
| x86 import return | `41 0f 0b 50 49 52 45 54` | Prototype `PIRET`: resumes the saved foreign return PC after an x86 helper returns normally from a descriptor-driven import call. |
| Syscall status | `2e 0f 0b 53 59 53 43 <id>` | Returns syscall state in `RAX`: `0=current mode`, `1=last foreign syscall number`, `2=last foreign syscall mode`. |
| Libcall status | `3e 0f 0b 4c 49 42 43 <id>` | Returns libcall state in `RAX`: `0=current libcall status`, `1=last libcall number`, `2=last libcall mode`. |
| Switch/status counters | `4e 0f 0b 53 57 43 48 <id>` | Returns mode/counter state in `RAX`: `0=switches`, `1=current mode`, `2=foreign raw instructions`, `3=foreign syscalls`, `4=foreign libcalls`. |
| Trap status | `36 0f 0b 54 52 41 50 <id>` | Returns last foreign trap state in `RAX`: `0=reason`, `1=source mode`, `2=number`, `3`-`8=arg0`-`arg5`, other ids return trap PC. |

When `POLY_ENABLED=1`, the prototype exposes a private CPUID discovery leaf for
runtime dispatch:

| Leaf | Registers | Meaning |
| --- | --- | --- |
| `0x40000000` | `EAX=0x40000002`, `EBX:EDX:ECX="PolyglotCPU!"` | Advertises the maximum poly CPUID leaf and the 12-byte poly vendor string. |
| `0x40000001` | `EAX=1`, `EBX=mode mask`, `ECX=feature mask`, `EDX=0` | Reports poly CPUID ABI version 1, supported frontend modes, implemented prototype features, and no architectural XSAVE component yet. |
| `0x40000002, subleaf 0` | `EAX[15:0]=0x7fff`, `EAX[31:16]=0x7ffe`, `EBX=0x7ffd`, `ECX=0x0000000b`, `EDX=0x0000002b` | Reports native raw-mode escape/cross-switch encodings: AArch64-to-x86, AArch64-to-RISC-V switch, AArch64-to-RISC-V call, RISC-V-to-x86, and RISC-V-to-AArch64 switch. |
| `0x40000002, subleaf 1` | `EAX=0x0000005b`, `EBX=ECX=EDX=0` | Reports the RISC-V-to-AArch64 native cross-call encoding. |

The current `0x40000001.EBX` mode mask sets bits `0`, `3`, and `4` for x86_64,
raw AArch64, and raw RISC-V.  `0x40000001.ECX` sets bits for raw AArch64, raw
RISC-V, neutral direct switches, native return cookies, x86 SysV `PCALL`,
`PCALL` sret, scalar FP bridging, trap records, user return restoration, x86 TSO
foreign ordering, per-thread synthetic banks, and deterministic compatibility
syscall/libcall traps.

Foreign execution always uses raw direct fetch.  Bochs enters raw mode through
the x86_64 switch envelope, bypasses x86 decode, and fetches foreign
instructions directly from `RIP`: fixed 32-bit instructions for AArch64 and
mixed 16/32-bit instructions for RISC-V.  AArch64 `brk #0x7fff` and RISC-V
custom-0 instruction `0x0000000b` escape back to x86_64 at the next byte.
AArch64 `brk #0x7ffe` switches directly to raw RISC-V, and RISC-V custom-1
instruction `0x0000002b` switches directly to raw AArch64, preserving the
shared low integer result/argument register state instead of routing through
x86.  AArch64 `brk #0x7ffd` is a prototype neutral call gate to a RISC-V target
address in `x16` with an AArch64 return PC in `x17`; RISC-V custom-2
`0x0000005b` is the reverse call gate to an AArch64 target in `x5` with a
RISC-V return PC in `x6`.  The callee returns with its ordinary native return
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
foreign loads and stores.  The current prototype covers the compiler-emitted
AArch64 exclusive and LSE atomic subset used by the test payloads plus the
RISC-V A-extension subset.

The current register bridge aliases the overlapping caller-visible integer ABI:

- x86_64 `RAX` carries the foreign return value and maps to AArch64 `x0` or
  RISC-V `a0`.
- x86_64 `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to AArch64 `x1`-`x6`
  and RISC-V `a1`-`a6`.
- x86_64 `RSP` maps to RISC-V `sp`; AArch64 `x31` is still decoded as zero for
  general register operands, with load/store base handling treating `x31` as
  `SP`.
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
foreign `SP` window below the x86 frame, and copies stack arguments from
`[RSP+24]` onward so the first foreign stack argument is visible at `[sp]`; x86
`RSP` is restored when the native foreign return hits the return cookie.  The
bridge preserves the shared `XMM0`-`XMM7` FP argument/return aliases, including
two-register homogeneous double aggregate arguments and returns through
`XMM0`/`XMM1`, including mixed integer/FP signatures where GPR and XMM lanes
are consumed in one native call, sets `x30` or `ra` to a return cookie, enters raw fetch at the
`R10` target, maps AArch64 `x0` or RISC-V `a0` back to x86 `RAX`, and maps
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
`aarch64-pcall-gnu-hash-real.so#poly_entry`,
`riscv-pcall-gnu-hash-real.so#poly_entry`,
`aarch64-pcall-state.so#poly_entry`, and
`riscv-pcall-state.so#poly_entry`) plus compiler-built imported-function
objects (`aarch64-pcall-import-real.so#poly_entry` and
`riscv-pcall-import-real.so#poly_entry`), compiler-built libc-style imported
function objects (`aarch64-pcall-libc-import-real.so#poly_entry` and
`riscv-pcall-libc-import-real.so#poly_entry`) that call `strlen`, `memcpy`,
`memset`, and `memcmp` through PLT/GOT descriptors, compiler-built imported-object
objects (`aarch64-pcall-import-value-real.so#poly_entry` and
`riscv-pcall-import-value-real.so#poly_entry`), compiler-built weak undefined
import objects (`aarch64-pcall-weak-import-real.so#poly_entry` and
`riscv-pcall-weak-import-real.so#poly_entry`), compiler-built relocated
function-pointer objects (`aarch64-pcall-funcptr-real.so#poly_entry` and
`riscv-pcall-funcptr-real.so#poly_entry`), compiler-built two-word aggregate
return objects (`aarch64-pcall-pair-real.so#poly_entry` and
`riscv-pcall-pair-real.so#poly_entry`), compiler-built hidden-sret aggregate
return objects (`aarch64-pcall-sret-real.so#poly_entry` and
`riscv-pcall-sret-real.so#poly_entry`), compiler-built constructor objects
(`aarch64-pcall-ctor-real.so#poly_entry` and
`riscv-pcall-ctor-real.so#poly_entry`), compiler-built conditional objects
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
`riscv-pcall-int-highmul-real.so#poly_entry`), compiler-built integer carry-chain
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
covering exclusive LL/SC, default GCC outline helpers, and LSE instructions
(`aarch64-pcall-atomic.so#poly_entry`,
`aarch64-pcall-atomic-outline.so#poly_entry`, and
`aarch64-pcall-atomic-lse.so#poly_entry`), RISC-V atomic object
(`riscv-pcall-atomic.so#poly_entry`), compiler-built unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-built indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-built callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), compiler-built scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`), compiler-built homogeneous double
aggregate return objects (`aarch64-pcall-fpair-real.so#poly_entry` and
`riscv-pcall-fpair-real.so#poly_entry`), compiler-built homogeneous double
aggregate argument objects (`aarch64-pcall-fpair-arg-real.so#poly_entry` and
`riscv-pcall-fpair-arg-real.so#poly_entry`), compiler-built mixed integer/FP
argument objects (`aarch64-pcall-mixed-args-real.so#poly_entry` and
`riscv-pcall-mixed-args-real.so#poly_entry`), compiler-built scalar double FP
import objects (`aarch64-pcall-fp64-import-real.so#poly_entry` and
`riscv-pcall-fp64-import-real.so#poly_entry`), compiler-built scalar double FP
callee-saved objects (`aarch64-pcall-fp64-callee-real.so#poly_entry` and
`riscv-pcall-fp64-callee-real.so#poly_entry`), compiler-built scalar double FP
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
`aarch64-pcall-relr.elf`, `aarch64-pcall-relr-bitmap.elf`,
`aarch64-pcall-irelative.elf`, `aarch64-pcall-dynsym.elf`,
`aarch64-pcall-dyntab.elf`,
`aarch64-pcall-dyntab-entry.elf`, `riscv-pcall-frame.elf`,
`riscv-pcall-split-load.elf`, `riscv-pcall-dynrel.elf`,
`riscv-pcall-relr.elf`, `riscv-pcall-relr-bitmap.elf`,
`riscv-pcall-irelative.elf`, `riscv-pcall-dynsym.elf`,
`riscv-pcall-dyntab.elf`, and
`riscv-pcall-dyntab-entry.elf`) that use native
`SP` adjustment, stack load/store, pair frame save/restore, local AArch64 `bl`
calls, AArch64 `adrp` page-relative data addressing, NZCV-backed AArch64
conditional branches, RISC-V `auipc` page-relative data addressing, real
AArch64 and RISC-V `ET_DYN` `.so` objects with dynamic symbol lookup, writable
static data, split ELF `PT_LOAD` text/data layout, simple `ET_DYN` relative
relocations, packed `DT_RELR` direct and bitmap relative relocation tables,
`R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolver relocations, same-image
symbolic 64-bit dynamic relocations and exported entrypoints through both
section-backed and `PT_DYNAMIC` symbol metadata, including SysV `DT_HASH` and
GNU `DT_GNU_HASH` symbol counts plus `DT_JMPREL`/`JUMP_SLOT` PLT relocations
for sectionless dynamic objects, scalar double FP arguments and returns through
the native FP register ABI, scalar double FP function imports through PLT/GOT call
descriptors, real compiler-emitted GOT loads for undefined object-symbol
imports, weak undefined function/object relocations resolving to zero,
compiler-emitted same-image function-pointer relocations and indirect native
calls, `DT_INIT_ARRAY` constructor execution before foreign entrypoints,
compiler-emitted conditional select variants and branch patterns
(`aarch64-pcall-select-variants-real.so`,
`riscv-pcall-select-variants-real.so`, `aarch64-pcall-import.elf`,
`riscv-pcall-import.elf`), prototype imported
function call gates (`aarch64-pcall-import-func.elf`,
`aarch64-pcall-import-mul.elf`, `aarch64-pcall-import-x86.elf`,
`riscv-pcall-import-func.elf`, `riscv-pcall-import-mul.elf`,
`riscv-pcall-import-x86.elf`) plus real compiler-emitted PLT/GOT calls to
`poly_import_add`, `strlen`, `memcpy`, `memset`, and `memcmp`, and teardown
before returning.
The `poly_import_x86_add` descriptor enters a real x86_64 helper, synthesizes
an x86 return address to `PIRET`, accepts the helper's ordinary `ret`, and maps
the x86 `RAX` result back to AArch64 `x0` or RISC-V `a0`.
The same descriptor mechanism currently resolves common GCC AArch64 outline
atomic helper imports used by default compiler output:
`__aarch64_ldadd8_acq_rel`, `__aarch64_swp8_acq_rel`,
`__aarch64_ldset4_relax`, and `__aarch64_cas8_acq_rel`.
More complex ABI cases such as arbitrary external import target descriptors,
heterogeneous register aggregates, variadic calls, TLS, unwind, and exceptions
still need full descriptor-driven or software thunk support.  Direct register
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

Foreign traps are now recorded as explicit architectural exits before any
compatibility behavior runs.  AArch64 `svc` and RISC-V `ecall` record reason
`1`; AArch64 `brk` and RISC-V `ebreak` record reason `2`.  The record includes
source mode, trap number, six ABI arguments, and the foreign PC.  The current
Bochs dispatcher may still return deterministic scaffold values after recording
the trap, but the trap record is the intended ISA boundary.

## Supported Foreign Subset

The direct-fetch AArch64 path covers the generated/probed subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `adr`, `adrp`, `movz`, `movn`,
`movk`, `add`/`sub` immediate forms including `SP`, flag-setting `adds`/`subs`
immediate and shifted-register forms, shifted-register
`add`/`sub`/`adc`/`sbc`/`mul`/`madd`/`msub`/`umulh`/`smulh`/`udiv`/`sdiv`/`and`/`bic`/`orr`/`orn`/`eor`/`eon`/`ands`/`bics`, variable register shifts
`lsl`/`lsr`/`asr`/`ror`, extended-register `add`/`sub` forms,
unsigned bitfield aliases `uxtb`/`uxth`/`lsl`/`lsr`/`ubfx`,
signed bitfield aliases `sxtb`/`sxth`/`sxtw`/`asr`/`sbfx`,
one-source bit operations `rbit`/`rev16`/`rev32`/`rev`/`clz`/`cls`,
extract/rotate `extr` and logical shifted-register `ror` operands,
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
read-modify-write forms `swp`, `ldadd`, `ldset`, and `cas` for the decoded
32-bit and 64-bit compiler-emitted encodings, `svc`, and `brk`.

The direct-fetch RISC-V path covers the generated/probed RV64 subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `lui`, `auipc`, OP-IMM
`addi`, `xori`, `ori`, `andi`, `slli`, `srli`, and `srai`, RV64 word
arithmetic, compare/register-shift and division/remainder forms,
register-register `add`, `sub`, `mul`, `mulh`, `mulhsu`, `mulhu`, `xor`, `and`, and `or`,
`beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu`, `jal`, generic `jalr`, selected
byte/halfword/word/dword load-store forms, A-extension `lr`/`sc` and AMO
word/dword forms, `fence`, `fence.i`, `ecall`,
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
x86.  The gate also covers scalar double FP cross-calls in both directions and
mixed integer/FP cross-calls where an integer argument is converted and folded
into the FP return value across the opposite foreign frontend.  Shared-stack
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
hardware-style `strlen`, `memset`, `memcpy`, and three-argument `memcmp` import
descriptors with ordinary AArch64 `blr` or RISC-V `jalr`, restore its native
link register, mutate a shared x86 buffer through the foreign memory path, and
return through the same cross-frontend hardware cookie.  The gate also covers
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

The Bochs compatibility runtime handles selected foreign Linux syscall traps
deterministically after recording the architectural trap.  Supported syscall
numbers currently include:

- Scalar/process syscalls: `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`,
  `getegid`, `gettid`, `getpgid(0)`, `getsid(0)`, and `exit`.
- File-style syscalls: `getcwd`, `read`, `write`, `openat`, `close`, `lseek`,
  and `uname`.
- Memory/time-style syscalls: `clock_gettime`, `getrusage`, `getcpu`,
  `gettimeofday`, `sysinfo`, and `mmap`.

The shared syscall dispatcher carries six foreign Linux ABI arguments for both
foreign architectures; current `mmap6` payloads verify argument registers beyond
`arg2` reach the dispatcher.

The Bochs compatibility runtime also handles selected breakpoint traps as
deterministic scaffold library calls.  This is not the hardware ISA contract;
real precompiled-code interop should use software thunks or OS/runtime trap
routing.

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

Run the fuller gate, including direct foreign ELF execution and guest
`binfmt_misc` registration:

```bash
make boot-poly-full
```

Expected success markers include:

- `BOOT_OK`
- `POLY_PROBE_OK`
- `POLYAPP_OK`
- `POLYEXEC_OK`
- `POLYCALL_OK`
- `POLYTHREAD_OK`
- `POLYBENCH_OK`
- `POLYBINFMT_OK`

## Known Gaps

- Foreign execution is a Bochs UD-envelope prototype, not full native hardware
  decode.  The silicon contract in `docs/poly-isa.md` replaces these envelopes
  with CPUID-gated frontend-switch opcodes.
- AArch64 and RISC-V ISA support is limited to the tested generated subset.
- Syscall and breakpoint traps are recorded explicitly, but the current
  compatibility runtime still returns deterministic scaffold results rather
  than complete host or guest Linux ABI behavior.
- Equal-speed or minimal-slowdown execution is a design target.  The current
  implementation demonstrates raw direct-fetch execution and multi-burst raw
  loops, but not full equal-speed execution across complete ISAs.
- Foreign ELF support is limited to the generated static payload shape used by
  `tools/mkpolyelf.c`, `tools/polyapp.c`, and `tools/polyexec.c`, but both
  `polyapp` and `polyexec` load variable-size executable segments up to 1 MiB.
