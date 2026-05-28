# Poly ISA Design Directions

This note records ISA-level directions for making Poly less like "x86 plus
foreign guests" and more like one CPU with multiple peer frontends. The goal is
compatibility with existing precompiled x86_64, AArch64, and RISC-V code while
keeping the hardware contract OS-neutral and suitable for silicon or FPGA.

## Design Principle

Poly should be a generic multi-frontend CPU extension. x86_64 is the boot and
system frontend, not the semantic center of all execution.

The hardware should accelerate frontend transitions, precise traps, explicit
architectural state, and ABI-defined argument/result movement. It should not
know Linux, libc, dynamic linker policy, or helper-function semantics.

## Generic Frontend IDs

Avoid baking the ISA around pairwise x86-to-AArch64 and x86-to-RISC-V
operations. Define architectural frontend IDs instead:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

The architectural operations should be mode-generic:

- `PENTER mode`: enter a frontend from x86_64/system code.
- `PJMP mode, target`: branch to a target in another frontend.
- `PCALL mode, target, descriptor`: call a target in another frontend.
- `PRET`: return through a hardware transition-return mechanism.
- `PTRAPRET`: resume after a precise poly trap.

The Bochs prototype can continue to encode these through its compact x86
control page, but the contract should describe generic frontend routing.

## Transition Descriptors

Real native ABIs cannot be represented by one fixed register shuffle. Stack
arguments, aggregate returns, variadic calls, vector arguments, callbacks,
PLT/GOT lazy binding, and helper imports all need metadata.

Use a user-memory descriptor for non-trivial calls:

```c
struct PolyCallDesc {
  uint16_t src_abi;
  uint16_t dst_abi;
  uint16_t ret_kind;
  uint16_t flags;
  uint32_t int_arg_mask;
  uint32_t fp_arg_mask;
  uint32_t stack_arg_bytes;
  uint32_t reserved;
  uint64_t target_pc;
  uint64_t continuation_pc;
  uint64_t resolver_pc;
};
```

Hardware should keep a direct fast path for common fixed-shape calls and use
the descriptor path for complex ABI cases. Runtime code owns descriptor
construction and policy.

## Native Cross-ISA Returns

Special poly-only return instructions should not be required for compatibility
with ordinary precompiled code. Native returns should be able to cross
frontends:

- x86_64 `ret`
- AArch64 `ret x30`
- RISC-V `ret` / `jalr x0, ra, 0`

The target can be represented by a hardware return cookie or protected
transition-return stack. This keeps normal compiler epilogues usable and avoids
requiring every foreign object to be rebuilt for a custom ABI.

## Register State

Do not globally alias all frontend registers. x86_64, AArch64, and RISC-V have
different register counts, caller/callee-save rules, FP/vector layouts, and
special registers.

The cleaner contract is:

- Each frontend has its own real architectural register file.
- `PCALL` performs ABI-defined argument/result movement.
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

Runtime or OS code decides syscall translation, signal delivery, lazy binding,
debugger behavior, helper-call policy, and failure handling.

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
- active transition record
- AArch64 GPR state
- AArch64 FP/SIMD state
- RISC-V GPR state
- RISC-V FP state
- transition-return state
- per-frontend TLS bases

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
2. Descriptor-based `PCALL` for real ABI interop.
3. Native return-cookie support.
4. XSAVE poly state as the only context-switch contract.
5. Trap packets as the only hardware syscall/libcall interface.

These changes make Poly more sympathetic to precompiled code: ordinary
functions keep ordinary ABI behavior, while hardware accelerates the crossing
points rather than hiding ABI mismatches in emulator-only state.
