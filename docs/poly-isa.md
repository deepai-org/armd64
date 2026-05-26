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
`IRET`/`SYSRET`/signal return must restore the foreign frontend mode when the
saved architectural state requires it.

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
  return PC after the x86 helper finishes.

The prototype `PCALL` forms use `R10` as the foreign target address and `R11`
as the x86_64 return continuation.  They currently cover the common register
fast path: x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` plus stack
slots `[RSP+8]` and `[RSP+16]` are mapped to AArch64 `x0`-`x7` or RISC-V
`a0`-`a7`; the foreign stack pointer is exposed as the x86 caller stack plus
24 bytes so the first foreign stack-passed argument is at `[sp]`;
`XMM0`-`XMM7` remain aliased to AArch64 `d0`-`d7` or RISC-V `fa0`-`fa7`; and
AArch64 `ret x30` or RISC-V `jalr x0, 0(ra)` returns through a cookie to the
saved x86 continuation and restores the x86 stack pointer.  The AArch64 raw
decoder treats register 31 as `SP` for add/sub-immediate instructions, matching
ordinary compiler stack-frame setup and teardown.
The AArch64 raw decoder also supports `adrp` page-relative PC materialization,
NZCV-backed `adds`/`subs` plus `b.cond` for ordinary condition-code branches,
64-bit `stp`/`ldp` pair load-store forms for normal frame save/restore, and
native `bl`/`blr` link-register calls for local helper calls inside foreign
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
table, with section tables kept as a fallback for synthetic test payloads. The
gate uses nonzero `poly_entry` symbol offsets for the
dynamic-relocation probes so symbol resolution, not the ELF entrypoint, selects
the target, and also includes sectionless `dyntab` probes that exercise only
`PT_DYNAMIC` symbol metadata. PLT-style dynamic relocation tables are also
accepted through `DT_JMPREL`/`DT_PLTREL=RELA`/`DT_PLTRELSZ`, including
`R_AARCH64_JUMP_SLOT` and `R_RISCV_JUMP_SLOT` entries for defined symbols.
Undefined object-symbol relocations can bind to process-provided imports; the
gate covers `poly_import_value` through an undefined dynamic symbol relocation.
Imported function symbols can bind to prototype hardware call-descriptor
slots. AArch64 `blr` or RISC-V `jalr` to a descriptor address maps the native
foreign argument registers through an x86/runtime import target, writes the
native foreign return register, and resumes at the foreign link address. The
gate covers deterministic `poly_import_add` and `poly_import_mul` descriptors,
proving that distinct undefined function symbols can dispatch through separate
descriptor slots. It also covers `poly_import_x86_add`, where the descriptor
enters a real x86_64 helper target supplied by the runtime, lets that helper
fall through to `PIRET`, and then resumes the saved AArch64/RISC-V return PC
with the x86 `RAX` result mapped back to the native foreign return register.
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
