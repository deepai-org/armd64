# Poly ISA

Concise contract for running existing precompiled x86_64, AArch64, and
RISC-V64 userspace objects in one x86_64 virtual address space. This is native
ABI compatibility, not a new compiler-only ABI. Constants are defined in
`tools/include/polycpuid.h`; design rationale is in
`docs/poly-isa-design-directions.md`.

## What Changes From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, atomics, virtual memory, and TSO memory ordering.
- CPL3 code can enter raw AArch64 or raw RISC-V fetch.
- Foreign instructions are fetched directly. There is no per-instruction `#UD`
  envelope.
- Cross-ISA calls target real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Foreign register state is explicit XSAVE-style architectural state.
- Foreign syscalls, breakpoints, illegal instructions, unsupported operations,
  and unresolved imports produce OS-neutral trap records.
- ABI signature slots are register-only rename/RAT templates. Stack arguments,
  aggregates, variadics, lazy binding, and incompatible vector layouts remain
  loader/runtime thunk work.

## Frontend Controls

Hardware should allocate real decoded x86 opcodes for these operations:

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a raw frontend from x86_64/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without installing a return cookie. |
| `PCALL frontend, target[, sig_imm]` | Apply an optional register-only signature slot, push transition-stack state, install a native return cookie, and branch to another frontend. |
| `PIRET` | Restore a previously interrupted frontend after trap handling. |

Every transition ends the current decode block and records precise source and
destination PCs. AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte
aligned so compressed instructions remain valid.

The Bochs prototype models the x86 controls with:

```text
0f 3a fc <subop>
```

This is a decoded control page, not an exception path. A future silicon opcode
allocation can differ, but it should preserve the same contract: fixed decode,
frontend redirect, no user-memory descriptor parsing, and no stack rewriting.

Important x86 prototype subops:

| Subop | Operation | Operands |
| --- | --- | --- |
| `0x03` | `PENTER_MODE` | `R15=frontend` |
| `0x04` | `PSWITCH_MODE` | `R15=frontend`, `RBX=target` |
| `0x05` | `PLANDING` | landing marker, no-op unless policy requires it |
| `0x2d` | `PCALL_SIG_MODE` | `R15=frontend`, `RBX=target`, `R11=return`, `R12=slot` |
| `0x2e <slot>` | `PCALL_SIG_IMM_MODE` | `R15=frontend`, `RBX=target`, `R11=return`, slot in encoding |
| `0x69` | `ABI_SIGNATURE_SET` | `RAX=slot`, `RDX=kind`; returns `RAX=0` or `-EINVAL` |
| `0x6a` | `ABI_SIGNATURE_GET` | `RAX=slot`; returns kind or `-EINVAL` |
| `0x6b` | `MONITOR_PACKET_SET` | `RAX=user pointer`, `0` disables packet writes |
| `0x6c` | `MONITOR_PACKET_GET` | returns packet pointer in `RAX` |
| `0x6d` | `LANDING_POLICY_SET` | `RAX=flags`; returns `RAX=0` or `-EINVAL` |
| `0x6e` | `LANDING_POLICY_GET` | returns active landing-policy flags |

Frontend IDs:

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

## Foreign Controls

AArch64 controls use reserved `HINT` encodings:

| Encoding | Meaning |
| --- | --- |
| `0xd5032e1f` | exit to x86_64 |
| `0xd5032e3f` | switch to RISC-V |
| `0xd5032edf` | trap return |
| `0xd5032f1f` | `PSWITCH`: `x16=target`, `x17=frontend` |
| `0xd5032f3f` | `PCALL`: `x16=target`, `x17=frontend`, `x18=return` |
| `0xd5032c1f + (slot << 5)` | `PCALL_SIG_IMM`, slots `0..7` |
| `0xd5032f5f` | `PCALL_SIG`: slot in `x19` |
| `0xd5032f7f` | landing marker |
| `0xd5032f9f` / `0xd5032fbf` | ABI signature set/get |
| `0xd5032d1f..0xd5032dbf` | trap-vector and monitor-packet controls |
| `0xd5032fdf..0xd5032fff` | landing-policy set/get |

RISC-V controls use custom-0, `funct3=7`, with the subop in `funct7`:

| Encoding | Meaning |
| --- | --- |
| `0x0000700b` | exit to x86_64 |
| `0x0200700b` | switch to AArch64 |
| `0x0c00700b` | trap return |
| `0x1000700b` | `PSWITCH`: `x5=target`, `x6=frontend` |
| `0x1200700b` | `PCALL`: `x5=target`, `x6=frontend`, `x7=return` |
| `0x2000700b + (slot << 25)` | `PCALL_SIG_IMM`, slots `0..7` |
| `0x1400700b` | `PCALL_SIG`: slot in `x28` |
| `0x1600700b` | landing marker |
| `0x1800700b` / `0x1a00700b` | ABI signature set/get |
| `0x3000700b..0x3a00700b` | trap-vector and monitor-packet controls |
| `0x3c00700b..0x3e00700b` | landing-policy set/get |

