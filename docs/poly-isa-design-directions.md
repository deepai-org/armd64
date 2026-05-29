# Poly ISA Design Directions

This note records ISA-level directions for making Poly less like "x86 plus
foreign guests" and more like one CPU with multiple peer frontends. The goal is
compatibility with existing precompiled x86_64, AArch64, and RISC-V code while
keeping the hardware contract OS-neutral and suitable for silicon or FPGA.

## Design Principle

Poly should be a generic multi-frontend CPU extension. x86_64 is the boot and
system frontend, not the semantic center of all execution.

The hardware should accelerate frontend transitions, precise traps, explicit
architectural state, register-only ABI handoff, and native return recovery. It
should not parse dynamic-linker descriptors, rewrite stack layouts, or know
Linux, libc, dynamic linker policy, or helper-function semantics.

The only intended reconfigurable ABI hardware is a semi-persistent register
alias/signature slot bank. It may rebind architectural register names through
rename/RAT state, but stack layouts, by-value aggregates, variadics, and other
memory-side ABI work stay in software thunks.

## Generic Frontend IDs

Avoid baking the ISA around pairwise x86-to-AArch64 and x86-to-RISC-V
operations. Define architectural frontend IDs instead:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

The architectural operations should be mode-generic and fixed-latency:

- `PENTER frontend`: enter a frontend from x86_64/system code.
- `PSWITCH frontend, target`: branch to a target in another frontend.
- `PCALL frontend, target`: push a hardware transition-stack entry, install a
  return cookie, and branch to a target in another frontend.
- `PTRAPRET`: resume after a precise poly trap.

The Bochs prototype can continue to encode these through its compact x86
control page, but the contract should describe generic frontend routing.

## No Hardware-Parsed Call Descriptors

Real native ABIs cannot be represented by one fixed register shuffle. Stack
arguments, aggregate returns, variadic calls, vector arguments, callbacks,
PLT/GOT lazy binding, and helper imports all need metadata.

Hardware should not read or parse user-memory call descriptors on the `PCALL`
fast path. That would make the transition instruction variable latency, create
new page-fault points inside control-flow execution, and force the frontend to
understand ABI metadata.

Instead, loader-generated software thunks own ABI translation. A thunk moves
arguments into the hardware exchange window, lays out target stack arguments,
handles aggregate returns or variadics, and then issues a fixed-latency
`PSWITCH` or `PCALL`. Complex metadata can still exist in user memory, but it
is consumed by runtime code when generating or entering a thunk, not by the CPU
pipeline on every transition.

## Programmable RAT ABI Signature Slots

A fixed exchange window is simple, but it still forces software moves for
ordinary native ABI calls such as SysV x86_64 `RDI,RSI,RDX` to AArch64
`x0,x1,x2`. Silicon can do better without becoming a memory-marshalling engine
by exposing a small bank of register-only ABI signature slots.

The design rule is narrow: semi-persistent, reconfigurable hardware is useful
only for register renaming. It must not become a configurable stack or memory
layout engine. Register aliasing fits the existing rename stage of an OoO CPU;
stack repacking, by-value aggregate conversion, and variadic argument handling
would require memory reads, memory writes, ABI policy, and page-fault-capable
microcode in the transition path.

The mechanism is a programmable register alias table (RAT) template. Modern
OoO cores already rename architectural registers onto physical registers; a
Poly ABI signature reuses that machinery by rebinding destination frontend
architectural names to source frontend physical registers during `PCALL`. The
data does not move through integer or FP execution pipes. The rename stage only
selects a cached mapping, for example:

- x86_64 `RDI` to AArch64 `x0`
- x86_64 `RSI` to AArch64 `x1`
- x86_64 `RDX` to AArch64 `x2`
- x86_64 `RCX` to AArch64 `x3`
- x86_64 `R8` to AArch64 `x4`
- x86_64 `R9` to AArch64 `x5`

This makes semi-persistent ABI reconfiguration plausible in silicon because
the CPU is only selecting physical-register mappings it already has to track.
It is not a general ABI translation engine. The implementation target is a
small set of prevalidated mapping slots plus rename-stage muxing, not
microcode that walks user stacks, copies structs, or handles page faults inside
the transition instruction.

