# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension.  The extension
keeps standard x86_64 execution as the host ISA, then adds synthetic userspace
mode-switch envelopes that let selected AArch64 and RISC-V instruction streams
run through direct fixed-width foreign fetch inside the x86_64 process.

This is an active scaffold, not a complete native-speed AArch64/RISC-V CPU.  The
current implementation validates the architecture shape, Linux boot path,
foreign ELF launch path, mixed-ISA transitions, explicit foreign trap records,
and a deterministic compatibility runtime for scaffolded syscall/libcall tests.
It does not yet implement full AArch64 or RISC-V ISA coverage, real foreign
Linux ABI passthrough, or equal-speed execution.

## Current State

- `scripts/boot.sh` downloads an Alpine Linux x86_64 kernel and BusyBox package,
  builds a minimal initramfs, creates a bootable ISO, and boots it in Bochs.
- The Docker image builds the local Bochs fork from
  `bochs-prepoly-src/bochs` and installs it as `bochs-poly`.
- The guest prints `BOOT_OK` on a clean baseline boot.
- With `POLY_ENABLED=1`, Bochs handles the polyglot userspace mode/status
  envelopes and raw foreign fetch in `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`.
- `tools/polyprobe.c` validates raw AArch64 and RISC-V fetch/decode, wide
  register state, native returns, mixed raw instruction streams, repeated
  mixed-mode switch stress, mixed libcalls, mixed syscalls, and status/counter
  markers.
- `tools/polyapp.c` runs manifest-backed generated foreign ELF64 payloads from
  `tools/polyapps/*.poly` by entering raw foreign mode, executing packed
  32-bit foreign instructions, and escaping back to x86_64.  The manifest path
  accepts variable-size executable segments up to 1 MiB.
- `tools/polyexec.c` runs generated foreign ELF64 payloads directly by path
  using the same raw-mode execution path, including executable segments larger
  than one raw burst.
- `tools/polycall.c` loads generated foreign ELF64 function payloads and calls
  their entrypoints through the prototype hardware ABI bridge (`PCALL`), so
  the return path uses ordinary AArch64/RISC-V return instructions rather than
  raw escape instructions.
- `tools/polybench.c` executes long raw AArch64 and RISC-V loops inside the
  guest, verifies that raw instruction counters advance across multiple
  fetch/decode bursts, and checks one mixed raw AArch64-to-RISC-V code blob.
- `tools/polybinfmt.sh` can register guest `binfmt_misc` entries so generated
  AArch64 and RISC-V ELF64 payloads execute directly from the x86_64 guest.
- `docs/poly-isa.md` defines the silicon-oriented ISA contract: dedicated
  frontend-switch opcodes, XSAVE-visible foreign state, explicit trap exits,
  and native-ABI thunking for precompiled cross-ISA libraries.

## ISA Changes From Standard x86_64

The Bochs fork treats selected userspace `#UD` byte sequences as polyglot CPU
operations when `POLY_ENABLED=1`.  Normal x86_64 instructions are unchanged.
The extension is intentionally encoded as invalid x86 byte sequences so an
unmodified CPU still traps.  The current handler accepts these envelopes only
from guest userspace.

All x86-visible poly operations are wrapped in fixed 8-byte envelopes:

| Operation | Bytes | Effect |
| --- | --- | --- |
| Switch to x86_64 mode | `64 0f 0b 58 4d 4f 44 45` | Sets current poly mode to x86_64. |
| Switch to raw AArch64 mode | `65 0f 0b 52 41 57 36 34` | Sets current poly mode to raw AArch64; following bytes are fetched as fixed 32-bit AArch64 instructions. |
| Switch to raw RISC-V mode | `66 0f 0b 52 41 57 52 56` | Sets current poly mode to raw RISC-V; following bytes are fetched as fixed 32-bit RISC-V instructions. |
| x86 SysV call to AArch64 | `40 0f 0b 50 43 41 36 34` | Prototype `PCALL.A64.SYSV`: `R10=foreign target`, `R11=x86 return`; maps x86_64 SysV integer args to AAPCS64 and enters raw AArch64. |
| x86 SysV call to RISC-V | `40 0f 0b 50 43 52 56 36` | Prototype `PCALL.RV64.SYSV`: `R10=foreign target`, `R11=x86 return`; maps x86_64 SysV integer args to RISC-V psABI and enters raw RISC-V. |
| Syscall status | `2e 0f 0b 53 59 53 43 <id>` | Returns syscall state in `RAX`: `0=current mode`, `1=last foreign syscall number`, `2=last foreign syscall mode`. |
| Libcall status | `3e 0f 0b 4c 49 42 43 <id>` | Returns libcall state in `RAX`: `0=current libcall status`, `1=last libcall number`, `2=last libcall mode`. |
| Switch/status counters | `4e 0f 0b 53 57 43 48 <id>` | Returns mode/counter state in `RAX`: `0=switches`, `1=current mode`, `2=foreign raw instructions`, `3=foreign syscalls`, `4=foreign libcalls`. |
| Trap status | `36 0f 0b 54 52 41 50 <id>` | Returns last foreign trap state in `RAX`: `0=reason`, `1=source mode`, `2=number`, `3`-`8=arg0`-`arg5`, other ids return trap PC. |

