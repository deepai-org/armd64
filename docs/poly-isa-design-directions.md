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

A fixed exchange window is simple, but it forces software to move registers for
ordinary native ABI calls such as SysV x86_64 `RDI,RSI,RDX` to AArch64
`x0,x1,x2`. Silicon can do better without becoming a memory-marshalling engine:
allow the runtime to program a small set of register-only ABI signature slots.

Modern out-of-order CPUs already rename architectural registers through a
register alias table (RAT). A poly ABI signature should be a small programmable
RAT update recipe: source frontend architectural register names are rebound to
destination frontend architectural register names for the call and return fast
paths. On `PCALL`, hardware selects a cached signature slot and applies those
mappings in or near the rename stage. The data does not move; only
architectural names are rebound to physical registers.

This is hardware-assisted thunk elision, not hardware ABI interpretation. For
register-only calls, a signature can replace a sequence of software moves such
as `RDI,RSI,RDX` to `x0,x1,x2` with a rename-table update. For calls whose ABI
meaning depends on stack layout, aggregate layout, variadic metadata, lazy
binding, or language runtime policy, the loader/runtime still emits a software
thunk and uses `PCALL` only for the final fixed-latency frontend branch.

This is the only kind of semi-persistent, reconfigurable ABI hardware that
belongs on the fast path. Register renaming fits the existing OoO machinery: the
runtime programs a small control state block, and later transitions consume that
state by updating rename mappings. Trying to extend the same mechanism to
memory, stack slots, or aggregate layouts would require page-walking loads,
variable-latency microcode, partial writes, and precise fault recovery inside
the transition instruction. That crosses the boundary from an ISA switch into a
general ABI-marshalling engine and should be kept out of hardware.

For example, a SysV-to-AAPCS64 signature can map:

- x86_64 `RDI` to AArch64 `x0`
- x86_64 `RSI` to AArch64 `x1`
- x86_64 `RDX` to AArch64 `x2`
- x86_64 `RCX` to AArch64 `x3`
- x86_64 `R8` to AArch64 `x4`
- x86_64 `R9` to AArch64 `x5`

The runtime or loader programs a small number of semi-persistent slots, for
example 4 to 8 per thread or address space. `PCALL` then names the target
frontend, target PC, and signature slot, ideally with a small immediate slot
operand in the final encoding. The common case stays fixed-latency: decode the
transition, select the signature, update rename mappings, install the return
cookie, and branch.

The slot should be semi-persistent and cheap to select, not reconstructed at
each call site. A plausible implementation exposes 4 to 8 slots, programmed by
the loader or runtime for common pairs such as SysV-to-AAPCS64,
AAPCS64-to-SysV, SysV-to-RISC-V psABI, and RISC-V psABI-to-SysV. Hot call sites
then encode or supply only the slot number. If a process needs a rare mapping,
runtime code can reprogram a cold slot outside the hot path.

The design point is speed and area efficiency: the hardware only changes
register aliases. It does not execute moves, copy stack slots, reformat memory
layouts, or inspect call descriptors in memory. A realistic OoO implementation
can treat the signature as extra control input to rename/RAT update logic,
which is much closer to ordinary register renaming than to an ABI interpreter.

The signature slot is semi-persistent reconfigurable hardware state, not part of
the dynamic call payload. Runtime code can program a slot once for a common
source/target ABI pair, and many call sites can reuse it. This keeps the hot
transition path small: `PCALL` selects a slot, applies register-name aliases,
installs native return-cookie state, and redirects fetch to the target frontend.
No operand data moves through the integer execution pipes.

The architectural contract for applying a signature must be as strict as a
branch-class operation:

- It performs no user-memory reads or writes.
- It cannot fault because of stack, descriptor, or aggregate layout access.
- It does not allocate temporary architectural registers.
- It only changes frontend mode, PC, return-cookie state, and register aliases.
- Its slot state is explicit poly architectural state, saved/restored by the
  poly state component, not hidden CR3 or emulator bookkeeping.

