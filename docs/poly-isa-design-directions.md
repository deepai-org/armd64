# Poly ISA Design Directions

This document records the ISA direction for making Poly a hardware-plausible,
OS-neutral multi-frontend CPU extension. The goal is compatibility with
existing precompiled x86_64, AArch64, and RISC-V64 code, including cross-ISA
library calls.

For the short reference, see `docs/poly-isa.md`.

## Non-Negotiable Contract

- x86_64 is the system frontend for boot, privilege, paging, interrupts, and
  the global memory model.
- AArch64 and RISC-V64 are peer user-mode frontends, not high-level emulated
  coprocessors.
- Foreign code executes real frontend instructions from the shared virtual
  address space.
- The hardware contract is OS-neutral: no Linux syscall policy, libc helpers,
  dynamic-linker descriptors, or hidden emulator state.
- Compatibility targets existing native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.

## Frontend Model

Frontend IDs are architectural and generic:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |
| `3..255` | Reserved |

The control operations should be fixed-latency and frontend-neutral:

- `PENTER frontend`: enter a frontend from system/runtime code.
- `PSWITCH frontend, target`: branch to another frontend without a return.
- `PCALL frontend, target, sig`: call another frontend and select an ABI
  signature slot.
- `PTRAPRET`: resume after a precise poly trap.

The Bochs prototype can keep temporary compact encodings, but the architecture
should be described in terms of these generic operations.

## ABI Boundary

Hardware must not parse user-memory call descriptors. That path creates
variable-latency control flow, page-fault points during call execution, and ABI
policy in the CPU pipeline.

The split is:

| Case | Mechanism |
| --- | --- |
| Integer, FP, and compatible fixed SIMD arguments already in native ABI registers | `PCALL ... sig` applies a cached register-only signature. |
| Stack arguments, by-value aggregates, variadics, hidden structure returns, lazy binding, incompatible vectors | Loader/runtime thunk performs memory-side ABI work, then uses `PCALL`. |

This preserves compatibility with ordinary precompiled objects while keeping
the hardware transition path small.

## ABI Signature Slots

The only intended reconfigurable ABI hardware is register-only aliasing.

A loader/runtime programs a small bank of ABI signature slots. A hot `PCALL`
selects one slot with an immediate or compact operand. On an out-of-order CPU,
the rename stage applies the slot by rebinding architectural names to physical
registers already holding the source ABI values. No execution unit moves the
data, and no memory is touched.

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

Invalid or unsupported slots trap before changing frontend mode or PC. Valid
slots cannot fault because they do not read memory.

## Baseline Exchange Window

The baseline/null signature is a small integer exchange window that all
frontends can use:

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

This is not a replacement ABI. It is the low-level handoff window used by
thunks and simple signatures. Native-register signatures remain preferred for
direct precompiled-code calls.

## Returns

Cross-ISA calls should return through ordinary native return instructions:

- x86_64 `ret`
- AArch64 `ret x30`
- RISC-V `ret` / `jalr x0, ra, 0`

`PCALL` pushes caller frontend, PC, SP, and flags to a hardware transition
stack, then installs a reserved return cookie in the callee's native return
location. A native return to that cookie pops the transition stack and resumes
the caller frontend. Same-ISA returns remain normal.

## Architectural State

Poly state must be explicit XSAVE-style architectural state. It must not be a
hidden CR3-keyed or TLS-keyed emulator bank.

The Poly state component should include:

- active frontend and interrupted frontend
- interrupted PC and trap packet
- hardware transition stack state
- ABI signature slots
- AArch64 GPR and FP/SIMD state
- RISC-V GPR and FP state
- per-frontend TLS bases
- user-space poly monitor PC and trap-packet address
- landing-pad policy bits

The OS only needs to save and restore the component. It does not need to know
AArch64 or RISC-V register semantics.

## Trap Delivery

Hardware emits precise trap packets. It does not emulate syscalls, libcalls,
libgcc, libatomic, or dynamic-linker policy.

Trap-producing events include:

- foreign `svc` / `ecall`
- breakpoints
- illegal or unsupported instructions
- unresolved imports
- page faults during foreign execution
- asynchronous interrupts during foreign execution

Recoverable user events may enter a registered Ring 3 poly monitor with a
trap packet in user memory. The monitor owns syscall translation, lazy binding,
helper calls, and debugger policy. The kernel remains responsible for hard
faults, signals, scheduling, and real syscalls issued by the monitor.

## Memory Model

All frontends share x86-style TSO. This is stronger than native AArch64 and
RISC-V ordering, so correctly synchronized foreign code remains correct.
AArch64 barriers and RISC-V fences can be implemented as cheap ordering points
or no-ops when TSO already satisfies the required ordering.

## Landing Pads

Cross-frontend indirect targets should optionally begin with frontend-specific
landing pads:

- x86_64 poly landing opcode: prototype subop `0x05`
- AArch64 reserved `HINT`: prototype `HINT #0x7b`
- RISC-V custom-0 marker: prototype subop `11`

Landing pads give hardware a validation point for wrong-frontend targets and
CET/BTI-like hardening. The policy is explicit Poly state, not hidden emulator
state.

## Prototype Notes

- Bochs currently models signature slots and frontend controls with temporary
  encodings.
- Bochs-only state-key controls are diagnostics, not architecture.
- The reserved import-call range is a trap surface for unresolved imports, not
  a CPU-parsed descriptor ABI.
- Temporary encodings should keep evolving toward dedicated, silicon-suitable
  control opcodes.

## Implementation Priority

1. Keep generic frontend IDs as the main ISA abstraction.
2. Keep `PSWITCH` and `PCALL` fixed-latency with no descriptor parsing.
3. Use register-only ABI signatures for fast native-ABI calls.
4. Route complex ABI cases through loader/runtime thunks.
5. Make XSAVE-style Poly state the only context-switch contract.
6. Support native return-cookie recovery through the hardware transition stack.
7. Deliver recoverable traps through OS-neutral trap packets and a Ring 3
   monitor.
