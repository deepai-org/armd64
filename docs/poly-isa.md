# Poly ISA

Contract for running existing precompiled x86_64, AArch64, and RISC-V userspace
objects in one x86_64 virtual address space. This is native ABI compatibility,
not a new compiler-only ABI. Constants live in `tools/include/polycpuid.h`.
Forward-looking design directions live in `docs/poly-isa-design-directions.md`.

## Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, page tables, interrupts,
  faults, atomics, virtual memory, and TSO memory ordering.
- CPL3 code can switch the frontend into raw AArch64 or raw RISC-V fetch.
- Foreign instructions are fetched directly; there is no per-instruction `#UD`
  envelope.
- Cross-ISA calls preserve real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI.
- Foreign register state is explicit XSAVE-style architectural state.
- Foreign syscalls, breakpoints, illegal instructions, unsupported operations,
  and unresolved imports produce OS-neutral trap records.

## Frontend Transitions

Hardware or FPGA should allocate real decoded x86 opcodes for:

| Operation | Purpose |
| --- | --- |
| `PENTER mode` | Enter a raw frontend from x86_64/system code. |
| `PSWITCH mode, target` | Fixed-latency branch to another frontend. |
| `PCALL mode, target` | Push a hardware transition-stack entry, install a native return cookie, and branch to another frontend. |
| `PIRET` | Restore a previously interrupted frontend after trap handling. |

Every transition ends the current decode block and records precise source and
destination PCs. AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned
so compressed instructions remain valid.

The Bochs prototype models this with a compact decoded x86 control page:

```text
0f 3a fc <subop>
```

`0f 3a fc` is the Poly Control Opcode Page and `<subop>` selects the operation.
This is a normal decoded instruction family: no `#UD` exception path, no magic
trailer, no variable-length envelope, and no following payload bytes. Future
silicon may allocate a different reserved x86 opcode page, but it should keep
the same hardware contract: one deterministic frontend-control decode that
flushes or terminates the current decode block before switching fetch mode.
The control instruction does not parse user-memory call descriptors.

## Foreign Escapes

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `HINT #0x70` / `0xd5032e1f` | exit to x86_64 |
| AArch64 | `HINT #0x71` / `0xd5032e3f` | switch to RISC-V |
| AArch64 | `HINT #0x72` / `0xd5032e5f` | call RISC-V target in `x16`, continuation in `x17` |
| AArch64 | `HINT #0x76` / `0xd5032edf` | trap return |
| RISC-V | custom-0, funct3=7, subop 0 / `0x0000700b` | exit to x86_64 |
| RISC-V | custom-0, funct3=7, subop 1 / `0x0200700b` | switch to AArch64 |
| RISC-V | custom-0, funct3=7, subop 2 / `0x0400700b` | call AArch64 target in `x5`, continuation in `x6` |
| RISC-V | custom-0, funct3=7, subop 6 / `0x0c00700b` | trap return |

These are decoded frontend-control instructions, not breakpoint or undefined
instruction traps. AArch64 `BRK`/RISC-V `EBREAK` remain ordinary trap exits for
debuggers or OS/user trap handling. The RISC-V encoding reserves one fixed
custom-0 funct3 signature and uses funct7 as the control subop, which gives a
simple hardware decode without consuming multiple custom opcode pages.

Native return instructions may cross frontends when the link register or stack
return slot contains a hardware return cookie installed by `PCALL`.

## ABI Bridge

The hardware provides a small integer exchange window for fast argument/result
handoff:

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

Loader/runtime thunks translate native ABI argument order into this window
before issuing `PSWITCH` or `PCALL`. This keeps the CPU transition fixed
latency while still allowing SysV, AAPCS64, and RISC-V psABI compatibility.
FP/vector arguments, stack arguments, aggregate returns, variadics, callbacks,
PLT/GOT lazy binding, and helper imports are software-thunk responsibilities.

Large memory returns follow the callee ABI. AArch64 uses `x8`; RISC-V uses `a0`
and shifts user arguments; x86_64 returns the hidden pointer in `RAX`.

## Traps And Syscalls

Hardware does not emulate Linux, libc, or libgcc. It only produces precise trap
records for foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved imports,
illegal instructions, and unsupported instructions.

A `POLYTRAP` record contains the reason, source mode, selector/immediate, trap
PC, resume PC, all eight POLYTRAP argument lanes, and the first eight native foreign ABI argument registers.
If a per-thread user-space poly monitor is installed, hardware writes the trap
packet to the registered user address and transfers to the monitor in Ring 3.
Otherwise syscall/import traps surface as x86 `#UD`, and breakpoint traps
surface as x86 `#BP`.

Runtime or OS code decides syscall translation, signal delivery, lazy binding,
debugger handling, and failure policy.

## State And Interrupts

Asynchronous events during foreign fetch are precise. Hardware records the
interrupted frontend mode and PC, saves enabled poly state through XSAVE, enters
the normal x86_64 interrupt/fault path, and restores the recorded foreign
frontend on `IRET64`, `SYSRET`, `SYSEXIT`, or signal return when required.

The prototype CPUID contract exposes poly state as XCR0 component `20`.
Component layout version `3` is 4096 bytes and contains the mode header, trap
packet, active transition record, AArch64 GPR/FP state, RISC-V GPR/FP state,
hardware transition-stack state, and user-space monitor registers.

Private CPUID leaves start at `0x40000000` and extend through `0x40000009`. The
prototype software state import layout version is `3`; it is a Bochs fallback,
not the silicon context-switch contract.

## Runtime Boundary

Hardware provides frontend transitions, the exchange window, trap packets,
explicit state, native return cookies, the hardware transition stack, and
optional user-space monitor delivery.

Userspace runtime code provides ELF loading, relocations, PLT/GOT binding,
IFUNC, TLS, dependency search policy, generated thunks, all ABI metadata
parsing, syscall translation for a chosen OS ABI, and libc/libgcc/libatomic
helper semantics.

## Validation

Prefer real boot tests:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

Scripts under `scripts/checks/` are quick consistency smoke tests only.
