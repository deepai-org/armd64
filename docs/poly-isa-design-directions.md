# Poly ISA Design Directions

Poly is an OS-neutral multi-frontend CPU extension for running existing
precompiled x86_64, AArch64, and RISC-V64 code in one virtual address space.
For commands and prototype encodings, see `docs/poly-isa.md`.

## Contract

- x86_64 is the system ISA: boot, privilege, paging, faults, interrupts,
  atomics, VM control, and global TSO memory ordering stay x86-owned.
- AArch64 and RISC-V64 are peer user-mode frontends that fetch real aligned
  32-bit instructions from the same address space.
- Hardware must not implement Linux, libc, libgcc, libatomic, dynamic-linker
  policy, stack repacking, or user-memory call descriptors.
- Poly state is explicit XSAVE-style architectural state, not hidden
  CR3/TLS-keyed emulator state.
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

These are decoded control instructions, not `#UD` envelopes.

## ABI Boundary

Hardware handles the fixed-latency part only: register aliasing at a mode
switch. Software thunks handle every memory-shaped ABI problem.

| Case | Mechanism |
| --- | --- |
| Integer, FP, and compatible fixed SIMD args already in native ABI registers | `PCALL ... sig` applies a cached register alias signature. |
| Stack args, by-value aggregates, variadics, hidden structure returns, lazy binding, incompatible vectors | Loader/runtime thunk marshals memory state, then uses `PCALL`. |

## Register Alias Signatures

The loader/runtime programs a small bank of signature slots. A hot `PCALL`
selects one slot. On an out-of-order CPU this can be implemented in the rename
stage by rebinding architectural names to existing physical registers. No data
moves through execution units, and no memory is touched.

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

The null signature exposes a simple exchange window for thunks:

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

Native ABI signatures are preferred for direct precompiled-code calls; the
exchange window is a low-level handoff path, not a replacement ABI.

## Returns

Cross-ISA calls return through ordinary native return instructions: x86_64
`ret`, AArch64 `ret x30`, and RISC-V `ret` / `jalr x0, ra, 0`.

`PCALL` pushes caller frontend, PC, SP, and flags to a hardware transition stack
and installs a reserved return cookie in the callee's native return location. A
native return to that cookie pops the transition stack and resumes the caller.
Same-ISA returns stay normal.

## State And Traps

The XSAVE-style Poly state component contains frontend state, interrupted PC,
trap packet, hardware transition stack, ABI signature slots, AArch64 GPR/FP/SIMD
state, RISC-V GPR/FP state, per-frontend TLS bases, user monitor addresses, and
landing-pad policy. The OS saves/restores the component without knowing foreign
register semantics.

Hardware emits precise trap packets for foreign `svc`/`ecall`, breakpoints,
illegal or unsupported instructions, unresolved imports, and recoverable
frontend exits. Recoverable events may enter a registered Ring 3 Poly monitor;
the monitor owns syscall translation, lazy binding, helper calls, and debugger
policy. The kernel still owns hard page faults, signals, scheduling,
interrupts, and real syscalls issued by the monitor.

## Priority

1. Keep `PSWITCH` and `PCALL` fixed-latency with no descriptor parsing.
2. Use register-only ABI signatures for fast native-ABI calls.
3. Route complex ABI cases through loader/runtime thunks.
4. Make XSAVE-style Poly state the only context-switch contract.
5. Support native return-cookie recovery through the hardware transition stack.
6. Deliver recoverable exits through OS-neutral trap packets and a Ring 3
   monitor.
