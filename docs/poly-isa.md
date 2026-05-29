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
- Programmable ABI signature slots are register-only RAT mappings; stack and
  aggregate ABI translation remains software thunk work.

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
| `0x05` | `PLANDING` | x86_64 landing-pad marker; no-op unless landing policy requires it |
| `0x69` | `ABI_SIGNATURE_SET` | `RAX=slot`, `RDX=kind`, returns `RAX=0` or `-EINVAL` |
| `0x6a` | `ABI_SIGNATURE_GET` | `RAX=slot`, returns signature kind in `RAX` or `-EINVAL` |
| `0x6b` | `MONITOR_PACKET_SET` | `RAX=user pointer` for the monitor trap-packet buffer, `0` disables memory packet writes |
| `0x6c` | `MONITOR_PACKET_GET` | returns the active monitor trap-packet buffer pointer in `RAX` |
| `0x6d` | `LANDING_POLICY_SET` | `RAX=policy flags`, returns `RAX=0` or `-EINVAL` |
| `0x6e` | `LANDING_POLICY_GET` | returns active landing-policy flags in `RAX` |

Prototype signature kinds are `0` for the baseline exchange window, `1` for the
older stack-capable x86_64 SysV compatibility mapping, `2` for the older
x86_64 SysV register-only mapping, `3` for the same register argument mapping
with a two-register integer return (`RAX/RDX` to the destination ABI's first
two integer return registers), and `4` for the preferred neutral native-ABI
register mapping. Prototype kind `5` is the neutral native-register mapping
with a two-GPR integer return. Fast `PCALL_SIG_*` code should use kind `4` for
ordinary register-only calls: the source frontend's native integer/FP argument
lanes are rebound to the target frontend's native integer/FP argument lanes,
and stack arguments are left to software thunks. Kind `4` does not implicitly
request a multi-GPR return import; calls with `unsigned __int128`-class returns
use an explicit return-shape signature such as kind `5`. Kinds `2` and `3`
remain valid prototype aliases for existing x86-oriented tests and direct x86
imports. These kinds are a model of cached hardware control state, not a final
x86 opcode allocation.

Signature slots are semi-persistent register-renaming templates. A real CPU
should apply them by updating RAT mappings during the `PCALL` control redirect,
not by executing move instructions or reading an ABI descriptor from memory.
They are suitable for integer and FP register arguments/results that already
match ABI classes. They deliberately do not describe stack arguments,
by-value aggregate layout, variadic calls, or vector reshaping; those cases use
loader/runtime thunks and then finish with a null, identity, or simple
signature `PCALL`.

The required hardware invariant is that a signature slot is a cached RAT
template, not a marshalling program. It can rename compatible register lanes
during a frontend transition; it cannot touch user memory, inspect a stack
frame, split an aggregate, scan variadic state, or participate in lazy binding.
That line is what makes the feature practical for silicon or FPGA: register
aliasing fits rename hardware, while memory-layout conversion belongs in
software thunks where page faults and ABI policy are already manageable.

The design intentionally limits reconfigurable hardware to register names.
Modern OoO CPUs already map architectural registers such as `RDI`, `x0`, or
`a0` onto physical registers through rename/RAT state, so a cached ABI
signature can make the target ABI names point at already-live source argument
registers. That can be implemented with a small signature-slot bank and rename
muxing. It does not require a memory-layout engine, descriptor walker, or stack
rewriter.

The silicon contract is intentionally narrow: signature slots reconfigure
architectural names onto existing physical registers. They do not reconfigure
stack or memory layouts. That keeps `PCALL` branch-like and fixed-latency
instead of turning it into a page-fault-capable ABI marshalling sequencer.

This is the intended hardware/software boundary. The loader or runtime may
program a small bank of signature slots, for example 4 to 8 common ABI pairs,
and hot call sites select one with an immediate. Hardware handles the
register-only majority by rebinding architectural names to existing physical
registers in the rename/RAT stage. Software handles the memory-side minority:
overflow stack arguments, by-value structs, variadics, stack realignment,
lazy-binding policy, and any conversion that would require memory reads or
writes during the transition.

