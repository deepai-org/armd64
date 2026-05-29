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
| `PENTER frontend` | Enter a raw frontend from x86_64/system code. |
| `PSWITCH frontend, target` | Fixed-latency branch to another frontend. |
| `PCALL frontend, target[, sig_imm]` | Apply an optional cached register-only ABI signature, push a hardware transition-stack entry, install a native return cookie, and branch to another frontend. |
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
trailer, and no variable-length envelope. Future
silicon may allocate a different reserved x86 opcode page, but it should keep
the same hardware contract: one deterministic frontend-control decode that
flushes or terminates the current decode block before switching fetch mode.
The control instruction does not parse user-memory call descriptors or rewrite
stack layouts.

Silicon-oriented `PCALL` encodings name the signature slot with a small
immediate. The Bochs prototype keeps the older register-slot forms for test
coverage, but the preferred generic form is `PCALL_SIG_IMM_MODE`.

| Subop | Operation | Register convention |
| --- | --- | --- |
| `0x2b` | `PCALL_SIG_A64` | target in `RBX`, return PC in `R11`, signature slot in `R12` |
| `0x2c` | `PCALL_SIG_RV64` | target in `RBX`, return PC in `R11`, signature slot in `R12` |
| `0x2d` | `PCALL_SIG_MODE` | frontend ID in `R15`, target in `RBX`, return PC in `R11`, signature slot in `R12` |
| `0x2e <slot>` | `PCALL_SIG_IMM_MODE` | frontend ID in `R15`, target in `RBX`, return PC in `R11`, signature slot as immediate byte |
| `0x69` | `ABI_SIGNATURE_SET` | `RAX=slot`, `RDX=kind`, returns `RAX=0` or `-EINVAL` |
| `0x6a` | `ABI_SIGNATURE_GET` | `RAX=slot`, returns signature kind in `RAX` or `-EINVAL` |

Prototype signature kinds are `0` for the baseline exchange window, `1` for the
older x86_64 SysV compatibility mapping, and `2` for the hardware-oriented
x86_64 SysV register-only mapping. Fast `PCALL_SIG_*` code should use kind `2`
when it wants RAT-style behavior: `RDI,RSI,RDX,RCX,R8,R9` are rebound to the
target argument registers and stack arguments are left to software thunks.
These kinds are a model of cached hardware control state, not a final x86
opcode allocation.

The prototype also exposes `0x03` as `PENTER_MODE`, with the frontend ID in
`R15`. This is the generic frontend-ID form of the older fixed AArch64/RISC-V
enter controls.

`0x04` is `PSWITCH_MODE`: frontend ID in `R15`, target PC in `RBX`. It is a
non-call branch and does not install a return cookie.

CPUID leaf `0x40000002`, subleaf `6` reports foreign generic frontend controls:
`EAX=AArch64 PSWITCH`, `EBX=RISC-V PSWITCH`, `ECX=AArch64 PCALL`, and
`EDX=RISC-V PCALL`. CPUID leaf `0x40000008`, subleaf `1` reports the
architectural frontend IDs: `EAX=x86_64`, `EBX=AArch64`, `ECX=RISC-V64`, and
`EDX` as the supported frontend-ID bitmask.
CPUID leaf `0x40000002`, subleaf `7` reports the preferred x86 immediate-slot
generic `PCALL` subop in `EAX` and the ABI signature-slot count in `EBX`.
CPUID leaf `0x40000002`, subleaf `8` reports foreign signature `PCALL`
controls: `EAX=AArch64 PCALL_SIG`, `EBX=RISC-V PCALL_SIG`, and `ECX` as the
ABI signature-slot count.

## Foreign Escapes

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `HINT #0x70` / `0xd5032e1f` | exit to x86_64 |
| AArch64 | `HINT #0x71` / `0xd5032e3f` | switch to RISC-V |
| AArch64 | `HINT #0x72` / `0xd5032e5f` | call RISC-V target in `x16`, continuation in `x17` |
| AArch64 | `HINT #0x76` / `0xd5032edf` | trap return |
| AArch64 | `HINT #0x78` / `0xd5032f1f` | `PSWITCH`: target in `x16`, frontend ID in `x17` |
| AArch64 | `HINT #0x79` / `0xd5032f3f` | `PCALL`: target in `x16`, frontend ID in `x17`, continuation in `x18` |
| AArch64 | `HINT #0x7a` / `0xd5032f5f` | `PCALL_SIG`: target in `x16`, frontend ID in `x17`, continuation in `x18`, signature slot in `x19` |
| RISC-V | custom-0, funct3=7, subop 0 / `0x0000700b` | exit to x86_64 |
| RISC-V | custom-0, funct3=7, subop 1 / `0x0200700b` | switch to AArch64 |
| RISC-V | custom-0, funct3=7, subop 2 / `0x0400700b` | call AArch64 target in `x5`, continuation in `x6` |
| RISC-V | custom-0, funct3=7, subop 6 / `0x0c00700b` | trap return |
| RISC-V | custom-0, funct3=7, subop 8 / `0x1000700b` | `PSWITCH`: target in `x5`, frontend ID in `x6` |
| RISC-V | custom-0, funct3=7, subop 9 / `0x1200700b` | `PCALL`: target in `x5`, frontend ID in `x6`, continuation in `x7` |
| RISC-V | custom-0, funct3=7, subop 10 / `0x1400700b` | `PCALL_SIG`: target in `x5`, frontend ID in `x6`, continuation in `x7`, signature slot in `x28` |

