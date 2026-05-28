# Poly ISA Design Directions

This note records ISA-level directions for making Poly less like "x86 plus
foreign guests" and more like one CPU with multiple peer frontends. The goal is
compatibility with existing precompiled x86_64, AArch64, and RISC-V code while
keeping the hardware contract OS-neutral and suitable for silicon or FPGA.

## Design Principle

Poly should be a generic multi-frontend CPU extension. x86_64 is the boot and
system frontend, not the semantic center of all execution.

The hardware should accelerate frontend transitions, precise traps, explicit
architectural state, a small register exchange window, and native return
recovery. It should not parse dynamic-linker descriptors or know Linux, libc,
dynamic linker policy, or helper-function semantics.

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

- `PENTER mode`: enter a frontend from x86_64/system code.
- `PSWITCH mode, target`: branch to a target in another frontend.
- `PCALL mode, target`: push a hardware transition-stack entry, install a
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

## Register Exchange Window

Fully separate register files are clean but force every cross-ISA call to spill
through memory. Full global aliasing is also wrong because each ISA has
different register counts and preservation rules.

Define a small physical exchange window shared by the common integer
argument/result lanes:

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

Software thunks translate native ABI argument order into this window before the
mode switch. Simple calls then cross without memory marshalling. Non-window
GPRs, FP/SIMD registers, vector state, and special registers remain separate
architectural state preserved through the poly XSAVE component.

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
3. A small integer register exchange window.
4. XSAVE poly state as the only context-switch contract.
5. Native return-cookie support through the HTS.
6. User-space poly monitor delivery for recoverable traps.

These changes make Poly more sympathetic to precompiled code: ordinary
functions keep ordinary ABI behavior, while hardware accelerates the crossing
points and software thunks handle ABI complexity outside the CPU pipeline.
