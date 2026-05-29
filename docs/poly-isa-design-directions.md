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

The design point is semi-persistent, reconfigurable hardware for register
translation only. Reconfiguring register aliases fits the machinery an OoO CPU
already needs for rename, while reconfiguring stack or memory layouts would turn
`PCALL` into a variable-latency memory operation with page-fault and recovery
complexity.

The hardware/software split is therefore strict:

- Hardware handles the all-register fast path by selecting a cached signature
  slot and rebinding architectural names in the RAT.
- Software handles stack arguments, aggregate layout, variadics, lazy binding,
  and any ABI rule that requires memory inspection or rewriting.
- A software thunk can still finish with `PCALL` using an identity, null, or
  simple register signature after it has completed the memory-side ABI work.

The intended concrete mechanism is a Poly ABI Signature Register backed by a
small slot bank. The runtime or loader programs each slot with a standard
register-only ABI pair, for example SysV x86_64 to AAPCS64. A hot `PCALL`
encodes or supplies only the target frontend, target PC, and signature slot.
The hardware action is a rename-map update: source architectural register names
such as x86_64 `RDI,RSI,RDX` are rebound to destination names such as AArch64
`x0,x1,x2`. No operand bytes move through ALUs, no stack memory is touched, and
no call descriptor is fetched.

An ABI signature slot is a cached rename template, not a data-moving program.
At minimum, a slot records source frontend, destination frontend, register
class, destination architectural register, source architectural register, and a
valid bit for each lane. The hardware validates the slot before the frontend
redirect, then applies it as part of transition rename. It does not discover the
mapping by reading a call-site descriptor, and it does not execute a hidden
sequence of move instructions.

Modern out-of-order CPUs already rename architectural registers through a
register alias table (RAT). A Poly ABI signature is a semi-persistent RAT update
recipe: source frontend architectural names are rebound to destination frontend
architectural names during a cross-ISA branch or call. On `PCALL`, hardware
selects a cached signature slot and applies those mappings in or near the
rename stage. The data does not move through integer or FP execution pipes; only
the architectural names are rebound to existing physical registers.

This is intentionally different from defining one fixed physical register file
where, for example, x86_64 `RDI` always aliases AArch64 `x0`. Fixed aliases are
simple, but they only fit one ABI ordering well. A programmable RAT signature
lets the same silicon support multiple common precompiled-code pairings:
SysV-to-AAPCS64, AAPCS64-to-SysV, SysV-to-RISC-V psABI, and their return paths.
The runtime pays the setup cost when programming the slot, while hot call sites
pay only the frontend redirect and cached rename-map selection.

The precise-exception rule should stay simple: an invalid slot traps before
changing frontend mode or architectural PC, while a valid slot cannot fault
because it performs no memory access. RAT ownership, physical-register lifetime,
and rollback are handled by the same rename-map and checkpoint machinery that
already supports branches, exceptions, and speculative execution. The caller's
architectural view remains recoverable through the hardware transition stack
and normal precise state recovery; the callee receives only the destination
architectural names described by the signature.

For example, a SysV-to-AAPCS64 slot can map:

- x86_64 `RDI` to AArch64 `x0`
- x86_64 `RSI` to AArch64 `x1`
- x86_64 `RDX` to AArch64 `x2`
- x86_64 `RCX` to AArch64 `x3`
- x86_64 `R8` to AArch64 `x4`
- x86_64 `R9` to AArch64 `x5`

The slot bank should be small, for example 4 to 8 slots. The loader or runtime
programs common mappings once, such as SysV-to-AAPCS64, AAPCS64-to-SysV,
SysV-to-RISC-V psABI, and RISC-V psABI-to-SysV. Hot call sites then encode only
the target frontend, target PC, and signature slot. A rare mapping can either
reuse a cold slot outside the hot path or fall back to a software thunk.

This is the silicon-realistic sweet spot: common precompiled functions whose
arguments and returns fit in integer or FP registers can cross frontends without
software move thunks, while uncommon or layout-heavy calls pay the software
translation cost outside the CPU pipeline.

The preferred hot encoding is therefore `PCALL frontend, target, signature_slot`
or an equivalent immediate-slot form. The instruction must not point at a
user-memory descriptor and must not carry a dynamic bitmask for hardware to
interpret at the call site. If a symbol needs a custom register arrangement, the
loader can program a cold slot before patching or entering that call path; if it
needs memory layout conversion, the loader emits a thunk.

This is hardware-assisted thunk elision, not hardware ABI interpretation. The
architectural contract for applying a signature is branch-like:

- It performs no user-memory reads or writes.
- It cannot fault because of stack, descriptor, or aggregate layout access.
- It does not allocate temporary architectural registers.
- It only changes frontend mode, PC, return-cookie state, and register aliases.
- Its slot state is explicit Poly architectural state, saved and restored by
  the Poly/XSAVE component, not hidden CR3 or emulator bookkeeping.

Architecturally, the mechanism should look like:

- `PABI_SIG_SET slot, kind`: program a register-only mapping slot.
- `PABI_SIG_GET slot`: report the active slot kind for discovery/debugging.
- `PCALL frontend, target, sig_imm`: branch to another frontend while applying
  the selected cached mapping.