Architecturally, the mechanism should look like:

- `PABI_SIG_SET slot, kind`: program a small register-only mapping slot.
- `PABI_SIG_GET slot`: report the active slot kind for discovery/debugging.
- `PCALL frontend, target, sig_imm`: branch to another frontend while applying
  the selected cached mapping.

The final encoding can differ, but `PCALL` must name an already-programmed slot
directly. It must not point at a user-memory descriptor that hardware has to
load or parse.

Slot programming should happen at load time, lazy binding time, or runtime
setup time, not on every call. A typical system can reserve one slot for
SysV-to-AAPCS64, one for AAPCS64-to-SysV, one for SysV-to-RISC-V psABI, and one
for the reverse direction. Additional slots can cover hot callbacks or
specialized return-value conventions. If a call site does not match a cached
slot, software can either program a less-used slot before entering a hot loop or
fall back to a thunk.

This mechanism must be strictly limited to registers:

- No hardware parsing of descriptors.
- No user-memory reads by `PCALL`.
- No stack argument repacking.
- No by-value aggregate layout conversion.
- No variadic call handling.
- No PLT/GOT or lazy-binding policy.

Stack arguments, split aggregates, variadics, unusual FP/vector ABI cases, and
all loader policy still go through software thunks. The thunk performs memory
layout work and then uses a null, identity, or simple register signature for the
final frontend transition.

This limit is the silicon boundary. Register renaming is small and predictable
because it reuses machinery an OoO core already needs. Stack and aggregate
translation is not the same class of problem: it needs memory accesses, endian
and layout policy, page-fault recovery, and variable amounts of work. Putting
that into `PCALL` would make frontend switching an ABI interpreter rather than
a fast architectural branch.

This creates a deliberate hybrid boundary:

- Hardware handles the common all-register case, roughly the 90% case for small
  C-style calls, by applying a cached signature with no data movement.
- Software handles the hard 10%: stack layout, by-value aggregates, variadics,
  lazy binding, and unusual vector cases before making the final fixed-latency
  transition.
- The null signature is the exchange-window ABI, so every implementation has a
  simple fallback even without RAT remapping.

The Bochs prototype currently models this with eight signature slots. Slot kind
`0` is the exchange-window mapping, kind `1` is the older x86_64 SysV
compatibility mapping, and kind `2` is the hardware-oriented x86_64 SysV
register-only mapping. Kind `2` is the important silicon-facing form: it maps
the six SysV integer argument registers into the target ABI without stack
access. The final silicon-oriented encoding should use a compact slot
immediate. The current preferred Bochs generic `PCALL` form uses frontend ID in
`R15`, target PC in `RBX`, return PC in `R11`, and an immediate signature-slot
byte. Older register-slot forms remain available for compatibility with
existing probes while the temporary control encoding evolves. `PSWITCH_MODE`
uses the same frontend ID and target registers but does not install a return
cookie.
Foreign generic `PSWITCH` controls use the existing scratch branch registers:
AArch64 `x16=target, x17=frontend ID`; RISC-V `x5=target, x6=frontend ID`.
Foreign generic `PCALL` adds one scratch continuation register: AArch64
`x18=return PC`; RISC-V `x7=return PC`. The callee still returns with its
ordinary native return instruction through the hardware return cookie.

The frontend ID space includes x86_64 as frontend `0`; it should not be a
privileged special case. In the prototype, foreign `PCALL frontend=0` is
accepted for descriptor-backed x86 import targets, so software thunks still own
ABI and loader policy while the CPU control path stays frontend-neutral.

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

- x86_64 poly landing opcode
- AArch64 reserved `HINT`
- RISC-V custom-0 marker

Landing pads give hardware a validation point for indirect cross-ISA control
flow, help catch wrong-frontend targets, and provide a path for CET/BTI-like
hardening without making normal direct calls expensive.

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