AArch64 `BRK` and RISC-V `EBREAK` remain ordinary trap exits for debugger or
runtime policy. Native return instructions may cross frontends when the link
register or x86 stack return slot contains a hardware return cookie installed
by `PCALL`.

## ABI Bridge

The baseline exchange window is:

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

Signature slots are semi-persistent register-renaming templates selected by
`PCALL ... sig_imm`. They can rebind compatible integer, FP, and fixed SIMD ABI
lanes without executing move instructions or touching memory. They must not
describe stack layouts, by-value aggregates, variadic metadata, PLT/GOT policy,
or lazy binding.

Prototype slot/kind defaults:

| Slot | Kind | Purpose |
| --- | --- | --- |
| `0` | `EXCHANGE` | baseline exchange window |
| `1` | `X86_SYSV_REGS` | x86 SysV register-only mapping |
| `2` | `X86_SYSV_REGS_I128` | x86 SysV register-only mapping with two-GPR integer return |
| `3` | `NATIVE_REGS` | preferred neutral native-register mapping |
| `4` | `NATIVE_REGS_I128` | preferred neutral native-register mapping with two-GPR integer return |

The runtime/loader programs or verifies the slot bank. Register-only hot calls
use a direct signature `PCALL`; calls requiring memory-side ABI work use a
software thunk and finish with a null, identity, or simple signature `PCALL`.

The first eight scalar FP lanes are also bridged directly:
`XMM0..XMM7`, AArch64 `v0..v7`, and RISC-V `fa0..fa7`. Fixed 128-bit vector
calls can stay direct when both ABIs classify them in compatible vector
registers. Wider AVX, SVE, RVV, stack FP overflow, and incompatible aggregate
or vector layouts remain thunk policy.

Large memory returns follow the callee ABI. AArch64 uses `x8`; RISC-V uses
`a0` and shifts user arguments; x86_64 returns the hidden pointer in `RAX`.

## Traps And Syscalls

Hardware does not emulate Linux, libc, libgcc, or libatomic. It only produces
precise trap records for foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved
imports, illegal instructions, and unsupported instructions.

A `POLYTRAP` record contains the reason, source mode, selector/immediate, trap
PC, resume PC, eight generic argument lanes, and the first eight native foreign
ABI argument registers. If a per-thread user-space monitor is installed,
hardware writes the packet to that address and transfers to the Ring 3 monitor.
Otherwise syscall/import traps surface as x86 `#UD`; breakpoint traps surface
as x86 `#BP`.

Runtime or OS code decides syscall translation, signal delivery, debugger
handling, lazy binding, and failure policy.

## State And Interrupts

Asynchronous events during foreign fetch are precise. Hardware records the
interrupted frontend mode and PC, saves enabled poly state through XSAVE, enters
the normal x86_64 interrupt/fault path, and restores the recorded foreign
frontend on `IRET64`, `SYSRET`, `SYSEXIT`, or signal return when required.

The prototype exposes Poly state as XCR0 component `20`. Layout version `8` is
4096 bytes and contains:

- mode header and trap packet
- active transition and hardware transition-stack state
- AArch64 GPR/FP state
- RISC-V GPR/FP state
- user-space monitor registers
- ABI signature slot bank
- AArch64 `TPIDR_EL0` and RISC-V `tp/x4`
- landing-policy flags

Private CPUID leaves start at `0x40000000` and currently extend through
`0x40000009`. The software import-state layout is a Bochs fallback; XSAVE is
the silicon context-switch contract.

## CPUID Summary

| Leaf/subleaf | Contents |
| --- | --- |
| `0x40000002/6` | foreign generic frontend controls |
| `0x40000002/7` | x86 `PCALL_SIG_IMM_MODE`, slot count, hot slot manifest |
| `0x40000002/8` | foreign signature `PCALL` controls |
| `0x40000002/10` | foreign ABI signature set/get controls |
| `0x40000002/11` | foreign immediate-slot signature-call encodings |
| `0x40000002/12..14` | trap-vector, monitor-packet, and mode controls |
| `0x40000002/17` | preferred native two-GPR return slot and kind |
| `0x40000008/1` | frontend IDs and supported frontend bitmask |
| `0x40000009/0` | ABI bridge capabilities |

## Runtime Boundary

Hardware provides frontend transitions, the exchange window, register-only ABI
signature slots, trap packets, explicit XSAVE state, native return cookies, the
hardware transition stack, and optional user-space monitor delivery.

Userspace runtime provides ELF loading, relocations, PLT/GOT binding, IFUNC,
TLS, dependency search policy, generated thunks, ABI metadata parsing, syscall
translation for a chosen OS ABI, and libc/libgcc/libatomic helper semantics.

## Validation

Prefer real boot tests:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

Scripts under `scripts/checks/` are quick consistency smoke tests only.