`PABI_SIG_SET` and `PABI_SIG_GET` must be frontend-neutral controls. x86_64 can
expose them for boot/runtime setup, but AArch64 and RISC-V code must be able to
program or query the same architectural slot bank without first switching back
to x86_64.

This mechanism must be strictly limited to registers:

- No hardware parsing of call descriptors.
- No user-memory reads by `PCALL`.
- No stack argument repacking.
- No by-value aggregate layout conversion.
- No variadic call handling.
- No PLT/GOT or lazy-binding policy.

Stack arguments, split aggregates, variadics, unusual FP/vector ABI cases, and
all loader policy still go through software thunks. The thunk performs memory
layout work and then uses a null, identity, or simple register signature for the
final frontend transition. This is the silicon boundary: register renaming is
small and predictable because it reuses OoO machinery, while stack and aggregate
translation needs memory accesses, page-fault recovery, and variable amounts of
work inside what should remain a fixed-latency frontend switch.

The deliberate hybrid model is:

- Hardware handles the common all-register case by applying a cached signature
  with no operand data movement.
- Software handles stack layout, by-value aggregates, variadics, lazy binding,
  and unusual vector cases before making the final fixed-latency transition.
- The null signature is the exchange-window ABI, so every implementation has a
  simple fallback even without RAT remapping.

This is the intended "90/10" split: hot calls whose arguments and returns fit in
native ABI registers should avoid thunks entirely through RAT remapping, while
the uncommon stack, aggregate, vector, or variadic cases stay in software where
memory access and policy belong.

Put another way, the hardware should remove thunks that only shuffle registers.
It should not attempt to remove thunks whose job is ABI memory semantics. Stack
overflow arguments, by-value structs, variadic save areas, PLT/GOT policy, and
lazy binding are all observable software ABI contracts; putting them in `PCALL`
would require page-fault-capable memory reads and writes inside the transition
operation and would make the instruction unsuitable as a fast silicon primitive.

Fixed-width vector values are eligible for this fast path only when both native
ABIs place the value in compatible vector/SIMD architectural registers. A
128-bit x86_64 SysV vector to AArch64 AAPCS64 vector call can be a direct
register-signature transition. A RISC-V psABI call that represents the same
128-bit value as integer register pairs is not the same hardware operation; it
requires cross-class reshaping and should use a software thunk or an explicitly
defined future signature kind.

The fast path therefore covers ordinary register-only calls between existing
precompiled objects, for example an x86_64 caller passing six scalar SysV
arguments to an AArch64 AAPCS64 function. The slow path is not a failure mode;
it is the architectural escape hatch for anything whose native ABI contract
requires memory layout work. A loader can choose per symbol or per relocation:
direct `PCALL ... sig_imm` for all-register signatures, or a generated thunk
that performs stack/aggregate conversion and then uses a null, identity, or
simple signature for the final branch.

The performance target for a hot signature `PCALL` is a frontend redirect plus
a rename-map selection, not a sequence of register moves. Slot programming is a
cold control operation performed by the loader or runtime; applying a slot at a
call site should be comparable to selecting a cached RAT template in the rename
stage. This is why the slot count should stay small and explicit, such as 4 to
8 entries, and why signatures must describe only architectural-register aliasing.
More precisely, the goal is zero execution-unit data-move latency for ABI
register handoff; the transition still has normal branch/frontend redirect
costs.

The silicon cost is intentionally bounded: a few architectural control
registers or XSAVE-backed slot records, validation logic for the slot number,
and muxing in the rename path. The design must not grow into a memory-layout
engine. If a call requires stack argument repacking, by-value struct conversion,
variadic metadata, or page-fault-capable memory reads, the loader/runtime emits
or enters a software thunk and then uses a null, identity, or simple register
signature for the final cross-ISA branch.

The Bochs prototype currently models this with eight signature slots. Slot kind
`0` is the exchange-window mapping, kind `1` is the older x86_64 SysV
compatibility mapping, and kind `2` is the hardware-oriented x86_64 SysV
register-only mapping. Kind `2` is the important silicon-facing form: it maps
the six SysV integer argument registers into the target ABI without stack
access. Kind `3` uses the same argument mapping but treats the x86
`RAX/RDX` pair as a two-register integer return for ABIs that return
`unsigned __int128`-class values in two GPRs. The final silicon-oriented
encoding should use a compact slot immediate.

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
privileged special case. In the prototype, foreign `PCALL frontend=0` supports
both descriptor-backed import targets in the reserved import range and direct
x86 targets. Direct signature calls select a cached register-only slot before
entering x86. The CPU places a hardware return cookie on the x86 stack, and
ordinary x86 `ret` consumes that cookie to restore the foreign frontend and
continuation. Software thunks still own ABI and loader policy while the CPU
control path stays frontend-neutral.

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

Non-window GPRs, FP/SIMD registers, vector state, and special registers remain
separate architectural state preserved through the poly XSAVE component.

The same idea can be extended to a fixed FP exchange window later, but integer
lanes should come first because they cover most loader, syscall, and simple C
function boundaries.

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