Foreign execution always uses raw direct fetch.  Bochs enters raw mode through
the x86_64 switch envelope, bypasses x86 decode, and fetches fixed 32-bit
foreign instructions directly from `RIP`.  AArch64 `brk #0x7fff` and RISC-V
custom-0 instruction `0x0000000b` escape back to x86_64 at the next byte.  Raw
foreign fetch is only active at guest CPL3; kernel, interrupt, and exception
paths continue through normal x86_64 decode even if the current userspace poly
mode is raw AArch64 or raw RISC-V.  Raw fetch is also bound to the guest CR3
active at the raw-mode switch, so unrelated userspace tasks do not inherit raw
decoding after a scheduler switch or a fault in the raw-mode task.
The current raw run loop batches up to 64 raw foreign instructions before
returning to the outer Bochs event loop, while still checking async events and
mode exits between individual raw instructions.
Raw native returns stay native: AArch64 `ret` branches to `x30`/the selected
link register and RISC-V `ret` is handled as ordinary `jalr x0, 0(ra)`.  Raw
loader tests may still synthesize an explicit return landing pad by setting
AArch64 `x30` or RISC-V `ra` to a following x86 escape instruction.  The newer
prototype `PCALL` forms set a hardware-style return cookie in `x30` or `ra`;
ordinary foreign `ret` to that cookie switches back to x86_64 at the `R11`
return address.

The hybrid CPU currently defines foreign-mode memory ordering as x86_64 TSO.
AArch64 `dmb`, `dsb`, and `isb` barriers and RISC-V `fence` and `fence.i`
instructions are decoded as ordering-preserving no-ops instead of introducing
weaker AArch64/RISC-V reordering inside Bochs.

The current register bridge aliases the overlapping caller-visible integer ABI:

- x86_64 `RAX` carries the foreign return value and maps to AArch64 `x0` or
  RISC-V `a0`.
- x86_64 `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to AArch64 `x1`-`x6`
  and RISC-V `a1`-`a6`.
- x86_64 `RSP` maps to RISC-V `sp`; AArch64 `x31` is still decoded as zero for
  general register operands, with load/store base handling treating `x31` as
  `SP`.
- x86_64 `XMM0`-`XMM7` low 64-bit lanes map to AArch64 scalar `d0`-`d7` and
  RISC-V `fa0`-`fa7` for the currently decoded scalar double FP subset.
- Bochs tracks the remaining foreign integer registers in synthetic banks keyed
  by guest `CR3` and user `FSBASE`: AArch64 `x7`-`x30` plus syscall scratch
  `x8`, and RISC-V non-aliased registers including `a7`.

Precompiled cross-ISA linking is expected to use native ABI contracts, not a
custom compiler ABI.  The prototype `PCALL.A64.SYSV` and `PCALL.RV64.SYSV`
forms move the common thunk work into the emulated ISA: they map x86_64 SysV
integer arguments into AAPCS64 `x0`-`x7` or RISC-V psABI `a0`-`a7`; x86 args
1-6 come from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9`, while args 7-8 come
from the SysV stack slots at `[RSP+8]` and `[RSP+16]`.  During the foreign call,
the hardware bridge saves the x86 `RSP` and exposes the foreign `SP` as
`RSP+24`, so the first foreign stack argument is visible at `[sp]`; x86 `RSP` is
restored when the native foreign return hits the return cookie.  The bridge
preserves the shared `XMM0`-`XMM7` FP argument/return aliases, sets `x30` or
`ra` to a return cookie, and enters raw fetch at the `R10` target.  `polycall`
verifies this against loaded foreign ELF64 function payloads
(`aarch64-pcall-sum.elf`, `riscv-pcall-sum.elf`, `aarch64-pcall-sum8.elf`,
`riscv-pcall-sum8.elf`, `aarch64-pcall-sum9.elf`, and
`riscv-pcall-sum9.elf`) plus an AArch64 compiler-shaped stack-frame payload
(`aarch64-pcall-frame.elf`) that uses `sub sp, sp, #imm`, stack load/store, and
`add sp, sp, #imm`.  More complex ABI cases such as aggregate returns, variadic
calls, TLS, unwind, and exceptions still need descriptor-driven or software
thunk support.  Direct register aliases are an implementation optimization only
where they match the native ABI contract; they are not the external
compatibility contract.

Cross-ISA returns are expected to use native return instructions.  AArch64
libraries return with `ret` through `x30`, and RISC-V libraries return with
`jalr x0, 0(ra)`.  For `PCALL`, the return cookie routes that native return
back to the saved x86_64 continuation without executing a foreign breakpoint or
custom escape instruction.

