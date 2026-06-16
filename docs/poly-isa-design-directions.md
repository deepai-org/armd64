# Poly ISA Design Directions

Poly is an OS-neutral multi-frontend CPU extension for running existing
precompiled x86_64, AArch64, and RISC-V64 code in one virtual address space.
For commands and current control-op encodings, see `docs/poly-isa.md`.

## Contract

- x86_64 is the system ISA: boot, privilege, paging, faults, interrupts,
  atomics, VM control, and global TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are peer user-mode frontends fetching real native
  instructions from the same address space.
- Frontends keep native fetch rules: x86_64 remains variable length, AArch64 is
  4-byte aligned, and RISC-V supports 16/32-bit instruction fetch for RVC.
- Hardware must not implement Linux, libc, libgcc, libatomic, dynamic-linker
  policy, stack repacking, or user-memory call descriptors.
- Poly state is an explicit 8KB user-owned spill/import image, not hidden
  emulator state and not OS-managed custom xstate.
- Compatibility targets ordinary native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.

## Operations

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64, `3..255` reserved.

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a precise Poly trap. |
| `PLANDING` | Validate an indirect cross-frontend target when enabled. |
| `PSET_EVENT_PTR addr, bytes` | Register the per-thread canonical event frame. |
| `PSET_SPILL_DESC addr, bytes` | Register the per-thread spill image, event frame, resume stack, and x86 monitor trampoline through a versioned descriptor. |
| `PRESTORE buffer` | Import an auto-spilled image before resuming Poly code. |

These are decoded control instructions, not `#UD` envelopes.

## ABI Boundary

Hardware handles only fixed-latency register aliasing, including implicit
structure-return pointer registers when the native ABI represents them as
ordinary register inputs. Software thunks handle stack arguments, by-value
aggregates, variadics, structure-return stack reshaping, lazy binding,
incompatible vectors, and all other memory-shaped ABI work.

The loader/runtime programs a small bank of register alias signature slots. A
hot `PCALL` selects one slot through a fixed control-instruction subopcode. On
an out-of-order CPU this can be implemented in rename by rebinding
architectural names to existing physical registers; no data moves through
execution units and no memory is touched.

Signature slots are architectural `kind | (register_map << 32)` values.
`ABI_SIGNATURE_GET` returns that full encoded value, including the TLS-base map
flag when present. This keeps software, Bochs, RTL, and a future rename-stage
RAT implementation aligned on the exact register-map selected by `PCALL`.

Example x86_64 SysV to AArch64 AAPCS64 slot:

| Source | Target |
| --- | --- |
| `RDI` | `x0` |
| `RSI` | `x1` |
| `RDX` | `x2` |
| `RCX` | `x3` |
| `R8` | `x4` |
| `R9` | `x5` |
| `XMM0..XMM7` | `v0..v7` |

Invalid slots trap before changing frontend or PC. Valid slots cannot fault
because they only rename registers.

The null signature exposes a simple exchange window for low-level thunks:

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

## Returns

Cross-ISA calls return through ordinary native return instructions: x86_64
`ret`, AArch64 `ret x30`, and RISC-V `ret` / `jalr x0, ra, 0`.

`PCALL` pushes caller frontend, PC, SP, and flags to a hardware transition stack
and installs a reserved return cookie in the callee's native return location. A
native return to that cookie pops the transition stack and resumes the caller.
Same-ISA returns stay normal.

## State And Traps

The Poly spill image contains frontend state, interrupted PC, the latest v2
event metadata, hardware transition stack, ABI signature slots, AArch64 GPR/FP/SIMD state,
RISC-V GPR/FP state, per-frontend TLS bases, user monitor addresses, and
landing-pad policy. The OS does not save or restore it; hardware writes it to
user memory before an OS-visible interrupt/fault boundary, and the Ring 3
monitor imports it with `PRESTORE`.

For zero-kernel-change execution, the monitor allocates one aligned 8KB spill
image and one canonical event frame per thread, then registers them with
`PSET_EVENT_PTR` and `PSET_SPILL_DESC` before `PENTER`. If a timer interrupt,
page fault, or other hardware exception arrives while a raw frontend is active,
microcode validates the descriptor generation, exports the full image, writes
the event frame with the current Poly PC and spill reason, switches the
architectural frontend back to x86, replaces the interrupted x86 RIP with the
descriptor-selected trampoline, and only then vectors to the unmodified OS. The
OS sees an ordinary x86 thread. On return or signal delivery, the monitor reads
the event frame and state header; timer exits restore and re-enter Poly, while
page-fault exits are translated into Poly-context exceptions in
userspace.

Hardware emits precise canonical event frames for foreign `svc`/`ecall`,
breakpoints, illegal or unsupported instructions, unresolved imports, and
recoverable frontend exits. Recoverable events may enter a registered Ring 3 Poly monitor;
the monitor owns syscall translation, lazy binding, helper calls, and debugger
policy. The kernel still owns page tables, signal delivery, scheduling,
interrupts, and real syscalls issued by the monitor, but it never owns Poly
register state.

If a monitor vector is enabled, hardware must publish the canonical event frame before redirecting the frontend to the vector PC.
A failed event-frame write or invalid event address prevents the redirect and
reports a precise fault instead. This keeps monitor entry replayable and avoids
hidden side effects before the runtime can inspect the event record.

Trap-vector, event-frame, and descriptor addresses are architectural control
addresses. Non-canonical values, invalid frontend alignment, unaligned event
frames, and event/descriptor ranges that cross the canonical boundary are
rejected by the control instruction or `PRESTORE` before mutating state. The
CPU does not pre-walk or pin event pages; event delivery writes through normal
virtual-memory semantics, so missing permissions or unmapped pages fault at the
event write like a hardware store.

Frontend transition targets and `PCALL` return addresses follow the same rule:
non-canonical control-flow addresses are rejected before changing frontend,
installing return cookies, or pushing hardware transition-stack state.

`PTRAPRET` is a restore operation, not a fallthrough commit. Hardware decodes it
as a precise control operation and stalls retirement until valid monitor-provided
restore frontend/PC state is available. Once available, the restore target is
applied through the same validated frontend/PC state boundary used by commits,
interrupt returns, trap vectors, and return-cookie recovery; the normal
fallthrough PC is not committed for that instruction.

## Priority

1. Keep `PSWITCH` and `PCALL` fixed-latency with no descriptor parsing.
2. Use register-only ABI signatures for fast native-ABI calls.
3. Route complex ABI cases through loader/runtime thunks.
4. Make the auto-spill image and monitor trampoline the only asynchronous
   context-switch contract.
5. Support native return-cookie recovery through the hardware transition stack.
6. Deliver recoverable exits through OS-neutral v2 event frames and a Ring 3
   monitor.
7. Track auto-spill count, spilled bytes, and estimated spill cycles so
   preemption stress tests can measure the cost of the 8KB spill path.
