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

Foreign traps are architectural exits, not hardware libcalls or Linux policy.
When foreign code executes AArch64 `svc`, RISC-V `ecall`, AArch64 `brk`, or
RISC-V `ebreak`, hardware records a trap frame, switches the frontend back to
x86_64, and transfers control to an OS/runtime-defined trap path.  The OS or
userspace runtime decides whether to translate a syscall, deliver a signal,
invoke a thunk, or reject the operation.

Foreign memory ordering is currently specified as x86_64 TSO for the prototype
compatibility target.  AArch64 and RISC-V barriers are legal decode points, and
foreign atomic read-modify-write instructions operate through the same coherent
virtual-memory path as ordinary x86_64 memory operations.

Interrupts, faults, and debug exceptions taken during foreign fetch must record
the interrupted foreign mode and PC before entering the x86_64 kernel path.
`IRET`, `SYSRET`, `SYSEXIT`, and signal return must restore the foreign
frontend mode when the saved architectural state requires it.

## Bochs Prototype Contract

Bochs now has a fixed 8-byte x86 opcode-family placeholder for hot frontend
and call operations:

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
- `0f 24 20 50 4f 4c 59 21`: prototype `PIRET`, used by
  descriptor-driven foreign-to-x86 import calls to resume the saved foreign
  return PC after an x86 helper returns normally.
- `0f 24 30+id 50 4f 4c 59 21`: syscall status read.  `id=0` returns the
  current mode, `id=1` returns the last foreign syscall number, and `id=2`
  returns the last foreign syscall mode.
- `0f 24 38+id 50 4f 4c 59 21`: libcall status read.  `id=1` returns the last
  libcall number and `id=2` returns the last libcall mode.
- `0f 24 40+id 50 4f 4c 59 21`: mode/counter status read.  `id=0` returns
  frontend switches, `id=1` returns the current mode, `id=2` returns raw foreign
  instructions, `id=3` returns foreign syscalls, and `id=4` returns foreign
  libcalls.
- `0f 24 50+id 50 4f 4c 59 21`: trap status read.  `id=0` returns the reason,
  `id=1` returns the source mode, `id=2` returns the trap number, `id=3`-`8`
  return trap arguments, and `id=9` returns the trap PC.

Bochs decodes the `0f 24` opcode slot through the prototype `BX_IA_POLYMODE`
handler and validates the trailing `POLY!` magic before changing frontend
state.  It is still a prototype allocation, but it no longer depends on the
generic invalid-opcode dispatch path or a `UD2` envelope.

`UD2` is not an alternate polyglot envelope in the current ISA contract;
ordinary `UD2` retains standard invalid-opcode behavior.

The prototype exposes private CPUID leaves when `poly_enabled=1` so runtimes can
discover the experimental hardware contract before emitting poly operations:

- `CPUID.EAX=0x40000000`: `EAX=0x40000002`, `EBX:EDX:ECX="PolyglotCPU!"`.
- `CPUID.EAX=0x40000001`: `EAX=1` for the poly CPUID ABI version.
- `0x40000001.EBX`: frontend mode mask.  Bits `0`, `3`, and `4` mean x86_64,
  raw AArch64, and raw RISC-V.
- `0x40000001.ECX`: feature mask.  Bits `0`-`14` mean raw AArch64, raw RISC-V,
  neutral direct switches, native return cookies, x86 SysV `PCALL`, `PCALL`
  sret, scalar FP bridging, trap records, user return restoration, x86 TSO
  foreign ordering, per-thread synthetic banks, and deterministic compatibility
  syscall/libcall traps, the prototype x86 poly opcode family, and two-float
  aggregate return packing and argument unpacking for native ABI `PCALL`.
- `0x40000001.EDX`: architectural XSAVE component id.  It is currently `0`
  because the Bochs prototype still uses synthetic banks rather than an
  OS-visible foreign XSAVE state component.
- `CPUID.EAX=0x40000002, ECX=0`: native escape encoding discovery.
  `EAX[15:0]=0x7fff` means AArch64 `brk #0x7fff` exits to x86_64;
  `EAX[31:16]=0x7ffe` means AArch64 `brk #0x7ffe` switches to RISC-V;
  `EBX=0x7ffd` means AArch64 `brk #0x7ffd` calls RISC-V;
  `ECX=0x0000000b` means RISC-V custom-0 exits to x86_64; and
  `EDX=0x0000002b` means RISC-V custom-1 switches to AArch64.