The silicon reason for this boundary is that register-only ABI translation can
reuse machinery that an out-of-order core already has. Architectural registers
such as x86_64 `RDI` or AArch64 `x0` are rename-table entries pointing at
physical registers. A cached Poly ABI signature is therefore a small RAT
template: on `PCALL`, the rename stage can make target ABI names point at the
physical registers that already hold the source ABI arguments. No integer or FP
move instructions are dispatched, and no stack memory is inspected.

This is intentionally semi-persistent hardware state, not per-call metadata.
The runtime programs signature slots at load time, link time, or lazy binding
time. A hot call uses `PCALL ... sig_imm` to select one prevalidated slot. The
fast path is deterministic: frontend redirect, return-cookie setup, and
rename-map rebinding. Calls that need stack arguments, by-value aggregate
repacking, variadic handling, or incompatible vector layout must still enter a
software thunk. That thunk performs the memory-side ABI work and then finishes
with a null, identity, or simple register signature.

The important silicon boundary is that a signature slot is a cached RAT
template, not a call descriptor. Hardware may relabel already-live physical
registers for compatible integer, FP, and fixed SIMD ABI lanes. It must not
read a stack frame, inspect variadic metadata, split structs, or marshal memory
during `PCALL`. Those cases deliberately stay in loader/runtime thunks.

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
generic `PCALL` subop in `EAX`, the ABI signature-slot count in `EBX`, and the
preferred hot slot manifest. `ECX` packs slot IDs as exchange, x86 SysV
register-only, x86 SysV register-only `__int128` return, and neutral
native-register slots in bytes 0..3. `EDX` packs the corresponding signature
kinds in the same byte lanes.
CPUID leaf `0x40000002`, subleaf `8` reports foreign signature `PCALL`
controls: `EAX=AArch64 PCALL_SIG`, `EBX=RISC-V PCALL_SIG`, and `ECX` as the
ABI signature-slot count.
CPUID leaf `0x40000002`, subleaf `10` reports foreign ABI signature-slot
controls: `EAX=AArch64 SET`, `EBX=AArch64 GET`, `ECX=RISC-V SET`, and
`EDX=RISC-V GET`.
CPUID leaf `0x40000002`, subleaf `11` reports foreign immediate-slot
signature-call encodings: `EAX=AArch64 PCALL_SIG_IMM slot 0`,
`EBX=RISC-V PCALL_SIG_IMM slot 0`, and `ECX` as the slot count. Slot `n`
is encoded by adding `n` to the reported control subop.
CPUID leaf `0x40000002`, subleaf `12` reports foreign trap-control setup
encodings: `EAX=AArch64 TRAP_VECTOR_SET`, `EBX=AArch64 MONITOR_PACKET_SET`,
`ECX=RISC-V TRAP_VECTOR_SET`, and `EDX=RISC-V MONITOR_PACKET_SET`.
CPUID leaf `0x40000002`, subleaf `13` reports AArch64 trap-control query and
mode encodings: `EAX=TRAP_VECTOR_GET`, `EBX=TRAP_VECTOR_MODE_SET`,
`ECX=TRAP_VECTOR_MODE_GET`, and `EDX=MONITOR_PACKET_GET`.
CPUID leaf `0x40000002`, subleaf `14` reports the same query and mode
encodings for RISC-V.

## Foreign Escapes