Architecturally, each slot is a semi-persistent Poly ABI Signature Register:
it holds a compact, prevalidated register-renaming recipe such as "SysV
x86_64 integer args to AAPCS64 integer args" or "AAPCS64 scalar FP args to
SysV scalar FP args." A `PCALL` carries the target frontend, target PC, and a
small immediate slot selector. The hot path does not program a mapping, read a
descriptor, or execute moves; it only applies the cached rename recipe while
redirecting the frontend.

The preferred generic signature kind is `native-registers`: source frontend
native ABI argument/result registers are rebound to target frontend native ABI
argument/result registers. For x86_64 this means SysV lanes such as
`RDI,RSI,RDX,RCX,R8,R9`; for AArch64 it means AAPCS64 `x0..x7` and
`v0..v7`; for RISC-V it means psABI `a0..a7` and `fa0..fa7`. This avoids
encoding x86-specific policy into the architectural fast path while preserving
compatibility with ordinary precompiled code.

The slot contents are intentionally small enough to look like rename metadata,
not microcode. A practical slot records the source frontend, destination
frontend, argument/result register class mappings, and validity bits. Applying
it is a rename checkpoint operation: destination architectural names are made
to point at the physical registers already holding the caller's live argument
values. No integer, FP, SIMD, load, or store execution unit needs to move the
data.

This is a hardware ABI accelerator, not a hardware ABI interpreter. For
example, a loader can program slot 0 with the standard SysV x86_64 to AAPCS64
register mapping. A hot call then executes a form such as
`PCALL mode, target, slot0`; the hardware action is only to make destination
names like AArch64 `x0,x1,x2` refer to the same physical registers currently
named by x86_64 `RDI,RSI,RDX`. The data never moves, and the instruction does
not inspect memory.

The slot bank is semi-persistent hardware state. A loader or runtime programs a
small number of slots, for example 4 to 8, with common register-only ABI pairs:
SysV-to-AAPCS64, AAPCS64-to-SysV, SysV-to-RISC-V psABI, and RISC-V
psABI-to-SysV. A hot call site then executes `PCALL frontend, target, sig_imm`.
The immediate selects a prevalidated slot; the instruction does not fetch a
user-memory descriptor or carry a dynamic per-call bitmask for hardware to
interpret.

This gives a concrete lifecycle:

- At load time, the runtime writes a Poly ABI Signature Register with a compact
  register-only mapping such as "SysV integer and scalar FP args to AAPCS64."
- The CPU validates the mapping once and caches it as a RAT template in the
  selected signature slot.
- A hot call site encodes only the target frontend, target PC, and slot number.
- During `PCALL`, the rename stage applies the cached template by rebinding
  architectural names to the existing physical registers.
- The data never moves, and the transition does not allocate a memory-side ABI
  sequencer.

The runtime should therefore program slots at load time, link time, or lazy
binding time, not on every call. Call sites that match a cached signature become
one frontend-control instruction. Call sites that need a custom layout branch
to a generated thunk; the thunk can still end with `PCALL` using a null,
identity, or simple cached signature after it has finished the memory work.

This is the only kind of reconfigurable ABI hardware that should be in the
ISA. Semi-persistent register remapping fits existing OoO machinery because it
is just a controlled rename-map update. The hardware is not asked to understand
stack layouts, copy overflow arguments, split by-value structs, or inspect
variadic metadata. Trying to extend the same mechanism from registers into
memory would turn `PCALL` into a variable-latency microcoded ABI engine with
new page-fault points in the transition path.

This is the silicon-realistic middle ground between a fixed exchange window and
a hardware ABI interpreter. The exchange window remains the null signature and
portable fallback. Signature slots remove thunks whose only job is register
shuffling, but they deliberately do not remove thunks whose job is ABI memory
semantics.

The area argument is the same as the latency argument. A few signature
registers plus muxing in rename/dispatch is small, deterministic hardware.
Adding stack or struct rewriting would require a memory walker, store
generation, rollback state, and page-fault handling inside the transition
instruction. That is a different class of machine, and it is not the Poly ISA
contract.

This is also where the design draws the line on "hardware ABI translation."
Register renaming is architectural state selection. Stack and aggregate
translation is memory transformation. The former fits naturally into the
rename/checkpoint machinery already required by an OoO frontend; the latter
creates variable-latency execution and precise-exception complexity. Poly
should expose the former and require software thunks for the latter.

The hardware/software split is strict:

- Hardware handles register-only argument and return handoff by selecting a
  cached signature slot and rebinding architectural names in RAT state.
- Hardware may include compatible integer and FP/SIMD register lanes in a
  signature, but only when source and destination ABI classes match.
- Software handles stack arguments, by-value aggregate layout, variadics,
  PLT/GOT and lazy-binding policy, cross-class vector reshaping, and any ABI
  rule that requires memory inspection or rewriting.
- A software thunk can still finish with `PCALL` using an identity, null, or
  simple register signature after it has completed memory-side ABI work.

The architectural contract for applying a signature is branch-like:

- It performs no user-memory reads or writes.
- It cannot fault because of stack, descriptor, or aggregate layout access.
- It does not allocate temporary architectural registers.
- It only changes frontend mode, PC, return-cookie state, and register aliases.
- Its slot state is explicit Poly architectural state, saved and restored by
  the Poly/XSAVE component, not hidden CR3 or emulator bookkeeping.

Architecturally, the controls should be:

- `PABI_SIG_SET slot, kind`: program a register-only mapping slot.
- `PABI_SIG_GET slot`: report the active slot kind for discovery/debugging.
- `PCALL frontend, target, sig_imm`: branch to another frontend while applying
  the selected cached mapping.

For example, a runtime can program slot 0 as SysV x86_64 to AAPCS64 so
`RDI,RSI,RDX` become `x0,x1,x2`, slot 1 as AAPCS64 to SysV, and slot 2 as
SysV to RISC-V psABI. Register-only call sites then use `PCALL` with an
immediate slot selector. Calls with stack arguments, structs, vectors that need
class conversion, or variadics branch to a generated software thunk first; that
thunk performs memory-side ABI work and then finishes with an identity or
simple signature `PCALL`.

`PABI_SIG_SET` and `PABI_SIG_GET` must be frontend-neutral controls. x86_64 can
expose them for boot/runtime setup, but AArch64 and RISC-V code must be able to
program or query the same architectural slot bank without first switching back
to x86_64.

The precise-exception rule stays simple: an invalid slot traps before changing
frontend mode or architectural PC, while a valid slot cannot fault because it
performs no memory access. RAT ownership, physical-register lifetime, and
rollback are handled by the same rename-map and checkpoint machinery used for
branches, exceptions, and speculation. The caller's architectural view remains
recoverable through the hardware transition stack and precise state recovery;
the callee receives only the destination architectural names described by the
signature.

The performance target for a hot signature `PCALL` is zero execution-unit
data-move latency for ABI register handoff. It is not a literal zero-cycle
call: the transition still pays normal control-redirect, frontend-switch,
return-cookie, and rename-checkpoint costs. The important property is that a
register-only cross-ISA call does not dispatch move instructions, enter a
software thunk, parse a descriptor, or touch stack memory.

This is also the area target. The hardware addition should be a handful of
architectural signature registers, prevalidation logic, and muxing in the
rename path. It must not grow into a stack-layout transformer, struct splitter,
variadic argument scanner, or page-fault-capable memory sequencer.

This is the intended hybrid model. Common precompiled functions whose arguments
and returns fit in standard integer or FP registers can cross frontends through
a few-slot RAT-remap fast path. Stack-heavy, aggregate-heavy, variadic, vector
layout, and lazy-binding cases stay in generated thunks where page faults,
memory policy, and ABI-specific layout rules belong.

The practical target is a 90/10 split:

- Hardware handles the common register-only calls with a semi-persistent,
  reconfigurable slot bank. A hot `PCALL` names the target frontend, target PC,
  and signature slot; the rename stage applies the cached RAT template.
- Software handles the uncommon but semantically complex cases. The thunk
  marshals stack arguments, structs, variadics, lazy binding, or vector layout,
  then performs the final jump with a null, identity, or simple register
  signature.

This keeps the architectural fast path sympathetic to real precompiled code
without bloating the CPU into an ABI-specific memory transformer.
It also keeps the implementation realistic for FPGA and silicon prototypes:
the small hardware addition is a bank of prevalidated mapping registers plus
rename-stage muxing, while all page-fault-capable memory layout work remains
ordinary software.

