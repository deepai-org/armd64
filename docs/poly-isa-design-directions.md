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
`x0,x1,x2`. Silicon can do better without becoming a memory-marshalling engine:
make the reconfigurable hardware strictly register-only.

Modern OoO cores already rename architectural registers onto physical
registers through a register alias table (RAT). A Poly ABI Signature Register
is a compact, prevalidated RAT template. When a hot `PCALL` names a signature
slot, the rename stage rebinds destination frontend architectural names to the
physical registers already holding source ABI arguments. No integer, FP, SIMD,
load, or store execution unit moves the data, and the transition instruction
does not inspect user memory.

This is the right level of reconfigurable hardware because it fits existing
rename machinery. The hardware changes are a small slot bank, decode-visible
slot selection, prevalidation, and muxing in the rename path. The data path does
not gain a stack-layout engine, descriptor walker, or ABI-specific memory
sequencer. In a real OoO implementation, applying a signature is closer to
selecting a rename-map template than executing a register-copy routine.

The hard design rule is that a signature slot reconfigures names, not layouts.
It may change which physical register backs `RDI`, `x0`, or `a0`; it must not
read a call descriptor, inspect a stack frame, split an aggregate, or rewrite
memory. Crossing that boundary turns `PCALL` into a variable-latency,
page-fault-capable ABI microsequencer, which is exactly what this design avoids.

The slot bank should be semi-persistent hardware state. A loader or runtime
programs a small number of slots, for example 4 to 8, with common register-only
ABI pairs such as SysV-to-AAPCS64, AAPCS64-to-SysV, SysV-to-RISC-V psABI, and
RISC-V psABI-to-SysV. Hot call sites encode only the target frontend, target
PC, and signature slot immediate. The mapping is not programmed on every call.
The area budget is intentionally small: several prevalidated mapping registers
plus muxing/check logic in the rename path, not a second ABI engine beside the
load/store unit.

The intended fast path is:

1. The loader or runtime programs a slot with a common native ABI mapping.
2. A hot call site executes `PCALL frontend, target, sig_imm`.
3. Decode selects the cached slot and redirects to the target frontend.
4. Rename applies the register alias template.
5. The callee observes its ordinary native ABI argument registers already
   populated.

No per-call software register shuffle is required for calls that fit in the
mapped lanes. No per-call hardware descriptor fetch is allowed.

This is a semi-persistent configuration cache, not per-call metadata. The
loader/runtime programs a slot once, for example "SysV x86_64 to AAPCS64
register arguments", and hot sites use `PCALL frontend, target, sig_imm` to
select that slot. The fast path should be a frontend redirect plus RAT-template
selection; it should not execute a register-shuffle thunk, reprogram the slot,
or fetch an ABI descriptor on every call.

Example signature action:

- x86_64 `RDI` to AArch64 `x0`
- x86_64 `RSI` to AArch64 `x1`
- x86_64 `RDX` to AArch64 `x2`
- x86_64 `RCX` to AArch64 `x3`
- x86_64 `R8` to AArch64 `x4`
- x86_64 `R9` to AArch64 `x5`

Allowed signature work:

- Rebind integer argument/result registers between compatible native ABI lanes.
- Rebind FP argument/result registers between compatible native ABI lanes.
- Rebind fixed-width SIMD lanes only when both ABIs classify the value the same
  way and the width is architecturally supported.
- Install the cross-frontend return cookie and update the hardware transition
  stack as part of the same branch-like operation.

Forbidden signature work:

- Read or write the user stack.
- Parse a user-memory call descriptor.
- Split, pack, or repack by-value aggregates.
- Scan variadic metadata.
- Lazy-bind symbols or walk PLT/GOT structures.
- Reshape incompatible vector formats such as AVX-512, SVE, or RVV.