| Source | Encoding | Meaning |
| --- | --- | --- |
| AArch64 | `HINT #0x70` / `0xd5032e1f` | exit to x86_64 |
| AArch64 | `HINT #0x71` / `0xd5032e3f` | switch to RISC-V |
| AArch64 | `HINT #0x72` / `0xd5032e5f` | compatibility fixed call to RISC-V; new register-only code should use `PCALL_SIG_IMM` |
| AArch64 | `HINT #0x76` / `0xd5032edf` | trap return |
| AArch64 | `HINT #0x78` / `0xd5032f1f` | `PSWITCH`: target in `x16`, frontend ID in `x17` |
| AArch64 | `HINT #0x79` / `0xd5032f3f` | `PCALL`: target in `x16`, frontend ID in `x17`, continuation in `x18` |
| AArch64 | `HINT #0x60..#0x67` / `0xd5032c1f + (slot << 5)` | `PCALL_SIG_IMM`: target in `x16`, frontend ID in `x17`, continuation in `x18`, signature slot in encoding |
| AArch64 | `HINT #0x7a` / `0xd5032f5f` | `PCALL_SIG`: target in `x16`, frontend ID in `x17`, continuation in `x18`, signature slot in `x19` |
| AArch64 | `HINT #0x7b` / `0xd5032f7f` | landing-pad marker; no-op unless landing policy requires it |
| AArch64 | `HINT #0x7c` / `0xd5032f9f` | `ABI_SIGNATURE_SET`: `x0=slot`, `x1=kind`, returns `x0=0` or `-EINVAL` |
| AArch64 | `HINT #0x7d` / `0xd5032fbf` | `ABI_SIGNATURE_GET`: `x0=slot`, returns signature kind in `x0` or `-EINVAL` |
| AArch64 | `HINT #0x7e..#0x7f` / `0xd5032fdf..0xd5032fff` | landing-policy set/get; operand/result in `x0` |
| AArch64 | `HINT #0x68..#0x6d` / `0xd5032d1f..0xd5032dbf` | trap-vector set/get, trap-vector-mode set/get, monitor-packet set/get; operand/result in `x0` |
| RISC-V | custom-0, funct3=7, subop 0 / `0x0000700b` | exit to x86_64 |
| RISC-V | custom-0, funct3=7, subop 1 / `0x0200700b` | switch to AArch64 |
| RISC-V | custom-0, funct3=7, subop 2 / `0x0400700b` | compatibility fixed call to AArch64; new register-only code should use `PCALL_SIG_IMM` |
| RISC-V | custom-0, funct3=7, subop 6 / `0x0c00700b` | trap return |
| RISC-V | custom-0, funct3=7, subop 8 / `0x1000700b` | `PSWITCH`: target in `x5`, frontend ID in `x6` |
| RISC-V | custom-0, funct3=7, subop 9 / `0x1200700b` | `PCALL`: target in `x5`, frontend ID in `x6`, continuation in `x7` |
| RISC-V | custom-0, funct3=7, subop 16..23 / `0x2000700b + (slot << 25)` | `PCALL_SIG_IMM`: target in `x5`, frontend ID in `x6`, continuation in `x7`, signature slot in encoding |
| RISC-V | custom-0, funct3=7, subop 10 / `0x1400700b` | `PCALL_SIG`: target in `x5`, frontend ID in `x6`, continuation in `x7`, signature slot in `x28` |
| RISC-V | custom-0, funct3=7, subop 11 / `0x1600700b` | landing-pad marker; no-op unless landing policy requires it |
| RISC-V | custom-0, funct3=7, subop 12 / `0x1800700b` | `ABI_SIGNATURE_SET`: `a0=slot`, `a1=kind`, returns `a0=0` or `-EINVAL` |
| RISC-V | custom-0, funct3=7, subop 13 / `0x1a00700b` | `ABI_SIGNATURE_GET`: `a0=slot`, returns signature kind in `a0` or `-EINVAL` |
| RISC-V | custom-0, funct3=7, subop 24..29 / `0x3000700b..0x3a00700b` | trap-vector set/get, trap-vector-mode set/get, monitor-packet set/get; operand/result in `a0` |
| RISC-V | custom-0, funct3=7, subop 30..31 / `0x3c00700b..0x3e00700b` | landing-policy set/get; operand/result in `a0` |

These are decoded frontend-control instructions, not breakpoint or undefined
instruction traps. AArch64 `BRK`/RISC-V `EBREAK` remain ordinary trap exits for
debuggers or OS/user trap handling. The RISC-V encoding reserves one fixed
custom-0 funct3 signature and uses funct7 as the control subop, which gives a
simple hardware decode without consuming multiple custom opcode pages.

The fixed AArch64/RISC-V call opcodes remain decoded for compatibility with
older probes. Runtime-generated default register-only cross-frontend calls use
`PCALL_SIG_IMM`; specialized bridge opcodes remain for compact mixed lanes,
FP-stack policy, and fixed 128-bit vector policy.

Native return instructions may cross frontends when the link register or stack
return slot contains a hardware return cookie installed by `PCALL`.

Landing-pad markers are optional validation points for cross-frontend indirect
targets. Policy bit `1<<0` requires a landing marker at indirect `PSWITCH`
targets. Policy bit `1<<1` requires a landing marker at indirect `PCALL`
targets. The policy is frontend-neutral architectural state and is exported in
the Poly XSAVE component. With policy disabled, landing markers decode as
no-ops.

