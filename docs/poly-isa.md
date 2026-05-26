# Polyglot ISA Contract

This document separates the architectural target from the current Bochs
prototype.  The target is compatibility with existing precompiled AArch64 and
RISC-V code linked into an x86_64 process, not a new compiler-only ABI.

## Hardware Contract

Production hardware should not use `#UD` envelopes for hot operations.  The
Bochs envelopes are prototype encodings only.  A silicon or FPGA
implementation should expose CPUID-gated x86 instructions for:

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

Interrupts, faults, and debug exceptions taken during foreign fetch must record
the interrupted foreign mode and PC before entering the x86_64 kernel path.
`IRET`, `SYSRET`, `SYSEXIT`, and signal return must restore the foreign
frontend mode when the saved architectural state requires it.

## Bochs Prototype Contract

Bochs still uses fixed 8-byte invalid x86 envelopes because that is the least
invasive way to prototype the frontend switch:

- `65 0f 0b 52 41 57 36 34`: enter raw AArch64 fetch.
- `66 0f 0b 52 41 57 52 56`: enter raw RISC-V fetch.
- `64 0f 0b 58 4d 4f 44 45`: return to x86_64 mode.
- `40 0f 0b 50 43 41 36 34`: prototype `PCALL.A64.SYSV`.
- `40 0f 0b 50 43 52 56 36`: prototype `PCALL.RV64.SYSV`.
- `41 0f 0b 50 49 52 45 54`: prototype `PIRET`, used by
  descriptor-driven foreign-to-x86 import calls to resume the saved foreign
  return PC after an x86 helper returns normally.

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
RISC-V-to-AArch64 code without an x86 trampoline.
The native cross-call forms additionally set the callee's native link register
to a hardware return cookie, so AArch64 `ret` or RISC-V `jalr x0, 0(ra)`
restores the caller frontend mode and continuation without an x86 rendezvous.
The Bochs prototype backs this with a small bounded cross-return stack and
`polybench` covers a nested AArch64 -> RISC-V -> AArch64 call chain.  The
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
as the x86_64 return continuation.  They currently cover the common register
fast path: x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` plus stack
slots `[RSP+8]` and `[RSP+16]` are mapped to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`; the foreign stack pointer is a separate 16-byte-aligned window below
the x86 frame with stack arguments copied from `[RSP+24]` onward so the first
foreign stack-passed argument is at `[sp]`;
`XMM0`-`XMM7` remain aliased to AArch64 `d0`-`d7` or RISC-V `fa0`-`fa7`; and
AArch64 `ret x30` or RISC-V `jalr x0, 0(ra)` returns through a cookie to the
saved x86 continuation and restores the x86 stack pointer.  The AArch64 raw
decoder treats register 31 as `SP` for add/sub-immediate instructions, matching
ordinary compiler stack-frame setup and teardown.
The AArch64 raw decoder also supports `adrp` page-relative PC materialization,
NZCV-backed `adds`/`subs` plus `b.cond` for ordinary condition-code branches,
64-bit `stp`/`ldp` pair load-store forms for normal frame save/restore, and
register-offset `ldr`/`str` forms for compiler-emitted indexed memory access.
Native `bl`/`blr` link-register calls handle local helper calls inside foreign
code.  The RISC-V raw decoder aliases `x2/sp` to the shared stack pointer, so
`addi sp, sp, imm` plus `ld`/`sd` stack accesses covers the same ordinary psABI
stack-frame pattern.
The `polycall` guest tool maps foreign ELF64 `PT_LOAD` segments into a single
in-memory image before entering raw mode, so page-relative code can address
separate text/data load segments rather than only inline instruction blobs. The
gate covers both AArch64 `adrp`/`ldr` and RISC-V `auipc`/`ld` split-load
payloads. `polycall` also accepts simple `ET_DYN` images with
`R_AARCH64_RELATIVE` or `R_RISCV_RELATIVE` relocations and same-image symbolic
64-bit dynamic relocations (`R_AARCH64_ABS64` or `R_RISCV_64`), applying them
with the actual runtime load bias before `PCALL`. Symbolic relocation metadata
and `path#symbol` entrypoint lookup are read from `DT_SYMTAB`/`DT_STRTAB` in the
loaded dynamic image. `DT_HASH` is used to bound the sectionless dynamic symbol
table, and `DT_GNU_HASH` is supported for common GNU-hash-only shared objects.
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
`riscv-pcall-import-value-real.so#poly_entry`), compiler-produced relocated
function-pointer objects (`aarch64-pcall-funcptr-real.so#poly_entry` and
`riscv-pcall-funcptr-real.so#poly_entry`), compiler-produced constructor
objects (`aarch64-pcall-ctor-real.so#poly_entry` and
`riscv-pcall-ctor-real.so#poly_entry`), compiler-produced conditional objects
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
`riscv-pcall-int-div-real.so#poly_entry`), compiler-produced unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-produced indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-produced callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), and compiler-produced scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`) plus compiler-produced scalar double FP
import objects (`aarch64-pcall-fp64-import-real.so#poly_entry` and
`riscv-pcall-fp64-import-real.so#poly_entry`), compiler-produced scalar double
FP callee-saved objects (`aarch64-pcall-fp64-callee-real.so#poly_entry` and
`riscv-pcall-fp64-callee-real.so#poly_entry`), compiler-produced scalar double
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
`poly_import_fp64_add` and verify descriptor-dispatched FP arguments and return
values. The imported-object probes exercise real compiler-emitted GOT loads of
undefined `poly_import_value`. The function-pointer probes exercise compiler
emitted same-image data relocations to local function symbols plus native
indirect calls through `blr` or `jalr`. The constructor probes execute
compiler-emitted `DT_INIT_ARRAY` entries before the requested foreign
entrypoint. The conditional probes exercise compiler-emitted AArch64
logical-immediate `tst`, `csel`, and conditional-select variants
`csinc`/`csinv`/`csneg`, plus RISC-V branch/select patterns. The
compare-and-branch probes exercise compiler-emitted AArch64 `cbz`/`cbnz` on
non-`x0` registers and RISC-V ordinary branch forms. The bit-test branch probes
exercise compiler-emitted AArch64 `tbz`/`tbnz` plus matching RISC-V branch
sequences. The unsigned-bitfield
probes exercise compiler-emitted AArch64 `lsl`/`ubfx` aliases and RISC-V
shift/mask sequences. The signed-extension probes exercise compiler-emitted
AArch64 `sxth`, `sbfx`, plus `add ... asr/sxtb/sxth/sxtw` and
RISC-V signed byte/halfword/word arithmetic. The integer-division probes
exercise compiler-emitted AArch64 `udiv`/`sdiv` plus RISC-V
`divu`/`div`/`divuw`/`divw`. The integer indexed-memory probes
exercise compiler-emitted AArch64 register-offset `ldr`/`str` forms and RISC-V
shift/add indexed load-store sequences. The scalar FP callee-saved probes
exercise compiler-emitted AArch64 `stp`/`ldp` of `d8` and later plus RISC-V
`fsd`/`fld` of `fs0` and later across imported calls. The compressed-word
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
Undefined object-symbol relocations can bind to process-provided imports; the
gate covers `poly_import_value` through an undefined dynamic symbol relocation.
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
the caller continuation in `ra`. The gate also covers `poly_import_x86_add`,
where the descriptor
enters a real x86_64 helper target supplied by the runtime, synthesizes an x86
return address to a nearby `PIRET` landing pad, lets the helper use an ordinary
`ret`, and then resumes the saved AArch64/RISC-V return PC with the x86 `RAX`
result mapped back to the native foreign return register.
A raw x86 function address is still not itself a valid AArch64 or RISC-V branch
target; production hardware needs either this kind of architectural call gate
or an OS/runtime descriptor that names the x86 callable target and ABI metadata.

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