Foreign traps are now recorded as explicit architectural exits before any
compatibility behavior runs.  AArch64 `svc` and RISC-V `ecall` record reason
`1`; AArch64 `brk` and RISC-V `ebreak` record reason `2`.  The record includes
source mode, trap number, six ABI arguments, and the foreign PC.  The current
Bochs dispatcher may still return deterministic scaffold values after recording
the trap, but the trap record is the intended ISA boundary.

## Supported Foreign Subset

The direct-fetch AArch64 path covers the generated/probed subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `adr`, `movz`, `movn`,
`movk`, `add`/`sub` immediate forms including `SP`, shifted-register
`add`/`sub`/`mul`/`eor`/`and`/`orr`, unconditional branch, `cbz`/`cbnz`, native
`ret`, `dmb`/`dsb`/`isb`, scalar double `fadd`/`fsub`/`fmul` and register
`fmov`, generic byte/halfword/word/dword load-store forms, `svc`, and `brk`.

The direct-fetch RISC-V path covers the generated/probed RV64 subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `lui`, `auipc`, OP-IMM
`addi`, `xori`, `ori`, `andi`, `slli`, `srli`, and `srai`, RV64 word
arithmetic, compare/register-shift and division/remainder forms,
register-register `add`, `sub`, `mul`, `xor`, `and`, and `or`,
`beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu`, `jal`, generic `jalr`, selected
byte/halfword/word/dword load-store forms, `fence`, `fence.i`, `ecall`,
`ebreak`, custom-0 escape, and scalar double `fadd.d`/`fsub.d`/`fmul.d` over
`fa0`-`fa7`.
`polybench` also validates the current efficient mixed-raw path: raw AArch64
escapes to x86_64 with `brk #0x7fff`, the next bytes immediately enter raw
RISC-V with the `RAWRV` envelope, and the RISC-V stream escapes with custom-0.
Synthetic AArch64/RISC-V register banks and the current poly mode are lazily
saved and restored per guest `CR3` plus user `FSBASE`.  A normal x86_64 Linux
process switch does not share foreign registers with another address space, and
threads in one process get separate synthetic banks when the kernel restores a
different TLS base. The low overlapping return/scratch values still use the
current x86 register bridge; this is not yet a full XSAVE-backed foreign
register ABI.

The Bochs compatibility runtime handles selected foreign Linux syscall traps
deterministically after recording the architectural trap.  Supported syscall
numbers currently include:

- Scalar/process syscalls: `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`,
  `getegid`, `gettid`, `getpgid(0)`, `getsid(0)`, and `exit`.
- File-style syscalls: `getcwd`, `read`, `write`, `openat`, `close`, `lseek`,
  and `uname`.
- Memory/time-style syscalls: `clock_gettime`, `getrusage`, `getcpu`,
  `gettimeofday`, `sysinfo`, and `mmap`.

The shared syscall dispatcher carries six foreign Linux ABI arguments for both
foreign architectures; current `mmap6` payloads verify argument registers beyond
`arg2` reach the dispatcher.

The Bochs compatibility runtime also handles selected breakpoint traps as
deterministic scaffold library calls.  This is not the hardware ISA contract;
real precompiled-code interop should use software thunks or OS/runtime trap
routing.

- AArch64 uses `brk #id`.
- RISC-V uses `a7=id; ebreak`.
- Supported ids are `1=strlen`, `2=memfill`, `3=memcmp`, and `4=memcpy`.
- `RDI`/AArch64 `x1`/RISC-V `a1` is the implicit left/destination pointer;
  explicit libcall operands use `RSI,RDX` via AArch64 `x2,x3` or RISC-V
  `a2,a3`.

## Validation Gates

Build the Docker image:

```bash
make image
```

Run the baseline x86_64 Linux boot:

```bash
make boot
```

Run the poly probe and manifest-backed generated foreign payloads:

```bash
make boot-poly
```

Run the fuller gate, including direct foreign ELF execution and guest
`binfmt_misc` registration:

```bash
make boot-poly-full
```

Expected success markers include:

- `BOOT_OK`
- `POLY_PROBE_OK`
- `POLYAPP_OK`
- `POLYEXEC_OK`
- `POLYCALL_OK`
- `POLYBENCH_OK`
- `POLYBINFMT_OK`

## Known Gaps

- Foreign execution is a Bochs UD-envelope prototype, not full native hardware
  decode.  The silicon contract in `docs/poly-isa.md` replaces these envelopes
  with CPUID-gated frontend-switch opcodes.
- AArch64 and RISC-V ISA support is limited to the tested generated subset.
- Syscall and breakpoint traps are recorded explicitly, but the current
  compatibility runtime still returns deterministic scaffold results rather
  than complete host or guest Linux ABI behavior.
- Equal-speed or minimal-slowdown execution is a design target.  The current
  implementation demonstrates raw direct-fetch execution and multi-burst raw
  loops, but not full equal-speed execution across complete ISAs.
- Foreign ELF support is limited to the generated static payload shape used by
  `tools/mkpolyelf.c`, `tools/polyapp.c`, and `tools/polyexec.c`, but both
  `polyapp` and `polyexec` load variable-size executable segments up to 1 MiB.