The Bochs prototype currently models this with eight signature slots. Slot kind
`0` is the exchange-window mapping, kind `1` is the older x86_64 SysV
compatibility mapping, and kind `2` is the hardware-oriented x86_64 SysV
register-only mapping. Kind `3` uses the same argument mapping but treats the
x86 `RAX/RDX` pair as a two-register integer return for ABIs that return
`unsigned __int128`-class values in two GPRs. Kind `4` is the preferred
silicon-facing form: it maps the source frontend's native integer and FP ABI
lanes onto the target frontend's native lanes without stack access. Kinds `2`
and `3` remain valid prototype aliases for x86-oriented tests and direct x86
imports, but hot neutral cross-frontend calls should use kind `4`. The final
silicon-oriented encoding should use a compact slot immediate.

The current preferred Bochs generic `PCALL` form uses frontend ID in `R15`,
target PC in `RBX`, return PC in `R11`, and an immediate signature-slot byte.
Foreign frontends have matching immediate-slot forms: AArch64 reserves
`HINT #0x60..#0x67`, and RISC-V reserves custom-0 subops `16..23`.
Older register-slot forms remain available for compatibility with existing
probes while the temporary control encoding evolves. `PSWITCH_MODE` uses the
same frontend ID and target registers but does not install a return cookie.

Foreign generic `PSWITCH` controls use the existing scratch branch registers:
AArch64 `x16=target, x17=frontend ID`; RISC-V `x5=target, x6=frontend ID`.
Foreign generic `PCALL` adds one scratch continuation register: AArch64
`x18=return PC`; RISC-V `x7=return PC`. Foreign `PCALL_SIG_IMM` encodes the
signature slot in the control instruction, avoiding a temporary slot register
on hot paths. The older foreign `PCALL_SIG` register-slot controls use AArch64
`x19` and RISC-V `x28`. The callee still returns with its ordinary native
return instruction through the hardware return cookie.

The frontend ID space includes x86_64 as frontend `0`; it should not be a
privileged special case. Direct signature calls select a cached register-only
slot before entering x86. The CPU places a hardware return cookie on the x86
stack, and ordinary x86 `ret` consumes that cookie to restore the foreign
frontend and continuation. The source frontend stack pointer is exposed to x86
in volatile `R11`, giving software thunks enough information to copy overflow
stack arguments without making `PCALL` parse descriptors or rewrite memory.
Software thunks still own ABI and loader policy while the CPU control path
stays frontend-neutral. The Bochs reserved import-call descriptor range is a
compatibility fallback, not a required silicon ABI.

## Register Exchange Window

Fully separate register files are clean but force every cross-ISA call to spill
through memory. Full global aliasing is also wrong because each ISA has
different register counts and preservation rules.

Define a small physical exchange window as the baseline/null signature shared by
the common integer argument/result lanes:

| Window | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| `P0` | `RAX` | `x0` | `a0` |
| `P1` | `RDX` | `x1` | `a1` |
| `P2` | `RCX` | `x2` | `a2` |
| `P3` | `RDI` | `x3` | `a3` |
| `P4` | `RSI` | `x4` | `a4` |
| `P5` | `R8` | `x5` | `a5` |
| `P6` | `R9` | `x6` | `a6` |
| `P7` | `R10` | `x7` | `a7` |

Software thunks can always translate native ABI argument order into this window
before the mode switch. Hardware ABI signature slots are an optimization over
the same model: when a call fits entirely in registers, the signature maps the
native source ABI registers directly onto the native destination ABI registers
without executing moves.

Non-window GPRs, non-ABI FP/SIMD registers, vector state, and special
registers remain separate architectural state preserved through the poly XSAVE
component.

Scalar FP and compatible fixed 128-bit SIMD ABI lanes should use the same
direct-register principle as the integer window when source and destination ABI
classes match. Wider AVX, SVE, RVV, cross-class vector reshaping, stack FP
overflow, and aggregate/vector layout conversion stay in software thunks unless
a future signature kind explicitly covers them.

## Native Cross-ISA Returns

Special poly-only return instructions should not be required for compatibility
with ordinary precompiled code. Native returns should be able to cross
frontends without slowing every same-ISA return:

- x86_64 `ret`
- AArch64 `ret x30`
- RISC-V `ret` / `jalr x0, ra, 0`

Use a hardware transition stack (HTS). On cross-ISA `PCALL`, hardware pushes
the caller frontend, caller PC, caller SP, and required flags to the HTS. It
then places a reserved return cookie in the callee's native return location:

- x86_64 stack return slot
- AArch64 `x30`
- RISC-V `ra`

Normal same-ISA returns are unaffected. When a native return branches to the
reserved cookie address, hardware takes a fast internal transition fault, pops
the HTS, restores the caller frontend and continuation PC, and resumes. If the
HTS is empty or corrupted, the event becomes a precise poly trap.

## Register State

Do not globally alias all frontend registers. x86_64, AArch64, and RISC-V have
different register counts, caller/callee-save rules, FP/vector layouts, and
special registers.

The cleaner contract is:

- Each frontend has its own real architectural register file outside the
  exchange window.
- The exchange window aliases the hot integer argument/result lanes.
- Software thunks perform ABI-defined argument/result movement.
- Long-lived non-current frontend registers live in explicit poly XSAVE state.
- There is no hidden CR3-keyed or thread-hashed emulator state in the hardware
  contract.
- Bochs-only state-key controls are diagnostics for the prototype, not
  advertised architectural features.

This is more compatible with threads, signals, debuggers, and precompiled ABI
expectations.

## Trap Packet Policy

Hardware should emit precise trap packets and stop there. It should not emulate
syscalls, libcalls, libgcc, libatomic, or dynamic linker policy.

Trap-producing events include:

- foreign `svc` / `ecall`
- foreign breakpoint instructions
- illegal instructions
- unsupported instructions
- unresolved imports
- page faults during foreign execution
- asynchronous interrupts during foreign execution

User-space runtime code should be able to register a per-thread poly monitor
entry point and trap-packet address through poly control registers. For
recoverable user events, hardware writes the packet to that address and enters
the monitor in Ring 3. The monitor can translate foreign syscalls, perform lazy
binding, call helper thunks, or reflect breakpoints to a debugger policy.
Those controls should be frontend-neutral: AArch64 and RISC-V code must be
able to install or query the same trap vector and monitor packet state directly,
instead of relying on an x86-only setup path.

The OS is only required for hard faults and real asynchronous events: monitor
page faults, target page faults that cannot be resolved in user space, hardware
interrupts, process signals, scheduling, and actual kernel syscalls issued by
the monitor.

## Memory Model

Define one global memory model for Poly execution. The simplest hardware and
software contract is x86-style TSO for all frontends.

This is stronger than native AArch64 and RISC-V, so correctly synchronized
foreign code remains correct. AArch64 barriers and RISC-V fences can be treated
as cheap ordering points or no-ops when the stronger global ordering already
satisfies their requirements.

## Poly XSAVE State

Expose poly state as a formal XSAVE-style component. The component should
contain at least:

- active frontend mode
- interrupted frontend mode and PC
- current trap packet
- hardware transition stack state
- ABI signature slot state
- AArch64 GPR state
- AArch64 FP/SIMD state
- RISC-V GPR state
- RISC-V FP state
- per-frontend TLS bases
- user-space poly monitor PC and trap-packet address

The OS should only need generic XSAVE/XRSTOR enablement. It should not need to
understand AArch64 or RISC-V register semantics to context-switch a task.

## Landing Pads

Cross-frontend branch targets should optionally begin with frontend-specific
landing-pad instructions:

- x86_64 poly landing opcode: prototype subop `0x05`
- AArch64 reserved `HINT`: prototype `HINT #0x7b`
- RISC-V custom-0 marker: prototype subop `11`

Landing pads give hardware a validation point for indirect cross-ISA control
flow, help catch wrong-frontend targets, and provide a path for CET/BTI-like
hardening without making normal direct calls expensive. The Bochs prototype
currently decodes these markers as no-ops and exposes them through CPUID; target
validation policy can be added later without changing the marker encodings.

## Implementation Priority

The next ISA-level work should prioritize:

1. Generic frontend IDs instead of pairwise mode names.
2. Fixed-latency `PSWITCH`/`PCALL` with no hardware descriptor parsing.
3. A baseline integer exchange window plus register-only ABI signature slots.
4. XSAVE poly state as the only context-switch contract.
5. Native return-cookie support through the HTS.
6. User-space poly monitor delivery for recoverable traps.

These changes make Poly more sympathetic to precompiled code: ordinary
functions keep ordinary ABI behavior, while hardware accelerates the crossing
points and software thunks handle ABI complexity outside the CPU pipeline.
