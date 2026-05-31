# Poly ISA

Poly is an x86_64 CPU extension for running precompiled AArch64 and RISC-V64
user code in one process and one virtual address space. The compatibility
targets are normal SysV x86_64, AAPCS64, and RISC-V psABI objects. Poly is not
a new compiler-only ABI.

## Model

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, VM control, atomics, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode instruction frontends over the same
  x86_64-owned virtual machine.
- Foreign code is fetched directly as native instructions. There are no
  per-instruction `#UD` envelopes.
- AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned so RVC can be
  supported.
- Non-x86 architectural state is explicit per-thread XSAVE-style state.

Frontend IDs:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

## Control Operations

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the current PC. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a user-mode Poly trap monitor. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing point. |

These are decoded instructions, not exception tricks. Same-ISA code keeps using
normal native branches and calls.

## ABI Boundary

Hardware only performs fixed-latency frontend switching and register aliasing.
The loader/runtime owns everything that touches memory layout: stack arguments,
by-value aggregates, variadics, hidden structure returns, lazy binding,
dynamic-linker policy, syscall translation, and libcalls.

Hot `PCALL` instructions select a small ABI signature slot. A hardware
implementation can apply the slot by register renaming/RAT updates rather than
moving data through execution units. Invalid slots trap before changing PC or
frontend.

The null exchange window is for low-level thunks:

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

Native ABI register-only calls use programmed signature slots instead of this
fixed window when the source and target ABIs differ.

## Returns, State, And Traps

Cross-ISA calls return through ordinary native return instructions:
x86_64 `ret`, AArch64 `ret x30`, and RISC-V `ret` / `jalr x0, ra, 0`.
`PCALL` records caller frontend, PC, SP, and flags in Poly XSAVE state and
installs a reserved return cookie in the callee return location.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unsupported
instructions, unresolved imports, and recoverable exits produce OS-neutral trap
packets. A Ring 3 Poly monitor may handle these packets. The kernel still owns
hard page faults, interrupts, scheduling, signals, and real syscalls issued by
the monitor.

## Prototype Encodings

The Bochs prototype uses temporary encodings chosen to model real decoded
hardware control instructions:

| Frontend | Encoding family |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` Poly Control Opcode Page |
| AArch64 | reserved `HINT` subspace, `0xd503201f | (subop << 5)` |
| RISC-V64 | `custom-0`, `0x0000700b | (subop << 25)` |

Hot x86_64 `PCALL` signature slots use fixed subopcodes
`0f 3a fc 30..39`. AArch64 and RISC-V64 have matching immediate-slot control
forms so hot calls do not need a temporary register move.

Hardware rationale and open design direction live in
`docs/poly-isa-design-directions.md`.