Foreign generic `PCALL` can name x86_64 as frontend `0`. Direct x86 targets are
the hardware contract: `PCALL_SIG` selects a cached register-only ABI signature
slot, so a foreign caller can enter an ordinary x86 SysV function without
executing register-move thunks when the call fits in registers. Hardware
installs a return cookie on the x86 stack; an ordinary x86 `ret` to that cookie
restores the foreign frontend and resumes at the foreign continuation register.
Loader/runtime thunks still own complex ABI policy, but the control transfer
itself now uses the same frontend-neutral `PCALL` path as AArch64-to-RISC-V and
RISC-V-to-AArch64. The Bochs reserved import-call range is an import-trap
delivery surface for unresolved targets, not a CPU-parsed descriptor ABI.
Foreign ABI signature-slot controls let AArch64 and RISC-V code program or
query the same architectural slot bank directly. x86_64 remains the boot and
system frontend, not the only frontend allowed to configure Poly call state.
Descriptor-backed x86 import calls are therefore not advertised in the base
Poly feature mask; software discovers and uses them as runtime fallback policy.

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
before issuing `PSWITCH` or `PCALL`. The preferred silicon fast path for hot
register-only native ABI calls is a small bank of programmable ABI signature
slots, so calls that only need register shuffling do not need software move
thunks.

A signature slot is hardware control state programmed by the loader or runtime.
It is a register-alias-table recipe, not a call descriptor. When `PCALL` names
a slot, the CPU rebinds source architectural names to target architectural
names in rename/RAT state, installs the return cookie, and redirects the
frontend. Operand data does not move through integer, FP, SIMD, load, or store
execution pipes, and the transition does not read user memory.

The slot is a compact ABI lane map. For an x86_64-to-AArch64 native-register
call, a slot can make AArch64 `x0,x1,x2` name the same physical registers that
currently back x86_64 `RDI,RSI,RDX`. The slot holds mapping metadata only; it
does not hold argument values and cannot point the CPU at stack or descriptor
memory.

The intended silicon shape is a small semi-persistent cache of signature slots,
for example 4 to 8 entries. A loader can program a neutral native-register slot
once, then emit `PCALL ... sig_imm` at register-only call sites. The slot maps
whichever frontend is currently the source onto the target frontend's native
ABI lanes: SysV x86_64 `RDI,RSI,RDX,RCX,R8,R9`, AAPCS64 `x0..x7`, and RISC-V
psABI `a0..a7` all use the same architectural kind. The setup cost is paid when
the loader programs the slot; the hot call-site cost is a frontend redirect
plus cached rename-template selection.

This is the only intended reconfigurable hardware in the ABI bridge. The
signature slot is a semi-persistent RAT template selected by a small immediate
on `PCALL`; it can make target ABI register names point at already-live source
physical registers without issuing move instructions. It must not describe
stack layouts, by-value aggregate packing, variadic state, or lazy-binding
policy.

Typical slots are programmed for common ABI pairs such as SysV-to-AAPCS64,
AAPCS64-to-SysV, SysV-to-RISC-V psABI, and RISC-V psABI-to-SysV. For these
all-register calls, the goal is zero execution-unit data moves: the rename/RAT
state makes target argument names point at source physical registers while the
frontend transition occurs. This is still a branch-like control operation, so
it has frontend and prediction costs; the win is that ABI register shuffles do
not dispatch as instructions and do not touch memory.

The lifecycle is intentionally simple: `PABI_SIG_SET` programs a compact
register-renaming recipe, the CPU validates and stores it in the slot bank, and
`PCALL ... sig_imm` applies that cached recipe in the rename/RAT stage. The
instruction does not parse a call descriptor, read a user stack, split structs,
or walk variadic metadata.

For example, if a native-register slot is applied to an x86_64-to-AArch64 call,
then `PCALL ..., slot` makes AArch64 `x0,x1,x2` name the physical registers
currently named by x86_64 `RDI,RSI,RDX`. No argument data is copied, and no
stack or descriptor memory is read by the transition instruction.

This is intentionally reconfigurable hardware, but only at the register-rename
boundary. It is suitable for silicon because modern OoO cores already maintain
rename maps from architectural registers to physical registers. The area cost
should stay small: a few prevalidated control registers plus muxing in the
rename path. The CPU must not grow a page-fault-capable stack repacker inside
`PCALL`.