These are decoded frontend-control instructions, not breakpoint or undefined
instruction traps. AArch64 `BRK`/RISC-V `EBREAK` remain ordinary trap exits for
debuggers or OS/user trap handling. The RISC-V encoding reserves one fixed
custom-0 funct3 signature and uses funct7 as the control subop, which gives a
simple hardware decode without consuming multiple custom opcode pages.

Native return instructions may cross frontends when the link register or stack
return slot contains a hardware return cookie installed by `PCALL`.

Foreign generic `PCALL` can name x86_64 as frontend `0`. In the Bochs
prototype, descriptor-backed imports still use the reserved import-call range,
but direct x86 targets are also supported. `PCALL_SIG` selects a cached
register-only ABI signature slot for direct targets, so a foreign caller can
enter an ordinary x86 SysV function without executing register-move thunks when
the call fits in registers. Hardware installs a return cookie on the x86 stack;
an ordinary x86 `ret` to that cookie restores the foreign frontend and resumes
at the foreign continuation register. Loader/runtime thunks still own complex
ABI policy, but the control transfer itself now uses the same frontend-neutral
`PCALL` path as AArch64-to-RISC-V and RISC-V-to-AArch64.

## ABI Bridge

The hardware provides a baseline integer exchange window for fast
argument/result handoff:

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

Loader/runtime thunks can translate native ABI argument order into this window
before issuing `PSWITCH` or `PCALL`. Silicon may also expose a small set of
programmable ABI signature slots. A signature is register-only: it lets `PCALL`
remap source architectural registers onto destination architectural registers
through rename/RAT state, without moving data and without reading memory.

These slots are semi-persistent hardware control state, typically programmed by
the loader or runtime and reused by many call sites. A final `PCALL` names the
target frontend, target PC, and signature slot, preferably as an immediate; the
CPU applies the cached register map in the rename path instead of executing
move instructions.

This is a programmable RAT mechanism, not a descriptor parser. The hot
transition path selects an already-programmed slot; it does not fetch a call
descriptor from user memory. Register-only native ABI calls can therefore avoid
software move thunks, while complex calls still use loader/runtime thunks.

The intended use is semi-persistent signature caching. A runtime can program a
small slot set once for common ABI pairs, then let many `PCALL` sites name only
the slot. This keeps the crossing close to a branch plus rename-table update:
the CPU changes which target architectural names reference existing physical
registers, instead of moving operand data through execution pipes.

The intended silicon implementation is a programmable register-alias-table
update, not a data-moving microcode loop. Slot programming is rare control
state setup. The hot `PCALL` path selects a cached slot, rebinds architectural
source names to destination names in rename/RAT state, installs the return
cookie, and branches.

The slot bank is the hardware-visible Poly ABI Signature Register model. A
runtime can program, for example, one slot for SysV-to-AAPCS64 and another for
AAPCS64-to-SysV, then many call sites reuse those slots by immediate number.
This is semi-persistent reconfigurable hardware state, not a per-call memory
descriptor.

Applying a signature is a fixed-latency control operation. It does not read a
descriptor, touch the user stack, allocate temporary architectural registers, or
perform ABI memory conversion. Invalid slot use should trap before changing
frontends; valid slot use should be no more complex than selecting cached
rename mappings plus the normal transition-stack update.

This is intentionally limited to register renaming. Hardware does not repack
stack arguments, copy by-value structs, interpret variadic call layouts, or read
ABI descriptors from user memory. Those cases remain software-thunk
responsibilities. The result is a small silicon mechanism that makes ordinary
all-register calls fast while keeping complex ABI policy outside the CPU
pipeline.

The intended split is simple: hardware handles all-register calls with cached
RAT remaps; software thunks handle stack and aggregate ABI conversion, then use
a null, identity, or simple signature for the final `PCALL`.

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

Hardware provides frontend transitions, the exchange window, optional
register-only ABI signature slots, trap packets, explicit state, native return
cookies, the hardware transition stack, and optional user-space monitor
delivery.

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