- `CPUID.EAX=0x40000002, ECX=1`: `EAX=0x0000005b` reports the RISC-V
  custom-2 AArch64 cross-call encoding.  Other registers are reserved zero.

Raw foreign modes also have native frontend-switch encodings so x86 is not the
only routing hub:

- AArch64 `brk #0x7fff`: exit raw AArch64 and resume x86_64 decode.
- AArch64 `brk #0x7ffe`: switch directly from raw AArch64 to raw RISC-V at
  the next byte.
- AArch64 `brk #0x7ffd`: call a raw RISC-V target held in `x16`, saving an
  AArch64 continuation from `x17`.
- RISC-V custom-0 `0x0000000b`: exit raw RISC-V and resume x86_64 decode.
- RISC-V custom-1 `0x0000002b`: switch directly from raw RISC-V to raw
  AArch64 at the next byte.
- RISC-V custom-2 `0x0000005b`: call a raw AArch64 target held in `x5`,
  saving a RISC-V continuation from `x6`.

These native switches preserve the shared low integer register aliases, so
`x0`/`a0`/`RAX` can carry a value through AArch64-to-RISC-V or
RISC-V-to-AArch64 code without an x86 trampoline.  The shared scalar FP aliases
similarly carry `d0`-`d7`/`fa0`-`fa7` through neutral cross-calls.
The native cross-call forms additionally set the callee's native link register
to a hardware return cookie, so AArch64 `ret` or RISC-V `jalr x0, 0(ra)`
restores the caller frontend mode and continuation without an x86 rendezvous.
The Bochs prototype backs this with a small bounded cross-return stack and
`polybench` covers scalar double FP cross-calls, mixed integer/FP cross-calls,
two-register integer returns, shared-stack cross-calls, caller callee-saved
register preservation, syscall trap routing inside neutral callees, and a
nested AArch64 -> RISC-V -> AArch64 call chain.  The stack probes use the
caller's real user `sp`: the caller allocates and restores an aligned slot,
while the opposite foreign frontend reads it through ordinary native stack
addressing.  The callee-saved probes keep AArch64 `x19` or RISC-V `s0` live
across a neutral call into the opposite frontend.  The pair-return probes
verify the second integer result lane maps across `x1`/`a1`.  The syscall
probes execute native AArch64 `svc` or RISC-V `ecall` inside the neutral
callee and return the deterministic syscall result through the caller's native
result register.  The libcall probes execute native AArch64 `brk #1` or
RISC-V `ebreak` strlen traps inside the neutral callee and return through the
same hardware cookie path.  Descriptor-import probes execute ordinary AArch64
`blr` or RISC-V `jalr` calls to `strlen`, `strnlen`, `memset`, `memcpy`, and
three-argument `memcmp` import descriptors inside the neutral callee, preserve
the callee's native link register, mutate a shared x86 buffer through the
foreign memory path, and return through the same cross-frontend cookie path.  The
prototype saves that stack, the `PCALL` return cookie, and x86-import return
state with the same synthetic bank as the non-aliased foreign registers.  The
bank key is guest `CR3`, user `FSBASE`, and an 8 MiB-aligned user stack-region
key, which keeps common pthread stacks isolated even when static TLS does not
give each guest thread a distinct `FSBASE`.  The `polythread` guest test
exercises this with real x86_64 pthreads repeatedly entering AArch64 and
RISC-V `PCALL` paths.  The `polysignal` guest test arms real `SIGALRM`
delivery during long raw AArch64 and RISC-V `PCALL` loops, verifying that the
x86_64 signal handler and `rt_sigreturn` path resume the interrupted foreign
frontend.  The Bochs prototype now records raw-mode interrupt state before
x86_64 long-mode interrupt delivery and restores the recorded foreign frontend
after `IRET64`, `SYSRET`, `SYSEXIT`, or Linux signal return reaches the
interrupted user RIP.  Final hardware still needs this state exposed as an
architectural, XSAVE-visible component.