The intended split is hybrid. Hardware handles the common all-register case,
including compatible integer and FP/SIMD ABI register lanes. Software handles
stack arguments, by-value aggregates, variadics, lazy binding, PLT/GOT policy,
cross-class vector reshaping, and every ABI case that requires memory
inspection or rewriting. A thunk performs that memory-side ABI work and then
finishes with a null, identity, or simple register signature `PCALL`.

In short: register-only native ABI calls use a cached signature slot; anything
that requires stack inspection, aggregate repacking, variadic metadata,
lazy-binding policy, or incompatible vector layout uses a loader/runtime thunk.

This is the intended 90/10 boundary. The CPU provides semi-persistent,
reconfigurable register translation because RAT remapping is close to machinery
already present in an OoO core. The CPU does not provide semi-persistent stack
or aggregate translation, because that would require memory reads, writes,
page-fault handling, ABI-specific layout engines, and variable-latency control
flow. Register-only calls get the hardware fast path; memory-shaped calls use
loader/runtime thunks.

The slot bank is explicit Poly architectural state. In the Bochs prototype it
is saved and restored by the Poly XSAVE component, not stored in a process-wide
global or inferred from CR3. That lets thread switches and explicit
`XSAVE`/`XRSTOR` preserve the active ABI signature configuration.

Older Bochs state-key controls may still exist as prototype diagnostics, but
they are not advertised as architectural hardware features. Software should use
the Poly XSAVE component as the context-switch contract.

The hardware boundary is strict: signatures can reconfigure register names,
not memory layouts. Stack arguments, by-value aggregate layout, variadics, lazy
binding, PLT/GOT policy, and unusual vector conventions remain
loader/runtime-thunk responsibilities. A thunk performs that memory-side ABI
work and then uses a null, identity, or simple register signature for the final
`PCALL`. This keeps `PCALL` fixed-latency and prevents page-fault-capable
memory marshalling from entering the CPU transition path.

When a direct `PCALL` enters x86_64, the Bochs prototype places the source
frontend stack pointer in volatile `R11`. This gives user-space x86 thunks a
fixed way to copy overflow stack arguments from the caller ABI into an x86 SysV
call frame without asking the CPU to parse descriptors or repack memory.

The prototype also treats the first eight scalar FP argument/result lanes as a
fast exchange window: x86_64 `XMM0..XMM7`, AArch64 `v0..v7`, and RISC-V
`fa0..fa7`. This is still register-only state. Fixed 128-bit vector calls can
use the same direct path when the source and target ABIs both use SIMD/vector
registers, as with x86_64 SysV and AArch64 AAPCS64. Cases that require
cross-class reshaping, such as RISC-V psABI 128-bit vector values passed in GPR
pairs, remain software-thunk or descriptor policy. Wider AVX, SVE, RVV, stack
FP overflow, and aggregate/vector ABI cases that require layout conversion also
remain thunk policy unless a future signature kind explicitly covers them.

Large memory returns follow the callee ABI. AArch64 uses `x8`; RISC-V uses `a0`
and shifts user arguments; x86_64 returns the hidden pointer in `RAX`.

## Traps And Syscalls

Hardware does not emulate Linux, libc, or libgcc. It only produces precise trap
records for foreign `svc`, `ecall`, `brk`, `ebreak`, unresolved imports,
illegal instructions, and unsupported instructions.

A `POLYTRAP` record contains the reason, source mode, selector/immediate, trap
PC, resume PC, all eight POLYTRAP argument lanes, and the first eight native
foreign ABI argument registers.
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
Component layout version `8` is 4096 bytes and contains the mode header, trap
packet, active transition record, AArch64 GPR/FP state, RISC-V GPR/FP state,
hardware transition-stack state, user-space monitor registers, and the ABI
signature slot bank. It also contains explicit per-frontend TLS bases:
AArch64 `TPIDR_EL0` state is separate from RISC-V `tp/x4` state, and both are
saved/restored through the Poly XSAVE component rather than inferred from x86
`R13`, CR3, or emulator-global state. Landing-policy flags are saved/restored
in the same Poly XSAVE component.

Private CPUID leaves start at `0x40000000` and extend through `0x40000009`. The
prototype software state import layout version is `8`; it is a Bochs fallback,
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