These forbidden cases are exactly where software thunks remain necessary.
Stack arguments, by-value structs, variadic functions such as `printf`, hidden
structure-return pointers, and incompatible vector layouts are memory and
language-ABI problems, not rename problems. Handling them in hardware would
require page-fault-capable microcode and OS-visible recovery state inside the
transition instruction, which would make the common case slower and less
portable.

The preferred generic signature kind is `native-registers`: source frontend
native ABI argument/result registers are rebound to target frontend native ABI
argument/result registers. For x86_64 this means SysV lanes such as
`RDI,RSI,RDX,RCX,R8,R9` and `XMM0..XMM7`; for AArch64 it means AAPCS64
`x0..x7` and `v0..v7`; for RISC-V it means psABI `a0..a7` and `fa0..fa7`.
This preserves compatibility with ordinary precompiled code instead of
inventing a compiler-only ABI.

The hardware/software split is strict:

- Hardware handles register-only argument and return handoff by selecting a
  cached signature slot and rebinding architectural names in RAT state.
- Hardware may include compatible integer, FP, and fixed SIMD lanes in a
  signature when source and destination ABI classes match.
- Hardware must not read user-memory descriptors, repack stacks, split structs,
  scan variadic metadata, reshape vectors, or perform memory-side ABI
  translation.
- Software thunks handle overflow stack arguments, by-value aggregates,
  variadics such as `printf`, target stack alignment, PLT/GOT and lazy-binding
  policy, and incompatible vector layouts.
- A thunk may finish with a null, identity, or simple register signature
  `PCALL` after it completes the page-fault-capable memory work.

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

`PABI_SIG_SET` and `PABI_SIG_GET` must be frontend-neutral controls. x86_64 can
expose them for boot/runtime setup, but AArch64 and RISC-V code must be able to
program or query the same architectural slot bank without first switching back
to x86_64.

The precise-exception rule stays simple: an invalid slot traps before changing
frontend mode or architectural PC, while a valid slot cannot fault because it
performs no memory access. RAT ownership, physical-register lifetime, and
rollback are handled by the same rename-map and checkpoint machinery used for
branches, exceptions, and speculation.

The performance target for a hot signature `PCALL` is zero execution-unit
data-move latency for ABI register handoff, not a literal zero-cycle call. The
transition still pays normal control-redirect, frontend-switch, return-cookie,
and rename-checkpoint costs. The important property is that a register-only
cross-ISA call does not dispatch move instructions, enter a software thunk,
parse a descriptor, or touch stack memory.

This is the silicon-realistic hybrid model. Common precompiled functions whose
arguments and returns fit in standard integer or FP registers can cross
frontends through a few-slot RAT-remap fast path. Stack-heavy, aggregate-heavy,
variadic, lazy-binding, and incompatible vector cases stay in generated thunks
where page faults, memory policy, and ABI-specific layout rules belong. The
area cost is a small bank of signature registers, prevalidation logic, and
rename-stage muxing, not a page-fault-capable ABI memory sequencer.

Put differently, the hardware should accelerate the common ABI case and refuse
to become a general ABI interpreter. If a call's live ABI state is already in
ordinary argument/result registers, a signature slot can make the transition
look like a frontend branch plus RAT remap. If the call needs stack arguments,
by-value struct packing, variadic layout, lazy symbol binding, or vector-shape
conversion, it is not a hardware signature call anymore; it is a software thunk
that may end with a simple `PCALL`. This keeps the fast path small enough for
silicon while still giving loaders and runtimes a correct path for the full
native ABI.

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
stays frontend-neutral. The Bochs reserved import-call range is now a trap
delivery surface for unresolved imports, not a CPU-parsed descriptor ABI.

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
decodes these markers as no-ops when policy is disabled and exposes opt-in
policy bits that require landing pads for indirect `PSWITCH` targets,
indirect `PCALL` targets, or both. The policy is explicit Poly XSAVE state, not
hidden emulator state, so context switching can preserve it like the signature
slot bank.

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