The prototype `PCALL` forms use `R10` as the foreign target address and `R11`
as the x86_64 return continuation; `R13` optionally carries the foreign TLS
block base used by AArch64 `TPIDR_EL0` and RISC-V TLS descriptors.  They
currently cover the common register
fast path: x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` plus stack
slots `[RSP+8]` and `[RSP+16]` are mapped to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`; the foreign stack pointer is a separate 16-byte-aligned window below
the x86 frame with stack arguments copied from `[RSP+24]` onward so the first
foreign stack-passed argument is at `[sp]`;
`XMM0`-`XMM7` remain aliased to AArch64 `d0`-`d7` or RISC-V `fa0`-`fa7`,
covering scalar FP arguments/returns and two-register homogeneous double
aggregate arguments and returns through `XMM0`/`XMM1`, plus two-`float`
homogeneous aggregate returns packed into `XMM0[63:0]` and two-`float`
homogeneous aggregate arguments unpacked from `XMM0[63:0]`, including mixed
integer/FP signatures where GPR and XMM lanes are consumed in one native call;
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
`R_AARCH64_RELATIVE` or `R_RISCV_RELATIVE` relocations, packed `DT_RELR`
direct and bitmap relative relocation tables,
`R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolver relocations, and same-image symbolic 64-bit dynamic relocations
(`R_AARCH64_ABS64` or `R_RISCV_64`), applying them with the actual runtime load
bias before `PCALL`. Symbolic relocation metadata and `path#symbol` entrypoint
lookup are read from `DT_SYMTAB`/`DT_STRTAB` in the loaded dynamic image.
`DT_HASH` is used to bound the sectionless dynamic symbol table, and
`DT_GNU_HASH` is supported for common GNU-hash-only shared objects.
Section tables are kept as a fallback for synthetic test payloads. The gate
uses compiler-produced AArch64 and RISC-V shared objects
(`aarch64-pcall-real.so#poly_entry`, `riscv-pcall-real.so#poly_entry`,
`aarch64-pcall-gnu-hash-real.so#poly_entry`,
`riscv-pcall-gnu-hash-real.so#poly_entry`,
`aarch64-pcall-state.so#poly_entry`, and
`riscv-pcall-state.so#poly_entry`) plus compiler-produced imported-function
objects (`aarch64-pcall-import-real.so#poly_entry` and
`riscv-pcall-import-real.so#poly_entry`), compiler-produced imported-object
objects (`aarch64-pcall-import-value-real.so#poly_entry` and
`riscv-pcall-import-value-real.so#poly_entry`), compiler-produced weak undefined
import objects (`aarch64-pcall-weak-import-real.so#poly_entry` and
`riscv-pcall-weak-import-real.so#poly_entry`), compiler-produced relocated
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
`aarch64-pcall-fini-real.so#poly_entry`, and
`riscv-pcall-fini-real.so#poly_entry`), compiler-produced conditional objects
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
`riscv-pcall-int-highmul-real.so#poly_entry`), compiler-produced integer carry-chain
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
exclusive LL/SC, default GCC outline helpers, and LSE instructions
(`aarch64-pcall-atomic.so#poly_entry`,
`aarch64-pcall-atomic-outline.so#poly_entry`, and
`aarch64-pcall-atomic-lse.so#poly_entry`), RISC-V atomic object
(`riscv-pcall-atomic.so#poly_entry`), compiler-produced unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-produced indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-produced callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), and compiler-produced scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`) plus compiler-produced homogeneous
double aggregate return objects (`aarch64-pcall-fpair-real.so#poly_entry` and
`riscv-pcall-fpair-real.so#poly_entry`), homogeneous float aggregate return
objects (`aarch64-pcall-fpair32-real.so#poly_entry` and
`riscv-pcall-fpair32-real.so#poly_entry`), homogeneous double aggregate argument
objects (`aarch64-pcall-fpair-arg-real.so#poly_entry` and
`riscv-pcall-fpair-arg-real.so#poly_entry`), homogeneous float aggregate
argument objects (`aarch64-pcall-fpair32-arg-real.so#poly_entry` and
`riscv-pcall-fpair32-arg-real.so#poly_entry`), mixed integer/FP argument objects
(`aarch64-pcall-mixed-args-real.so#poly_entry` and
`riscv-pcall-mixed-args-real.so#poly_entry`), and scalar double/float FP
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
descriptor-dispatched FP arguments and return values. The imported-object
probes exercise real compiler-emitted GOT loads of
undefined `poly_import_value`. The function-pointer probes exercise compiler
emitted same-image data relocations to local function symbols plus native
indirect calls through `blr` or `jalr`. The constructor probes execute
compiler-emitted `DT_INIT_ARRAY` entries before the requested foreign
entrypoint. The destructor probes execute compiler-emitted `DT_FINI_ARRAY`
entries during foreign-object teardown and verify their effect on foreign
static state. The TLS probes exercise compiler-emitted AArch64 TLSDESC with
`mrs tpidr_el0` and RISC-V `__tls_get_addr` against a copied `PT_TLS` initial
image supplied through the `PCALL` TLS-base register, plus initial-exec
`R_AARCH64_TLS_TPREL64` and `R_RISCV_TLS_TPREL64` accesses against the same
TLS base. The conditional probes exercise compiler-emitted AArch64
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
`stxr`/`stlxr`, `clrex`, LSE `ldadd`, `swp`, `ldset`, and `cas`, and GCC
outline atomic helper imports for default compiler output. The RISC-V atomic
probe exercises compiler-emitted `amoadd.d`, `amoswap.d`, `amoor.w`, `lr.d`,
and `sc.d` forms from C `__atomic` builtins. The integer indexed-memory probes
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
`PT_DYNAMIC` symbol metadata. PLT-style dynamic relocation tables are accepted through
`DT_JMPREL`/`DT_PLTREL=RELA`/`DT_PLTRELSZ`, including `R_AARCH64_JUMP_SLOT` and
`R_RISCV_JUMP_SLOT` entries for defined symbols.
Packed relative relocation tables are accepted through
`DT_RELR`/`DT_RELRSZ`/`DT_RELRENT`, including direct entries and bitmap entries.
IFUNC-style resolver relocations are accepted through
`R_AARCH64_IRELATIVE` and `R_RISCV_IRELATIVE`; the resolver runs in the
foreign frontend after ordinary relocations are applied, and its return value is
written to the relocation target.
Undefined object-symbol relocations can bind to process-provided imports; the
gate covers `poly_import_value` through an undefined dynamic symbol relocation.
Undefined weak object/function relocations resolve to zero, matching ordinary
ELF optional-symbol semantics; the weak-import probes cover AArch64 weak
`GLOB_DAT`/`JUMP_SLOT` and RISC-V weak symbolic relocations.
Imported function symbols can bind to prototype hardware call-descriptor
slots. AArch64 `blr` or RISC-V `jalr` to a descriptor address maps the native
foreign argument registers through an x86/runtime import target, writes the
native foreign return register, and resumes at the foreign link address. The
gate covers deterministic `poly_import_add` and `poly_import_mul` descriptors,
proving that distinct undefined function symbols can dispatch through separate
descriptor slots. The compiler-produced import objects exercise real
PLT/GOT-backed `JUMP_SLOT` calls to `poly_import_add`: AArch64 PLT code may
branch with `br` after the caller's `bl` saved the continuation in `x30`, and
RISC-V PLT code may use `jalr` with a scratch link register while preserving
the caller continuation in `ra`. The descriptor path also accepts common libc
symbol names `strlen`, `strcmp`, `strncmp`, `memcpy`, `memmove`, `memset`,
`memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`, `strncpy`,
`strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`, `stpcpy`,
`stpncpy`, `mempcpy`, `rawmemchr`, `strchrnul`, `bcmp`, `bcopy`, `bzero`,
`index`, and `rindex`, so
compiler-produced foreign objects can call those routines through ordinary
PLT/GOT entries without using synthetic breakpoint libcalls. The neutral
cross-call gate also covers direct descriptor `strlen`, `strnlen`, `memset`,
`memcpy`, and three-argument `memcmp` calls inside foreign callees, proving
descriptor imports are not limited to x86-entered `PCALL` payloads. The gate also covers
`poly_import_x86_add`, where the descriptor
enters a real x86_64 helper target supplied by the runtime, synthesizes an x86
return address to a nearby dedicated `0f 24` `PIRET` landing pad, lets the
helper use an ordinary `ret`, and then resumes the saved AArch64/RISC-V return
PC with the x86 `RAX` result mapped back to the native foreign return register.
A raw x86 function address is still not itself a valid AArch64 or RISC-V branch
target; production hardware needs either this kind of architectural call gate
or an OS/runtime descriptor that names the x86 callable target and ABI metadata.
The same descriptor path currently provides prototype imports for common GCC
TLS accessors (`R_AARCH64_TLSDESC`, RISC-V `__tls_get_addr`, and initial-exec
`R_AARCH64_TLS_TPREL64`/`R_RISCV_TLS_TPREL64`) and common GCC
AArch64 outline atomic helpers: `__aarch64_ldadd8_acq_rel`,
`__aarch64_swp8_acq_rel`, `__aarch64_ldset4_relax`, and
`__aarch64_cas8_acq_rel`.  These are compatibility descriptors for observed
compiler output, not a general libgcc or libc implementation.

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
