# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension.  The extension
keeps standard x86_64 execution as the host ISA, then adds CPUID-gated
prototype opcode-family operations that let selected AArch64 and RISC-V
instruction streams run through direct foreign fetch inside the x86_64 process.

This is an active scaffold, not a complete native-speed AArch64/RISC-V CPU.  The
current implementation validates the architecture shape, Linux boot path,
foreign ELF launch path, mixed-ISA transitions, explicit foreign trap records,
and guest-routed syscall/break trap handling through an OS-neutral packet path.
It does not yet implement full AArch64 or RISC-V ISA coverage, real foreign
Linux ABI passthrough, or equal-speed execution.

## Current State

- `scripts/boot.sh` downloads an Alpine Linux x86_64 kernel and BusyBox package,
  builds a minimal initramfs, creates a bootable ISO, and boots it in Bochs.
- The Docker image builds the local Bochs fork from
  `bochs-prepoly-src/bochs` and installs it as `bochs-poly`.
- Every boot script run checks the poly import ID manifest, architecture
  contract, and CPUID/XSAVE contract gates.  These fail if Bochs CPU execution
  code starts using the deprecated compatibility trap knob again, if foreign
  syscalls stop routing through the OS-neutral architectural trap packet, if the
  removed fixed x86 import helper fallback reappears, if foreign barrier/fence
  decode stops honoring the x86-TSO no-op contract, if raw-mode interrupt
  entry/`IRET64`/`SYSRET`/`SYSEXIT` resume hooks drift, if direct
  AArch64/RISC-V frontend switches or native cross-call cookies start routing
  through x86 policy, or if Bochs' CPUID/XSAVE/trap constants drift from the
  guest/runtime header.
- The guest prints `BOOT_OK` on a clean baseline boot.
- The baseline `make boot` path runs `nativecheck.elf`, proving ordinary x86_64
  userspace still runs and the private poly CPUID leaves are hidden when
  `POLY_ENABLED=0`.
- The default `make boot-poly` path runs the guest trap-vector probe, manifest
  app suite, focused direct ELF execution, `PCALL`, thread, signal, and native
  CPUID checks.  This makes the hardware-style trap packet path the normal
  smoke test.
- The `make boot-poly-arch-traps` path verifies that AArch64/RISC-V syscall and
  breakpoint traps route
  through the architectural trap vector.  Its guest x86 handler translates the
  selected foreign Linux syscalls into real x86 Linux `syscall` instructions,
  then resumes the original raw frontend with `POLY_TRAP_RETURN`; it also runs
  generated AArch64/RISC-V process-identity, `getcwd`, `uname`,
  `clock_gettime`, `clock_getres`, `gettimeofday`, `set_tid_address`,
  `rt_sigprocmask`, robust-list, `prlimit64`, `getrandom`, real anonymous
  `mmap` store, real `mprotect`/`munmap` on anonymous mappings,
  `openat`/`read`/`close`, `newfstatat`, `fstat`, `statx`, zero-length
  `write`, `strlen`, `memfill`, `memcmp`, `memcpy`, and generated `ET_DYN`
  relative-relocation ELF payloads through the same architectural trap path.
  `nativecheck.elf` also verifies trap return preserves the source frontend's
  live native argument, syscall-number, and scalar FP alias registers while
  committing only the handler result, and that a userspace-installed trap
  vector, recorded trap packet, last-syscall status, and last-break status
  status do not leak across a `fork()` address-space boundary.
- The `make boot-poly-probe-arch-traps` path runs `polyprobe` with its own guest
  trap-vector handler, so the low-level raw syscall/break probe exercises packet
  delivery without Bochs synthesizing Linux or libc behavior.
- The `make boot-poly-binfmt-arch-traps` path registers `binfmt_misc` and
  executes a focused AArch64/RISC-V ELF set, including generated
  `ET_DYN #poly_entry` payloads, through the same userspace trap-vector policy.
- The `make boot-poly-call-arch-traps` path runs the generated cross-ISA `PCALL`
  suite, covering ordinary native AArch64/RISC-V returns, dynamic relocations,
  dependency loading, TLS, FP/int ABI bridging, x86 import descriptors,
  libc-style import descriptors, and atomic helper descriptors without relying
  on Bochs syscall/break emulation.  It also runs the thread-bank and
  signal-resume stress checks through the same OS-neutral trap path.
- The `make boot-poly-bench-arch-traps` path runs `polybench` with a
  guest-installed architectural trap vector, so cross-ISA syscall/break/import
  benchmark exits are routed through userspace packet handling rather than
  Bochs CPU policy.
- The `make boot-poly-apps-arch-traps` path runs the manifest-backed `polyapp`
  payload suite through a guest-installed trap vector, keeping syscall/break
  policy outside the CPU model.
- The `make boot-poly-full` and `make boot-poly-full-arch-traps` paths combine
  the trap-vector probe, manifest apps, focused direct ELF execution, `PCALL`,
  thread/signal, benchmark, binfmt, and native CPUID gates in one run.
  They intentionally use the real-syscall/trap-vector direct-exec set instead
  of the legacy deterministic fake-syscall `polyexec` matrix.
- With `POLY_ENABLED=1`, Bochs handles the polyglot userspace opcode-family
  operations and raw foreign fetch in `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`.
- `tools/polyprobe.c` validates raw AArch64 and RISC-V fetch/decode, wide
  register state, shared foreign stack-pointer frame handling, native returns,
  mixed raw instruction streams, repeated mixed-mode switch stress, mixed
  breakpoint traps, mixed syscalls, and status/counter markers.
- `tools/polyapp.c` runs manifest-backed generated foreign ELF64 payloads from
  `tools/polyapps/*.poly` by entering raw foreign mode, executing packed
  32-bit foreign instructions, and escaping back to x86_64.  The manifest path
  accepts variable-size executable segments up to 1 MiB.  It installs an x86
  architectural trap-vector handler for deterministic test syscalls/breaks,
  so no Bochs CPU syscall/string helper policy is required.  Breakpoint
  trap manifests use neutral `break_*` keys.
- `tools/polyexec.c` runs generated foreign ELF64 payloads directly by path
  using the same raw-mode execution path, preserving executable bytes exactly
  so RISC-V compressed 16-bit code does not have to be repacked as 32-bit
  words. It accepts explicit `foreign.elf#symbol` entries for generated
  `ET_DYN` images and applies simple architecture-relative dynamic relocations
  in userspace before entering raw mode. The full boot gate includes a 6-byte
  compressed RISC-V ELF entry segment to cover this path.
- `tools/polycall.c` loads generated foreign ELF64 function payloads and calls
  their entrypoints through the prototype hardware ABI bridge (`PCALL`), so
  the return path uses ordinary AArch64/RISC-V return instructions rather than
  raw escape instructions. It also resolves prototype foreign `JUMP_SLOT`
  function imports to hardware call-descriptor slots; `tools/polycall_x86_helpers.c`
  supplies separately compiled x86_64 helper targets for the foreign-to-x86
  descriptor tests.
- `tools/polythread.c` runs real x86_64 pthreads that repeatedly enter long
  AArch64 and RISC-V `PCALL` loops while keeping hidden foreign integer and FP
  registers live, exercising guest thread-bank isolation and interrupt/`IRET64`
  raw-mode restoration across many foreign transitions.
- `tools/polysignal.c` arms real guest `SIGALRM` delivery while executing long
  raw AArch64 and RISC-V `PCALL` loops with hidden integer and FP foreign
  registers live, checking that the x86_64 signal handler and `rt_sigreturn`
  path resume the interrupted foreign frontend and synthetic bank correctly.
- `tools/polybench.c` executes long raw AArch64 and RISC-V loops inside the
  guest, verifies that raw instruction counters advance across multiple
  fetch/decode bursts, and checks mixed raw AArch64-to-RISC-V and
  RISC-V-to-AArch64 code blobs, including a compressed RISC-V instruction
  before a direct AArch64 switch, that switch, call, nest calls, and carry
  scalar double, mixed integer/FP, two-register integer returns, shared-stack
  values, caller callee-saved integer/FP registers, and syscall trap results
  directly without returning to x86.
- `tools/polybinfmt.sh` can register guest `binfmt_misc` entries so generated
  AArch64 and RISC-V ELF64 payloads execute directly from the x86_64 guest,
  including expected-result checks for compressed RISC-V fixtures.
- `docs/poly-isa.md` defines the silicon-oriented ISA contract: dedicated
  frontend-switch opcodes, XSAVE-visible foreign state, explicit trap exits,
  and native-ABI thunking for precompiled cross-ISA libraries.

## ISA Changes From Standard x86_64

The Bochs fork treats selected userspace x86 byte sequences as polyglot CPU
operations when `POLY_ENABLED=1`.  Normal x86_64 instructions are unchanged.
The preferred prototype hot path decodes a fixed `0f 24 <op> POLY!`
opcode-family placeholder through the `BX_IA_POLYMODE` handler; the runtime
tools use this opcode family for hot frontend switches, `PCALL`, and status
reads.  Bochs' 32-bit and 64-bit x86 decoders must route `0f 24` to that
dedicated handler; the undefined-opcode path is not allowed to recover poly
opcodes as a fallback.  `UD2` is no longer an alternate polyglot envelope;
ordinary `UD2` retains standard invalid-opcode behavior.
The current handler accepts these operations only from guest userspace.

Preferred 8-byte x86 poly opcode-family operations:

| Operation | Bytes | Effect |
| --- | --- | --- |
| Switch to x86_64 mode | `0f 24 00 50 4f 4c 59 21` | Prototype `PEXIT`: sets current poly mode to x86_64. |
| Switch to raw AArch64 mode | `0f 24 01 50 4f 4c 59 21` | Prototype `PENTER.A64`: enters raw AArch64 direct fetch at the next byte. |
| Switch to raw RISC-V mode | `0f 24 02 50 4f 4c 59 21` | Prototype `PENTER.RV64`: enters raw RISC-V direct fetch at the next byte. |
| x86 SysV call to AArch64 | `0f 24 10 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV`: `R10=foreign target`, `R11=x86 return`, optional `R13=foreign TLS base`; maps x86_64 SysV integer args to AAPCS64 and enters raw AArch64. |
| x86 SysV call to RISC-V | `0f 24 11 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV`: `R10=foreign target`, `R11=x86 return`, optional `R13=foreign TLS base`; maps x86_64 SysV integer args to RISC-V psABI and enters raw RISC-V. |
| x86 SysV sret call to AArch64 | `0f 24 12 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to AAPCS64 `x8`, shifts user args back to `x0`-`x7`, and enters raw AArch64. |
| x86 SysV sret call to RISC-V | `0f 24 13 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.SRET`: maps the x86_64 hidden result pointer in `RDI` to RISC-V `a0`, shifts user args to `a1`-`a7`, and enters raw RISC-V. |
| x86 SysV call to AArch64 with two-float return | `0f 24 14 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FPAIR32RET`: same scalar argument mapping as `PCALL.A64.SYSV`, but packs AAPCS64 `s0`/`s1` into x86_64 SysV `XMM0[63:0]` on return. |
| x86 SysV call to RISC-V with two-float return | `0f 24 15 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FPAIR32RET`: same scalar argument mapping as `PCALL.RV64.SYSV`, but packs RISC-V `fa0`/`fa1` into x86_64 SysV `XMM0[63:0]` on return. |
| x86 SysV call to AArch64 with two-float argument | `0f 24 16 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FPAIR32ARG`: unpacks x86_64 SysV `XMM0[63:0]` into AAPCS64 `s0`/`s1` and shifts following FP argument lanes up by one foreign FP register. |
| x86 SysV call to RISC-V with two-float argument | `0f 24 17 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FPAIR32ARG`: unpacks x86_64 SysV `XMM0[63:0]` into RISC-V `fa0`/`fa1` and shifts following FP argument lanes up by one foreign FP register. |
| x86 SysV call to AArch64 with `{u64,double}` aggregate | `0f 24 18 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_U64_F64`: maps x86_64 SysV `RDI`/`XMM0` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `RAX`/`XMM0`. |
| x86 SysV call to AArch64 with `{double,u64}` aggregate | `0f 24 19 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_F64_U64`: maps x86_64 SysV `XMM0`/`RDI` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `XMM0`/`RAX`. |
| x86 SysV call to AArch64 with `{u64,float}` aggregate | `0f 24 1a 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_U64_F32`: maps x86_64 SysV `RDI`/`XMM0[31:0]` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `RAX`/`XMM0[31:0]`. |
| x86 SysV call to AArch64 with `{float,u64}` aggregate | `0f 24 1b 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.HETERO_F32_U64`: maps x86_64 SysV `XMM0[31:0]`/`RDI` aggregate argument lanes to AAPCS64 `x0`/`x1`, shifts the following integer argument to `x2`, and maps the returned `x0`/`x1` lanes back to x86_64 `XMM0[31:0]`/`RAX`. |
| x86 SysV call to RISC-V with `{u32,float}` compact aggregate | `0f 24 1c 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.COMPACT_U32_F32`: unpacks the x86_64 SysV packed aggregate from `RDI` into RISC-V `a0`/`fa0`, shifts the following integer argument to `a1`, and repacks the returned `a0`/`fa0` lanes into `RAX`. |
| x86 SysV call to RISC-V with `{float,u32}` compact aggregate | `0f 24 1d 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.COMPACT_F32_U32`: unpacks the x86_64 SysV packed aggregate from `RDI` into RISC-V `fa0`/`a0`, shifts the following integer argument to `a1`, and repacks the returned `fa0`/`a0` lanes into `RAX`. |
| x86 SysV FP-stack call to AArch64 | `0f 24 1e 50 4f 4c 59 21` | Prototype `PCALL.A64.SYSV.FP64STACK`: same scalar FP register aliases as `PCALL.A64.SYSV`, but copies x86_64 SysV FP overflow stack arguments from `[RSP+8]` to the foreign stack window. |
| x86 SysV FP-overflow call to RISC-V | `0f 24 1f 50 4f 4c 59 21` | Prototype `PCALL.RV64.SYSV.FP64STACK`: same scalar FP register aliases as `PCALL.RV64.SYSV`, but maps up to eight x86_64 SysV FP overflow stack arguments from `[RSP+8]` onward into RISC-V `a0`-`a7` as required by psABI once `fa0`-`fa7` are consumed. |
| x86 import return | `0f 24 20 50 4f 4c 59 21` | Prototype `PIRET`: resumes the saved foreign return PC after an x86 helper returns normally from a descriptor-driven import call. |
| Syscall status | `0f 24 30+id 50 4f 4c 59 21` | Returns syscall state in `RAX`: `id=0` current mode, `id=1` last foreign syscall number, `id=2` last foreign syscall mode. |
| Break status | `0f 24 38+id 50 4f 4c 59 21` | Returns breakpoint-trap state in `RAX`: `id=1` last break number, `id=2` last break source mode. |
| Switch/status counters | `0f 24 40+id 50 4f 4c 59 21` | Returns mode/counter state in `RAX`: `id=0` switches, `id=1` current mode, `id=2` foreign raw instructions, `id=3` foreign syscalls, `id=4` foreign breakpoint traps. |
| Trap status | `0f 24 50+id 50 4f 4c 59 21` | Returns last foreign trap state in `RAX`: `id=0` reason, `id=1` source mode, `id=2` number, `id=3`-`8` args 0-5, `id=9` trap PC, `id=10` trap selector/immediate, `id=11` resume PC, `id=12`-`13` args 6-7. |
| Trap vector | `0f 24 60-64 50 4f 4c 59 21` | `0x60` sets the architectural trap vector PC from `RAX`, `0x61` reads it into `RAX`, `0x62` resumes the recorded source frontend at the trap resume PC, `0x63` sets the trap-handler frontend mode from `RAX`, and `0x64` reads the handler mode into `RAX`. |
| State key | `0f 24 65-66 50 4f 4c 59 21` | `0x65` sets the explicit userspace poly state key from `RAX`, with `RAX=0` disabling the explicit key and falling back to the stack-region key. `0x66` reads the explicit key into `RAX`. |
| State save/restore | `0f 24 67-68 50 4f 4c 59 21` | Prototype fixed-layout state operations. `0x67` exports the current keyed poly state to the 4096-byte `struct poly_xsave_state` buffer pointed to by `RAX`; `0x68` imports that layout back into the current keyed state and continues in x86 mode. This is an explicit software operation, not a claim that leaf `0x40000003` exposes a real XCR0 component yet. |

When `POLY_ENABLED=1`, the prototype exposes a private CPUID discovery leaf for
runtime dispatch:

| Leaf | Registers | Meaning |
| --- | --- | --- |
| `0x40000000` | `EAX=0x40000009`, `EBX:EDX:ECX="PolyglotCPU!"` | Advertises the maximum poly CPUID leaf and the 12-byte poly vendor string. |
| `0x40000001` | `EAX=1`, `EBX=mode mask`, `ECX=feature mask`, `EDX=0` | Reports poly CPUID ABI version 1, supported frontend modes, implemented prototype features, and no architectural XSAVE component yet. |
| `0x40000002, subleaf 0` | `EAX[15:0]=0x7fff`, `EAX[31:16]=0x7ffe`, `EBX=0x7ffd`, `ECX=0x0000000b`, `EDX=0x0000002b` | Reports native raw-mode escape/cross-switch encodings: AArch64-to-x86, AArch64-to-RISC-V switch, AArch64-to-RISC-V call, RISC-V-to-x86, and RISC-V-to-AArch64 switch. |
| `0x40000002, subleaf 1` | `EAX=0x0000005b`, `EBX=0x0000107b`, `ECX=0x0000207b`, `EDX=0` | Reports the RISC-V-to-AArch64 native cross-call encoding and compact `{u32,float}`/`{float,u32}` native ABI cross-call variants. |
| `0x40000002, subleaf 2` | `EAX=106`, `EBX=8`, `ECX=32`, `EDX=16` | Reports the first prototype foreign-to-x86 import descriptor slot id, slot count, descriptor byte size, and import-call stride. |
| `0x40000002, subleaf 3` | `EAX=0x7ffa`, `EBX=0x0000307b`, `ECX=0`, `EDX=0` | Reports the neutral FP64 overflow stack-argument cross-call encodings: AArch64 `brk #0x7ffa` to RISC-V and RISC-V custom `0x0000307b` to AArch64. |
| `0x40000002, subleaf 4` | `EAX=0x7ff9`, `EBX=0x0000407b`, `ECX=0x63`, `EDX=0x64` | Reports the native raw-mode trap-return encodings and x86 trap-vector mode set/get opcodes. |
| `0x40000002, subleaf 5` | `EAX=140`, `EBX=0xffffe000`, `ECX=0xffffffff`, `EDX=16` | Reports the foreign import-call manifest: import ID count, 64-bit import-call window base split low/high, and import-call stride. |
| `0x40000003` | `EAX=state flags`, `EBX=23`, `ECX=0`, `EDX=0` | Reports the prototype foreign-state contract: overlapping x86-visible GPR/FP state plus synthetic banks, status registers, trap-vector policy, trap-packet state, trap-return save state, and fixed 32-byte transition frames keyed by `CR3`, `FSBASE`, and either an explicit userspace state key or an 8 MiB stack-region fallback key. `ECX=0`/`EDX=0` means no XCR0 component or XSAVE byte area is assigned yet. |
| `0x40000004` | `EAX=20`, `EBX=4096`, `ECX=0x00400002`, `EDX=0x1f` | Defines the silicon-target XSAVE contract: proposed XCR0 component 20, 4096-byte 64-byte-aligned save area, layout version 2, and flags requiring OSXSAVE/XCR0 enablement, interrupt-resume state, trap state, and no hidden foreign banks. This leaf is a formal architecture contract; the Bochs prototype still reports the active component as zero in leaf `0x40000003`. |
| `0x40000005` | `EAX=2`, `EBX=64`, `ECX=8`, `EDX=0x1f` | Defines the architectural `POLYTRAP` packet ABI: layout version 2, 64-byte trap header, eight native ABI argument slots, and flags for vector delivery, no-vector x86 exception delivery, trap-return state restoration, handler entry from all frontends, and trap-status opcodes. |
| `0x40000006` | `EAX=1`, `EBX=0x1f`, `ECX=0x0f`, `EDX=0x18` | Defines the raw-mode interrupt/resume ABI: raw fetch is CPL3-only, asynchronous entry uses a standard x86 interrupt frame after saving precise foreign state, raw loops check events between foreign instructions, and `IRET64`/`SYSRET`/`SYSEXIT`/signal-return paths can restore AArch64 or RISC-V raw mode. |
| `0x40000007` | `EAX=1`, `EBX=1`, `ECX=0x1f`, `EDX=0x18` | Defines the foreign memory-ordering ABI: raw AArch64/RISC-V execute under the x86 TSO model, use the same coherent x86 memory subsystem, decode AArch64 barriers and RISC-V fences as ordering-preserving no-ops, route atomics through coherent memory operations, and do not introduce weak reordering. |
| `0x40000008` | `EAX=1`, `EBX=0x1ff`, `ECX=0x00020004`, `EDX=0x19` | Defines the cross-frontend transition ABI: x86 transitions use decoded poly opcodes, raw frontends use native escape instructions, each transition flushes the frontend and ends the current block, next PCs are precise, raw instruction fetch is fixed-width/aligned, AArch64/RISC-V can switch or call each other without x86 rendezvous, native returns use hardware cookies, and trap return is architectural. |
| `0x40000009` | `EAX=1`, `EBX=0x7ff`, `ECX=0x00100808`, `EDX=0x00100020` | Defines the native ABI bridge/runtime descriptor contract: x86 SysV can enter AAPCS64 and RISC-V psABI, sret/scalar-FP/focused aggregate/FP64 overflow forms are hardware-described, imports use user-supplied descriptors with no CPU helper fallback, TLS base is explicit, x86 helpers return with ordinary `ret`, eight GPR and eight FP argument lanes are covered, the foreign stack is 16-byte aligned, descriptor records are 32 bytes, and descriptor call slots are 16 bytes apart. |

The current `0x40000001.EBX` mode mask sets bits `0`, `3`, and `4` for x86_64,
raw AArch64, and raw RISC-V.  `0x40000001.ECX` sets bits for raw AArch64, raw
RISC-V, neutral direct switches, native return cookies, x86 SysV `PCALL`,
`PCALL` sret, scalar FP bridging, trap records, user return restoration, x86 TSO
foreign ordering, and per-thread synthetic banks.  Bit `11` is reserved and is
not used to advertise syscall or breakpoint compatibility behavior; trap policy
is guest software, not CPU architecture.  Bit `12` additionally advertises the
prototype x86 poly opcode
family. Bit `13` advertises the two-float aggregate return packing
variants for native ABI `PCALL`; bit `14` advertises two-float aggregate
argument unpacking; bits `15`-`18` advertise the `{u64,double}`,
`{double,u64}`, `{u64,float}`, and `{float,u64}` heterogeneous aggregate
bridges for AArch64 `PCALL`; bits `19`-`20` advertise the RISC-V
`{u32,float}` and `{float,u32}` compact aggregate bridges; bit `21` advertises
the corresponding neutral AArch64<->RISC-V compact aggregate cross-call
bridges; bit `22` advertises runtime-supplied foreign-to-x86 import descriptor
slots; bit `23` advertises FP64 overflow stack-argument `PCALL` variants; bit
`24` advertises neutral AArch64<->RISC-V FP64 overflow stack-argument
cross-call variants; bit `25` advertises the architectural trap vector and
trap-return path; bit `26` advertises explicit software-selected poly state
keys.  The same
double-lane bridge forms also cover the ABI-compatible `{u32,double}` and
`{double,u32}` shapes.
Import ID `2` is reserved for the removed legacy x86-add helper and now traps
like any unresolved descriptor-backed import; runtime-provided x86 helper calls
start at descriptor slot ID `106`.
`0x40000003.EAX` reports the current state-management contract. Bits `0`-`6`
mean overlapping x86 GPR/FP state, prototype synthetic banks, `CR3` keying,
`FSBASE` keying, stack-region keying, user-return restore support, and x86 TSO
foreign ordering. Bit `7` is intentionally clear until foreign state is exposed
as an architectural XSAVE component. Bit `8` means software can select an
explicit state key with `0f 24 65 ... POLY!`; a zero key disables the explicit
selector and restores the stack-region fallback. Bit `9` means native
cross-frontend return state uses fixed 32-byte transition records rather than
ad hoc variable C fields. Bit `10` means the prototype implements explicit
`0f 24 67/68 ... POLY!` export/import operations using the same fixed
`struct poly_xsave_state` layout that leaf `0x40000004` assigns to a future
hardware XSAVE component. Bit `11` means the formal silicon-target XSAVE
component contract is present in leaf `0x40000004`, even if bit `7` is still
clear because the component is not active in guest `XSAVE/XRSTOR`.

Leaf `0x40000004` is the intended hardware state ABI.  When a silicon/FPGA
implementation sets bit `7` in `0x40000003.EAX` and reports component `20`,
the OS must include that XCR0 bit in its normal XSAVE/XRSTOR context-switch
mask.  Component `20` is reserved for the poly architecture because lower
component `11` is already the standard x86 CET_U state slot in current x86
XSAVE maps.  The 4096-byte component is versioned and 64-byte aligned.  It
contains the current foreign frontend mode, foreign PC, trap vector and trap
packet, fixed transition records, AArch64 `x0`-`x30`/`sp`/`v0`-`v31`/`NZCV`/FP
state, and RISC-V `x0`-`x31`/`f0`-`f31`/`fcsr` state.  Standard x86 GPR, XMM,
and other x86 architectural state remain in the normal x86 save areas.
Hardware must not rely on CR3/FSBASE hash tables or any other hidden foreign
register banks once this XSAVE component is active.

The C layout in `tools/polycpuid.h` is the executable ABI definition for this
component.  `struct poly_xsave_state`, `struct poly_trap_packet`, and the
fixed 32-byte `struct poly_transition_frame` are checked with compile-time
offset and size assertions, and the probe tools derive their expected CPUID
state/trap sizes from those structures rather than from independent literals.

Leaf `0x40000005` makes trap handling discoverable as a hardware ABI rather
than Bochs policy.  The packet header is the `0x40`-byte region stored at
offset `0x040` in the proposed XSAVE component: reason, source mode, trap
number, selector/immediate, trap PC, and resume PC.  The eight argument slots are
the `0x40`-byte region at offset `0x080`.  Software can depend on these fields
for syscall translation, debugger breakpoints, dynamic-linker binding, or
runtime policy without relying on Bochs CPU helpers.

Leaf `0x40000006` makes interrupt and exception resume behavior discoverable.
`EAX=1` is the interrupt ABI version.  `EBX` flags define CPL3-only raw fetch,
standard x86 interrupt-frame entry, state-component save before entry, precise
foreign PC recording, and event checks between raw instructions.  `ECX` reports
the supported return paths: `IRET64`, `SYSRET`, `SYSEXIT`, and guest signal
return.  `EDX` is the raw frontend mask covered by the contract, currently
bits `3` and `4` for AArch64 and RISC-V.

Leaf `0x40000007` makes memory ordering discoverable.  `EAX=1` is the memory
ABI version.  `EBX=1` selects the x86 TSO model.  `ECX` flags require raw
frontends to share the coherent x86 memory subsystem, treat AArch64
`dmb`/`dsb`/`isb` and RISC-V `fence`/`fence.i` as ordering-preserving no-ops,
route foreign atomics through coherent memory operations, and avoid injecting
weak AArch64/RISC-V reordering.  `EDX` is the covered raw frontend mask,
currently bits `3` and `4`.

Leaf `0x40000008` makes frontend-transition semantics discoverable separately
from the concrete encodings in leaf `0x40000002`.  `EAX=1` is the transition
ABI version.  `EBX` flags require x86 transitions to arrive through the decoded
poly opcode family, raw frontends to use native escape instructions, every
transition to flush the current frontend and end the current decode block,
source and destination PCs to be precise, raw fetch to use fixed-width aligned
instructions, AArch64/RISC-V transitions to work directly without an x86
rendezvous, native returns to use hardware cookies, and trap return to be an
architectural path.  `ECX[15:0]=4` is the AArch64 raw alignment, and
`ECX[31:16]=2` is the RISC-V raw alignment to allow RV64C entry points.
`EDX=0x19` is the covered frontend mask: x86 bit `0`, AArch64 bit `3`, and
RISC-V bit `4`.

Leaf `0x40000009` makes the native ABI bridge discoverable.  `EAX=1` is the
ABI bridge version.  `EBX` flags advertise x86_64 SysV to AAPCS64 and RISC-V
psABI call entry, hidden-sret forms, scalar FP aliases, the currently focused
aggregate bridge forms, FP64 overflow stack-argument forms, descriptor-backed
foreign-to-x86 imports, explicit TLS-base handoff, runtime-supplied descriptor
tables, removal of fixed CPU helper fallback semantics, and x86 helper return
through ordinary `ret`.  `ECX[7:0]=8` is the covered native GPR argument lane
count, `ECX[15:8]=8` is the covered scalar FP argument lane count, and
`ECX[31:16]=16` is the required foreign stack alignment.  `EDX[15:0]=32` is
the descriptor byte size, and `EDX[31:16]=16` is the descriptor call stride.
The `polycall` loader and `polybench` descriptor benchmarks validate this leaf
before installing descriptor-backed foreign-to-x86 imports, so runtime import
tables are only used when the CPU advertises user descriptors, ordinary x86
`ret` helper return, and no fixed CPU helper fallback.

Foreign execution always uses raw direct fetch.  Bochs enters raw mode through
the x86_64 poly opcode, bypasses x86 decode, and fetches foreign
instructions directly from `RIP`: fixed 32-bit instructions for AArch64 and
mixed 16/32-bit instructions for RISC-V.  AArch64 `brk #0x7fff` and RISC-V
custom-0 instruction `0x0000000b` escape back to x86_64 at the next byte.
AArch64 `brk #0x7ffe` switches directly to raw RISC-V, and RISC-V custom-1
instruction `0x0000002b` switches directly to raw AArch64, preserving the
shared low integer result/argument register state instead of routing through
x86.  AArch64 `brk #0x7ffd` is a prototype neutral call gate to a RISC-V target
address in `x16` with an AArch64 return PC in `x17`; RISC-V custom-2
`0x0000005b` is the reverse call gate to an AArch64 target in `x5` with a
RISC-V return PC in `x6`.  AArch64 `brk #0x7ffc`/`brk #0x7ffb` and RISC-V
custom opcodes `0x0000107b`/`0x0000207b` are compact `{u32,float}` and
`{float,u32}` native ABI variants for direct AArch64<->RISC-V calls.  AArch64
`brk #0x7ffa` and RISC-V custom opcode `0x0000307b` are FP64 overflow
stack-argument variants; they preserve the shared `d0`-`d7`/`fa0`-`fa7`
aliases while mapping the eight extra double lanes between AArch64 stack slots
and RISC-V `a0`-`a7`. AArch64 `brk #0x7ff9` and RISC-V custom opcode
`0x0000407b` are native raw-mode trap returns for mode-qualified architectural
trap handlers. The callee returns with its ordinary native return
instruction to a hardware cookie, which restores the caller frontend mode and
maps the shared integer and scalar FP argument/result registers back.  Raw
cross-call returns use a small bounded hardware-style return stack in the
prototype, so nested AArch64-to-RISC-V-to-AArch64 calls can unwind through
ordinary native returns without an x86 trampoline.  Raw
foreign fetch is only active at guest CPL3; kernel, interrupt, and exception
paths continue through normal x86_64 decode even if the current userspace poly
mode is raw AArch64 or raw RISC-V.  When a long-mode interrupt hits raw
foreign fetch, the prototype records the interrupted foreign frontend mode and
RIP in the synthetic bank, lets the x86_64 kernel run as x86, and restores raw
mode after `IRET64` returns to the recorded user RIP.  Raw fetch is bound to
the guest `CR3`, user `FSBASE`, and the explicit state key or stack-region
fallback key, so unrelated userspace tasks and common pthread stacks do not
inherit raw decoding after a scheduler switch or a fault in the raw-mode task.
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
Foreign atomic instructions use the same Bochs virtual-memory path as ordinary
foreign loads and stores.  The current prototype covers compiler-emitted
AArch64 exclusive and LSE atomics, RISC-V A-extension word/dword AMOs including
signed and unsigned min/max, and the LR.W/SC.W loops GCC emits for RISC-V byte
and halfword atomics, including compiler-emitted subtract and NAND loops.  GCC
AArch64 outline atomic helper symbols are compatibility imports resolved through
the descriptor call gate, not fixed CPU-side helper semantics.

The current register bridge aliases the overlapping caller-visible integer ABI:

- x86_64 `RAX` carries the foreign return value and maps to AArch64 `x0` or
  RISC-V `a0`.
- x86_64 `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to AArch64 `x1`-`x6`
  and RISC-V `a1`-`a6`.
- x86_64 `RSP` maps to RISC-V `sp` and to AArch64 `SP` where the AArch64
  encoding defines register 31 as stack pointer.  AArch64 operands that define
  register 31 as zero register still read as `XZR/WZR`.
- x86_64 `XMM0`-`XMM7` low lanes map to AArch64 scalar `s0`/`d0`-`s7`/`d7`
  and RISC-V `fa0`-`fa7` for the currently decoded scalar float/double FP
  subset.
- Bochs tracks the remaining foreign integer registers in synthetic banks keyed
  by guest `CR3` and user `FSBASE`: AArch64 `x7`-`x30` plus syscall scratch
  `x8`, and RISC-V non-aliased registers including `a7`.

Precompiled cross-ISA linking is expected to use native ABI contracts, not a
custom compiler ABI.  The prototype `PCALL.A64.SYSV` and `PCALL.RV64.SYSV`
forms move the common thunk work into the emulated ISA: they map x86_64 SysV
integer arguments into AAPCS64 `x0`-`x7` or RISC-V psABI `a0`-`a7`; x86 args
1-6 come from `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9`, while args 7-8 come
from the SysV stack slots at `[RSP+8]` and `[RSP+16]`.  During the foreign call,
the hardware bridge saves the x86 `RSP`, exposes a separate 16-byte-aligned
foreign `SP` window below the x86 frame, and copies integer overflow stack
arguments from `[RSP+24]` onward so the first foreign stack argument is visible
at `[sp]`; AArch64 FP64 stack-argument variants copy from `[RSP+8]` because
x86_64 SysV uses the overflow stack only after `XMM0`-`XMM7` are consumed, while
the RISC-V variant maps those overflow FP lanes into `a0`-`a7` per psABI.  x86
`RSP` is restored when the native foreign return hits the return cookie.  The
bridge preserves the shared `XMM0`-`XMM7` FP argument/return aliases, including
two-register homogeneous double aggregate arguments and returns through
`XMM0`/`XMM1`, two-`float` homogeneous aggregate returns packed into
`XMM0[63:0]`, two-`float` homogeneous aggregate arguments unpacked from
`XMM0[63:0]` into two foreign FP argument registers, including mixed integer/FP signatures where GPR and XMM lanes
are consumed in one native call.  AArch64 also has focused bridges for common
heterogeneous `{u64,double}`, `{double,u64}`, `{u64,float}`, and
`{float,u64}` aggregate shapes, moving FP lanes between x86 `XMM0` and
AAPCS64 integer-register bit lanes.  The `{u64,double}` and `{double,u64}`
bridge forms also cover the corresponding `{u32,double}` and `{double,u32}`
native ABI layouts because x86 SysV, AAPCS64, and RISC-V psABI keep the
integer lane in the same logical argument/result position with narrower
low-bit payloads.  RISC-V also has focused compact aggregate bridges for
`{u32,float}` and `{float,u32}`, where x86 SysV and AAPCS64 use one packed GPR
but RISC-V psABI splits the same value across `a0` and `fa0`.  The neutral
AArch64<->RISC-V cross-call opcodes have matching compact aggregate variants,
so those packed/split mappings do not need an x86 rendezvous.  The bridge sets `x30` or `ra` to a return cookie, enters raw fetch at the
`R10` target, carries an optional foreign TLS block base in x86 `R13`, maps
AArch64 `x0` or RISC-V `a0` back to x86 `RAX`, and maps
AArch64 `x1` or RISC-V `a1` back to x86 `RDX` for ordinary two-word integer
aggregate returns.  The
prototype `SRET` variants cover larger memory-return aggregates by mapping the
x86 hidden result pointer to AArch64 `x8` or RISC-V `a0` and shifting the
remaining user arguments according to the target ABI.  `polycall`
verifies this against loaded foreign ELF64 function payloads
(`aarch64-pcall-sum.elf`, `riscv-pcall-sum.elf`, `aarch64-pcall-sum8.elf`,
`riscv-pcall-sum8.elf`, `aarch64-pcall-sum9.elf`,
`riscv-pcall-sum9.elf`, and compiler-produced AArch64/RISC-V shared objects
(`aarch64-pcall-real.so#poly_entry`, `riscv-pcall-real.so#poly_entry`,
`riscv-pcall-real-rv64gc.so#poly_entry`,
`aarch64-pcall-gnu-hash-real.so#poly_entry`,
`riscv-pcall-gnu-hash-real.so#poly_entry`,
`aarch64-pcall-state.so#poly_entry`, and
`riscv-pcall-state.so#poly_entry`) plus compiler-built imported-function
objects (`aarch64-pcall-import-real.so#poly_entry` and
`riscv-pcall-import-real.so#poly_entry`,
`riscv-pcall-import-real-rv64gc.so#poly_entry`), compiler-built libc-style imported
function objects (`aarch64-pcall-libc-import-real.so#poly_entry` and
`riscv-pcall-libc-import-real.so#poly_entry`) that call `strlen`, `strcmp`, `strncmp`, `memcpy`,
`memmove`, `memset`, `memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`,
`strncpy`, `strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`,
`stpcpy`, `stpncpy`, `mempcpy`, `memrchr`, `memmem`, `rawmemchr`,
`strchrnul`, `strcasecmp`, `strncasecmp`, `strcasestr`, `bcmp`, `bcopy`, `bzero`, `index`, and `rindex` through
PLT/GOT descriptors, compiler-built imported-object
objects (`aarch64-pcall-import-value-real.so#poly_entry` and
`riscv-pcall-import-value-real.so#poly_entry`), compiler-built weak undefined
import objects (`aarch64-pcall-weak-import-real.so#poly_entry` and
`riscv-pcall-weak-import-real.so#poly_entry`), compiler-built stack-protected
objects (`aarch64-pcall-stack-protector-real.so#poly_entry` and
`riscv-pcall-stack-protector-real.so#poly_entry`), compiler-built errno-access
objects (`aarch64-pcall-errno-real.so#poly_entry` and
`riscv-pcall-errno-real.so#poly_entry`), compiler-built aux-vector dispatch
objects (`aarch64-pcall-getauxval-real.so#poly_entry` and
`riscv-pcall-getauxval-real.so#poly_entry`), compiler-built page-size query
objects (`aarch64-pcall-getpagesize-real.so#poly_entry` and
`riscv-pcall-getpagesize-real.so#poly_entry`) plus compiler-built `sysconf`
page-size query objects (`aarch64-pcall-sysconf-real.so#poly_entry` and
`riscv-pcall-sysconf-real.so#poly_entry`) and compiler-built environment-query
objects (`aarch64-pcall-env-real.so#poly_entry` and
`riscv-pcall-env-real.so#poly_entry`), compiler-built allocator objects
(`aarch64-pcall-alloc-real.so#poly_entry` and
`riscv-pcall-alloc-real.so#poly_entry`) plus allocator-backed string-copy
objects (`aarch64-pcall-strdup-real.so#poly_entry` and
`riscv-pcall-strdup-real.so#poly_entry`) and aligned-allocation objects
(`aarch64-pcall-aligned-alloc-real.so#poly_entry` and
`riscv-pcall-aligned-alloc-real.so#poly_entry`) plus teardown-registration
objects (`aarch64-pcall-atexit-real.so#poly_entry` and
`riscv-pcall-atexit-real.so#poly_entry`) and process-query objects
(`aarch64-pcall-process-real.so#poly_entry` and
`riscv-pcall-process-real.so#poly_entry`), compiler-built `DT_NEEDED`
shared-library pairs (`aarch64-pcall-needed-real.so#poly_entry` with
`libpolyneeded-aarch64.so` and its leaf dependency
`libpolyneeded-leaf-aarch64.so`, and
`riscv-pcall-needed-real.so#poly_entry` with `libpolyneeded-riscv.so` and its
leaf dependency `libpolyneeded-leaf-riscv.so`), compiler-built `DT_NEEDED`
dependency-IFUNC pairs (`aarch64-pcall-needed-ifunc-real.so#poly_entry` with
`libpolyneededifunc-aarch64.so`, and
`riscv-pcall-needed-ifunc-real.so#poly_entry` with
`libpolyneededifunc-riscv.so`), compiler-built `DT_NEEDED` dependency-`DT_INIT`
and dependency-`DT_FINI` pairs
(`aarch64-pcall-needed-dt-init-real.so#poly_entry` with
`libpolyneededdtinit-aarch64.so`, and
`riscv-pcall-needed-dt-init-real.so#poly_entry` with
`libpolyneededdtinit-riscv.so`), compiler-built relocated function-pointer
objects (`aarch64-pcall-funcptr-real.so#poly_entry` and
`riscv-pcall-funcptr-real.so#poly_entry`), compiler-built two-word aggregate
return objects (`aarch64-pcall-pair-real.so#poly_entry` and
`riscv-pcall-pair-real.so#poly_entry`), compiler-built hidden-sret aggregate
return objects (`aarch64-pcall-sret-real.so#poly_entry` and
`riscv-pcall-sret-real.so#poly_entry`), compiler-built constructor/destructor
objects (`aarch64-pcall-ctor-real.so#poly_entry`,
`riscv-pcall-ctor-real.so#poly_entry`,
`aarch64-pcall-fini-real.so#poly_entry`, and
`riscv-pcall-fini-real.so#poly_entry`), compiler-built TLS objects
(`aarch64-pcall-tls-real.so#poly_entry` and
`riscv-pcall-tls-real.so#poly_entry`) plus initial-exec TLS objects
(`aarch64-pcall-tls-ie-real.so#poly_entry` and
`riscv-pcall-tls-ie-real.so#poly_entry`), compiler-built conditional objects
(`aarch64-pcall-cond-real.so#poly_entry` and
`riscv-pcall-cond-real.so#poly_entry`), compiler-built compare-and-branch
objects (`aarch64-pcall-cbz-real.so#poly_entry` and
`riscv-pcall-cbz-real.so#poly_entry`), compiler-built bit-test branch
objects (`aarch64-pcall-bitbranch-real.so#poly_entry` and
`riscv-pcall-bitbranch-real.so#poly_entry`), compiler-built signed-extension
objects (`aarch64-pcall-signed-ext-real.so#poly_entry` and
`riscv-pcall-signed-ext-real.so#poly_entry`), compiler-built signed-load
objects (`aarch64-pcall-signed-load-real.so#poly_entry` and
`riscv-pcall-signed-load-real.so#poly_entry`), compiler-built integer-division
objects (`aarch64-pcall-int-div-real.so#poly_entry` and
`riscv-pcall-int-div-real.so#poly_entry`), compiler-built integer multiply-add
objects (`aarch64-pcall-int-madd-real.so#poly_entry` and
`riscv-pcall-int-madd-real.so#poly_entry`), compiler-built integer high-multiply
objects (`aarch64-pcall-int-highmul-real.so#poly_entry` and
`riscv-pcall-int-highmul-real.so#poly_entry`), compiler-built 128-bit
division/modulo helper objects (`aarch64-pcall-int128-helpers-real.so#poly_entry`
and `riscv-pcall-int128-helpers-real.so#poly_entry`), compiler-built 128-bit
integer/double conversion helper objects
(`aarch64-pcall-int128-fp-helpers-real.so#poly_entry` and
`riscv-pcall-int128-fp-helpers-real.so#poly_entry`), compiler-built 128-bit
integer/float conversion helper objects
(`aarch64-pcall-int128-float-helpers-real.so#poly_entry` and
`riscv-pcall-int128-float-helpers-real.so#poly_entry`), compiler-built libgcc
bit helper objects (`aarch64-pcall-bit-helpers-real.so#poly_entry` and
`riscv-pcall-bit-helpers-real.so#poly_entry`), compiler-built quad-precision
`long double` helper objects (`aarch64-pcall-longdouble-helpers-real.so#poly_entry`,
`riscv-pcall-longdouble-helpers-real.so#poly_entry`,
`aarch64-pcall-longdouble-signed-helpers-real.so#poly_entry`, and
`riscv-pcall-longdouble-signed-helpers-real.so#poly_entry`), compiler-built
quad-precision comparison/conversion helper objects
(`aarch64-pcall-longdouble-compare-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-compare-helpers-real.so#poly_entry`), compiler-built
quad-precision 32-bit integer conversion helper objects
(`aarch64-pcall-longdouble-int32-helpers-real.so#poly_entry` and
`riscv-pcall-longdouble-int32-helpers-real.so#poly_entry`), compiler-built integer carry-chain
objects (`aarch64-pcall-int-carry-real.so#poly_entry` and
`riscv-pcall-int-carry-real.so#poly_entry`), compiler-built integer variable-shift
objects (`aarch64-pcall-int-varshift-real.so#poly_entry` and
`riscv-pcall-int-varshift-real.so#poly_entry`), compiler-built integer logical
objects (`aarch64-pcall-int-logic-real.so#poly_entry` and
`riscv-pcall-int-logic-real.so#poly_entry`), compiler-built integer bit-operation
objects (`aarch64-pcall-int-bitops-real.so#poly_entry` and
`riscv-pcall-int-bitops-real.so#poly_entry`), compiler-built integer rotate/extract
objects (`aarch64-pcall-int-rotate-real.so#poly_entry` and
`riscv-pcall-int-rotate-real.so#poly_entry`), compiler-built integer conditional
compare objects (`aarch64-pcall-int-ccmp-real.so#poly_entry` and
`riscv-pcall-int-ccmp-real.so#poly_entry`), AArch64 post-index memory
object (`aarch64-pcall-postindex-mem.so#poly_entry`), AArch64 atomic objects
covering exclusive LL/SC, default GCC outline helpers, LSE instructions, and
16-byte libatomic helper imports
(`aarch64-pcall-atomic.so#poly_entry`,
`aarch64-pcall-atomic-outline.so#poly_entry`, and
`aarch64-pcall-atomic-lse.so#poly_entry`), RISC-V atomic object with AMO and
16-byte libatomic helper import coverage
(`riscv-pcall-atomic.so#poly_entry`), compiler-built unscaled-memory
objects (`aarch64-pcall-unscaled-mem-real.so#poly_entry` and
`riscv-pcall-unscaled-mem-real.so#poly_entry`), compiler-built indexed-memory
objects (`aarch64-pcall-indexed-mem-real.so#poly_entry` and
`riscv-pcall-indexed-mem-real.so#poly_entry`), compiler-built callee-saved
stack-frame objects (`aarch64-pcall-callee-real.so#poly_entry` and
`riscv-pcall-callee-real.so#poly_entry`), compiler-built scalar double FP
objects (`aarch64-pcall-fp64-real.so#poly_entry` and
`riscv-pcall-fp64-real.so#poly_entry`) plus sixteen-double FP stack-argument objects
(`aarch64-pcall-fp64-stack-real.so#poly_entry` and
`riscv-pcall-fp64-stack-real.so#poly_entry`), compiler-built homogeneous double
aggregate return objects (`aarch64-pcall-fpair-real.so#poly_entry` and
`riscv-pcall-fpair-real.so#poly_entry`), compiler-built homogeneous float
aggregate return objects (`aarch64-pcall-fpair32-real.so#poly_entry` and
`riscv-pcall-fpair32-real.so#poly_entry`), compiler-built homogeneous double
aggregate argument objects (`aarch64-pcall-fpair-arg-real.so#poly_entry` and
`riscv-pcall-fpair-arg-real.so#poly_entry`), compiler-built homogeneous float
aggregate argument objects (`aarch64-pcall-fpair32-arg-real.so#poly_entry` and
`riscv-pcall-fpair32-arg-real.so#poly_entry`), compiler-built mixed integer/FP
argument objects (`aarch64-pcall-mixed-args-real.so#poly_entry` and
`riscv-pcall-mixed-args-real.so#poly_entry`), compiler-built heterogeneous
aggregate objects (`aarch64-pcall-hetero-real.so#poly_entry` and
`riscv-pcall-hetero-real.so#poly_entry`,
`aarch64-pcall-hetero-rev-real.so#poly_entry`, and
`riscv-pcall-hetero-rev-real.so#poly_entry`,
`aarch64-pcall-hetero32-real.so#poly_entry`,
`riscv-pcall-hetero32-real.so#poly_entry`,
`aarch64-pcall-hetero32-rev-real.so#poly_entry`, and
`riscv-pcall-hetero32-rev-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-real.so#poly_entry`,
`riscv-pcall-hetero-u32-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-rev-real.so#poly_entry`, and
`riscv-pcall-hetero-u32-rev-real.so#poly_entry`,
`aarch64-pcall-hetero-u32-f32-real.so#poly_entry`,
`riscv-pcall-hetero-u32-f32-real.so#poly_entry`,
`aarch64-pcall-hetero-f32-u32-real.so#poly_entry`, and
`riscv-pcall-hetero-f32-u32-real.so#poly_entry`), compiler-built scalar double and
float FP import objects (`aarch64-pcall-fp64-import-real.so#poly_entry`,
`riscv-pcall-fp64-import-real.so#poly_entry`,
`aarch64-pcall-fp32-import-real.so#poly_entry`, and
`riscv-pcall-fp32-import-real.so#poly_entry`), compiler-built scalar double FP
callee-saved objects (`aarch64-pcall-fp64-callee-real.so#poly_entry` and
`riscv-pcall-fp64-callee-real.so#poly_entry`), compiler-built scalar float FP
callee-saved objects (`aarch64-pcall-fp32-callee-real.so#poly_entry` and
`riscv-pcall-fp32-callee-real.so#poly_entry`), compiler-built scalar double FP
conditional objects (`aarch64-pcall-fp64-cond-real.so#poly_entry` and
`riscv-pcall-fp64-cond-real.so#poly_entry`), compiler-built scalar double FP
division objects (`aarch64-pcall-fp64-div-real.so#poly_entry` and
`riscv-pcall-fp64-div-real.so#poly_entry`), compiler-built scalar double FP
unary/zero-compare objects (`aarch64-pcall-fp64-unary-real.so#poly_entry` and
`riscv-pcall-fp64-unary-real.so#poly_entry`), compiler-built scalar double FP
absolute-value objects (`aarch64-pcall-fp64-abs-real.so#poly_entry` and
`riscv-pcall-fp64-abs-real.so#poly_entry`), compiler-built scalar double FP
square-root objects (`aarch64-pcall-fp64-sqrt-real.so#poly_entry` and
`riscv-pcall-fp64-sqrt-real.so#poly_entry`), compiler-built scalar double FP
fused multiply-add objects (`aarch64-pcall-fp64-fma-real.so#poly_entry` and
`riscv-pcall-fp64-fma-real.so#poly_entry`) plus fused multiply-add variant
objects (`aarch64-pcall-fp64-fma-variants-real.so#poly_entry` and
`riscv-pcall-fp64-fma-variants-real.so#poly_entry`), compiler-built scalar double
FP min/max objects (`aarch64-pcall-fp64-minmax-real.so#poly_entry` and
`riscv-pcall-fp64-minmax-real.so#poly_entry`), compiler-built scalar double FP
select objects (`aarch64-pcall-fp64-select-real.so#poly_entry` and
`riscv-pcall-fp64-select-real.so#poly_entry`), compiler-built scalar double FP
indexed-memory objects (`aarch64-pcall-fp64-indexed-mem-real.so#poly_entry` and
`riscv-pcall-fp64-indexed-mem-real.so#poly_entry`), compiler-built scalar double
FP conversion objects (`aarch64-pcall-fp64-convert-real.so#poly_entry` and
`riscv-pcall-fp64-convert-real.so#poly_entry`) plus signed conversion objects
(`aarch64-pcall-fp64-signed-convert-real.so#poly_entry` and
`riscv-pcall-fp64-signed-convert-real.so#poly_entry`) and 32-bit conversion
objects (`aarch64-pcall-fp64-i32-convert-real.so#poly_entry`,
`riscv-pcall-fp64-i32-convert-real.so#poly_entry`,
`aarch64-pcall-fp64-u32-convert-real.so#poly_entry`, and
`riscv-pcall-fp64-u32-convert-real.so#poly_entry`) plus scalar float/double
conversion objects (`aarch64-pcall-fp-mixed-convert-real.so#poly_entry` and
`riscv-pcall-fp-mixed-convert-real.so#poly_entry`), compiler-built scalar float FP
objects (`aarch64-pcall-fp32-real.so#poly_entry` and
`riscv-pcall-fp32-real.so#poly_entry`), compiler-built scalar float FP
absolute-value objects (`aarch64-pcall-fp32-abs-real.so#poly_entry` and
`riscv-pcall-fp32-abs-real.so#poly_entry`), compiler-built scalar float FP
square-root objects (`aarch64-pcall-fp32-sqrt-real.so#poly_entry` and
`riscv-pcall-fp32-sqrt-real.so#poly_entry`), compiler-built scalar float FP
fused multiply-add objects (`aarch64-pcall-fp32-fma-real.so#poly_entry` and
`riscv-pcall-fp32-fma-real.so#poly_entry`) plus fused multiply-add variant
objects (`aarch64-pcall-fp32-fma-variants-real.so#poly_entry` and
`riscv-pcall-fp32-fma-variants-real.so#poly_entry`), compiler-built scalar float
FP min/max objects (`aarch64-pcall-fp32-minmax-real.so#poly_entry` and
`riscv-pcall-fp32-minmax-real.so#poly_entry`), compiler-built scalar float FP
select objects (`aarch64-pcall-fp32-select-real.so#poly_entry` and
`riscv-pcall-fp32-select-real.so#poly_entry`), compiler-built scalar float FP
memory objects (`aarch64-pcall-fp32-mem-real.so#poly_entry` and
`riscv-pcall-fp32-mem-real.so#poly_entry`), and compiler-shaped stack-frame payloads
(`aarch64-pcall-frame.elf`, `aarch64-pcall-native-frame.elf`,
`aarch64-pcall-bl.elf`, `aarch64-pcall-adrp.elf`, `aarch64-pcall-cond.elf`,
`aarch64-pcall-split-load.elf`, `aarch64-pcall-dynrel.elf`,
`aarch64-pcall-rel.elf`, `aarch64-pcall-relr.elf`,
`aarch64-pcall-relr-bitmap.elf`,
`aarch64-pcall-irelative.elf`, `aarch64-pcall-dynsym.elf`,
`aarch64-pcall-dyntab.elf`,
`aarch64-pcall-dyntab-entry.elf`, `riscv-pcall-frame.elf`,
`riscv-pcall-split-load.elf`, `riscv-pcall-dynrel.elf`,
`riscv-pcall-rel.elf`, `riscv-pcall-relr.elf`,
`riscv-pcall-relr-bitmap.elf`,
`riscv-pcall-irelative.elf`, `riscv-pcall-dynsym.elf`,
`riscv-pcall-dyntab.elf`, and
`riscv-pcall-dyntab-entry.elf`) that use native
`SP` adjustment, stack load/store, pair frame save/restore, local AArch64 `bl`
calls, AArch64 `adrp` page-relative data addressing, NZCV-backed AArch64
conditional branches, RISC-V `auipc` page-relative data addressing, real
AArch64 and RISC-V `ET_DYN` `.so` objects with dynamic symbol lookup, writable
static data, split ELF `PT_LOAD` text/data layout, simple `ET_DYN` relative
relocations through both `RELA` addends and `REL` in-place addends, packed
`DT_RELR` direct and bitmap relative relocation tables,
`R_AARCH64_IRELATIVE`/`R_RISCV_IRELATIVE` resolver relocations,
symbolic `STT_GNU_IFUNC` resolver relocations, same-image symbolic 64-bit
dynamic relocations and exported entrypoints through both section-backed and
`PT_DYNAMIC` symbol metadata, including SysV `DT_HASH` and GNU `DT_GNU_HASH`
symbol counts plus `DT_JMPREL`/`JUMP_SLOT` PLT relocations
using both `DT_PLTREL=RELA` and `DT_PLTREL=REL` tables for sectionless dynamic
objects, `R_AARCH64_COPY`/`R_RISCV_COPY` copy relocations for non-PIE foreign
executables referencing data exported by same-directory foreign shared
libraries, scalar double FP arguments and returns through
the native FP register ABI, scalar double FP function imports through PLT/GOT call
descriptors, real compiler-emitted GOT loads for undefined object-symbol
imports, weak undefined function/object relocations resolving to zero,
same-directory `DT_NEEDED` foreign shared-library dependencies with direct
binding of undefined function and object-symbol relocations to dependency
text/data, dependency-to-root callback, TLS, and IFUNC relocations, and dependency-exported symbolic IFUNC resolvers, weak undefined
relocations binding to root or dependency exports when present, direct-dependency
symbol interposition ahead of transitive dependencies when symbols collide,
GNU symbol-version and provider-SONAME matching for dependency exports,
ELF binding/visibility filtering for dependency exports including
`STB_GNU_UNIQUE` object symbols,
recursive loading of dependency libraries' own `DT_NEEDED`
entries, and dependency sets larger than the original four-library prototype
limit, with boot coverage for direct dependency fans larger than eight
libraries. Foreign `DT_RUNPATH`/`DT_RPATH` entries support `$ORIGIN/...`,
`${ORIGIN}/...`, `$LIB`/`${LIB}` token expansion to `lib`, and
`$PLATFORM`/`${PLATFORM}` expansion to the foreign frontend platform name
(`aarch64` or `riscv`), relative dependency subdirectories, and absolute
dependency directories, and absolute `DT_NEEDED` path entries are honored,
including `$ORIGIN/...` dynamic-string token expansion in `DT_NEEDED` names
and old-style RPATH-only objects built without
new dtags, colon-separated RUNPATH fallback directories, and `LD_LIBRARY_PATH`
lookup for foreign dependencies with the same `$ORIGIN`, `$LIB`, and
`$PLATFORM` dynamic-string token expansion.
Colon-separated `LD_LIBRARY_PATH` entries continue after missing directories.
`LD_LIBRARY_PATH` is searched before declared RUNPATH/RPATH directories, and
declared RUNPATH/RPATH directories are searched before the loader's
compatibility fallback to the caller's directory,
so a colocated same-SONAME object does not override the object's explicit
dynamic linker metadata. Dependency library dynamic relocations are applied
before those dependency calls
execute, dependency-local TLS blocks in the shared `PCALL` TLS image,
dependency `DT_INIT`/`DT_INIT_ARRAY` constructor execution before entering
dependent foreign code and dependency `DT_FINI_ARRAY`/`DT_FINI` teardown
afterward with destructor-visible dependency state checks,
compiler-emitted same-image function-pointer relocations and indirect native
calls, `DT_INIT`/`DT_INIT_ARRAY` constructor execution before foreign
entrypoints, `DT_FINI_ARRAY`/`DT_FINI` destructor execution during teardown,
compiler-emitted AArch64 TLSDESC, AArch64 traditional
`R_AARCH64_TLS_DTPMOD64`/`R_AARCH64_TLS_DTPREL64`, and RISC-V
`__tls_get_addr` access to `PT_TLS` initial images through the `PCALL`
TLS-base register, plus
initial-exec `R_AARCH64_TLS_TPREL64` and `R_RISCV_TLS_TPREL64` accesses,
including undefined `STT_TLS` relocations bound to `DT_NEEDED` dependencies
or root-object TLS exports referenced by dependencies,
versioned `DT_NEEDED` calls that require a non-default dependency symbol
version,
compiler-emitted conditional select variants and branch patterns
(`aarch64-pcall-select-variants-real.so`,
`riscv-pcall-select-variants-real.so`, `aarch64-pcall-import.elf`,
`riscv-pcall-import.elf`), prototype imported
function call gates (`aarch64-pcall-import-func.elf`,
`aarch64-pcall-import-mul.elf`, `aarch64-pcall-import-x86.elf`,
`riscv-pcall-import-func.elf`, `riscv-pcall-import-cjalr.elf`,
`riscv-pcall-import-cjr.elf`, `riscv-pcall-import-mul.elf`,
`riscv-pcall-import-x86.elf`, `aarch64-pcall-import-x86-mul.elf`, and
`riscv-pcall-import-x86-mul.elf`) plus real compiler-emitted PLT/GOT calls to
foreign-to-x86 helper descriptors including eight-argument x86 SysV
stack-argument calls (`aarch64-pcall-x86-sum8-import-real.so#poly_entry` and
`riscv-pcall-x86-sum8-import-real.so#poly_entry`) plus non-tail
post-import continuations that fold in a ninth foreign stack argument
(`aarch64-pcall-x86-sum8-post-import-real.so#poly_entry` and
`riscv-pcall-x86-sum8-post-import-real.so#poly_entry`), and
`poly_import_add`, `strlen`, `strcmp`, `strncmp`, `memcpy`, `memmove`, `memset`,
`memcmp`, `memchr`, `strchr`, `strrchr`, `strstr`, `strcpy`, `strncpy`,
`strnlen`, `strcat`, `strncat`, `strspn`, `strcspn`, `strpbrk`, `stpcpy`,
`stpncpy`, `mempcpy`, `memrchr`, `memmem`, `rawmemchr`, `strchrnul`, `bcmp`,
`bcopy`, `bzero`, `strcasecmp`, `strncasecmp`, `strcasestr`, `index`, `rindex`,
`__stack_chk_guard`, `__stack_chk_fail`, `__errno_location`, `getauxval`, `getpagesize`, `sysconf`, `getenv`, `secure_getenv`, `malloc`, `calloc`, `realloc`, `free`, `strdup`, `strndup`, `posix_memalign`, `aligned_alloc`, `memalign`, `atexit`, `__cxa_atexit`, `__cxa_finalize`, `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`, `getegid`, and `gettid`, and teardown
before returning.
The `poly_import_x86_add`, `poly_import_x86_mul`,
`poly_import_x86_sum6`, `poly_import_x86_sum8`,
`poly_import_x86_fp64_add`, `poly_import_x86_fp64_sum8`,
`poly_import_x86_mixed_u64_fp64`, and `poly_import_x86_fp32_add` descriptors enter
real x86_64 helpers through a runtime-supplied descriptor table.  The same
generic descriptor table is now used by `polycall` for `poly_import_add` and
`poly_import_mul`, `poly_import_fp64_add`/`poly_import_fp32_add`,
compiler-emitted libc string/memory imports such as
`strlen`, `memcpy`, and `memcmp`, plus
environment, allocation, teardown-registration, stack-failure, aux-vector/page-size,
errno, and process-query imports such as `getenv`, `malloc`, `atexit`,
`__stack_chk_fail`, `getauxval`, `__errno_location`, and `getpid`, plus
TLS accessor imports for AArch64 TLSDESC, AArch64 traditional
`__tls_get_addr`, and RISC-V `__tls_get_addr`, plus scalar
libgcc 128-bit div/mod helpers `__udivti3`, `__umodti3`, `__divti3`, and
`__modti3`, scalar int128/float conversion helpers `__fixdfti`,
`__fixunsdfti`, `__floattidf`, `__floatuntidf`, `__fixsfti`,
`__fixunssfti`, `__floattisf`, and `__floatuntisf`, and bit helpers
`__clzdi2`, `__ctzdi2`, `__paritydi2`, and
`__popcountdi2`, plus 16-byte libatomic helpers
`__atomic_compare_exchange_16`, `__atomic_load_16`, and
`__atomic_store_16`, and quad-precision libgcc helpers `__addtf3`,
`__subtf3`, `__multf3`, `__divtf3`, `__floatunditf`, `__fixunstfdi`,
`__floatditf`, `__floatsitf`, `__floatunsitf`, `__fixtfdi`, `__fixtfsi`,
`__fixunstfsi`, `__eqtf2`, `__lttf2`, `__letf2`, `__gttf2`, `__getf2`,
`__netf2`, `__unordtf2`, `__extendsftf2`, `__extenddftf2`,
`__trunctfsf2`, and `__trunctfdf2`.  Every valid import-window slot is now
descriptor-backed or trap-backed: if the runtime supplies a descriptor, Bochs
performs the generic cross-frontend x86 call gate; otherwise Bochs records an
architectural import trap with the fixed native ABI argument lanes.  Bochs no
longer classifies individual libc, TLS, process-query, libatomic, or libgcc
helper IDs to decide whether a software descriptor is required, and it does not
synthesize fixed CPU fallback results.
This makes those library, TLS, and process-query calls runtime policy rather than CPU semantics. The
descriptor call gate maps the first six native foreign integer arguments to
x86_64 SysV `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9`, places seventh and
eighth integer arguments in the standard x86 stack-argument slots when needed,
reuses the shared
`XMM0-XMM7`/`v0-v7`/`fa0-fa7` FP register aliases for scalar FP arguments and
returns, including an eight-double x86 SysV import that exercises every scalar
FP argument register and a mixed integer/double import that exercises the
independent SysV GPR and XMM argument counters, synthesize an x86 return address to the dedicated `0f 24` `PIRET`,
accept each helper's ordinary `ret`, and map the x86 `RAX` result back to
AArch64 `x0` or RISC-V `a0` for integer returns.  ABI-specific details such as
the AAPCS64 even-register hole before a 16-byte integer argument are handled by
the runtime descriptor target, not by CPU-side import-ID argument rewrites.
RISC-V quad-precision helper descriptors likewise rebuild `__float128`
operands from the fixed `a0/a1` and `a2/a3` GPR lane pairs in runtime x86
wrappers instead of asking the CPU import gate to populate x86 XMM argument
registers for specific helper IDs. Each 32-byte x86 import descriptor is
`{target, trampoline, flags, reserved}`.  Flag bit `0` requests the seventh and
eighth native GPR argument lanes be written to x86 stack-argument slots, bit
`1` maps an x86 `RDX:RAX` 128-bit integer return back to the native foreign
return register pair, and bit `2` maps an x86 `XMM0` binary128 return back to
AArch64 `v0` or RISC-V `a0/a1`.  These flags are runtime metadata, not
CPU-side helper-ID policy. The current
`polycall` harness points these descriptors at `noinline` x86_64 C functions
linked from `tools/polycall_x86_helpers.c`, so the path exercises a separately
compiled x86 helper object with compiler-generated function bodies rather than
handwritten helper bytecode in the call trampoline. The eight-argument x86
helper also performs a nested compiled x86 helper call before returning, so the
descriptor path covers ordinary x86 call/return activity inside the imported
target.
The same descriptor mechanism currently resolves AArch64 TLSDESC, AArch64
traditional `__tls_get_addr`, and RISC-V `__tls_get_addr` TLS accesses for
self-contained foreign shared objects through runtime x86 helper descriptors
that consume the `PCALL` TLS-base register, and common GCC AArch64 outline
atomic helper imports used by default compiler output:
8-, 16-, 32-, and 64-bit `ldadd`, `swp`, `ldclr`, `ldeor`, `ldset`, and `cas`
helpers with `relax`, `acq`, `rel`, and `acq_rel` suffixes.  The loader
aliases these suffixes to the same operation descriptor because the prototype
defines foreign atomic memory ordering in terms of the x86-TSO execution
model.
More complex ABI cases such as arbitrary external import target descriptors,
additional heterogeneous register aggregates, variadic calls, dynamic-loader TLS module
allocation across multiple DSOs, unwind, and exceptions still need full
descriptor-driven or software thunk support.  Direct register
aliases are an implementation optimization only where they match the native ABI
contract; they are not the external compatibility contract.

Cross-ISA returns are expected to use native return instructions.  AArch64
libraries return with `ret` through `x30`, and RISC-V libraries return with
`jalr x0, 0(ra)`.  For `PCALL`, the return cookie routes that native return
back to the saved x86_64 continuation without executing a foreign breakpoint or
custom escape instruction.
`polycall` accepts `foreign.elf#symbol` requests so tests can target a named
exported function instead of only the ELF entrypoint; the dynamic-relocation
probes export `poly_entry` away from offset zero to exercise that path.

Foreign traps are recorded as explicit, operating-system-neutral architectural
exits.  AArch64 `svc` and RISC-V `ecall` record reason `1`; AArch64 `brk` and
RISC-V `ebreak` record reason `2`; unresolved descriptor-backed foreign
imports record reason `3`.  The record includes source mode, trap number, eight ABI arguments, the foreign PC, the resume PC, and the raw trap
selector/immediate when the foreign instruction encoding carries one.  For
syscall traps, the number is the native ABI syscall register (`x8` for
AArch64, `a7` for RISC-V); AArch64 `svc #imm` stores `imm` only in the
selector field.  For import traps, the number is the unresolved import
descriptor id, the selector is zero, the PC is the descriptor target, and the
resume PC is the native link-register return address.  In hardware or FPGA this
packet is the boundary: firmware, the OS, or a userspace runtime routes it.
Bochs does not synthesize Linux syscall or libc-helper results from raw trap
instructions.

Software can install an architectural trap
vector with `0f 24 60 ... POLY!` using `RAX=handler_pc` and can select the
handler frontend with `0f 24 63 ... POLY!` using `RAX=mode`.  The default
handler mode is x86_64.  In the Bochs prototype, the installed trap vector and
handler frontend mode are part of the same keyed userspace poly state as the
synthetic foreign registers.  The recorded trap packet/status, temporary
trap-return save state, and fixed 32-byte cross-frontend transition records are
keyed there as well.  The legacy syscall/break
status registers are keyed with the same state, so a different guest address
space starts with no installed vector, no stale trap packet, no stale
trap-return frame, and no stale last-syscall/break status.  For an x86
handler, trap delivery uses
`RAX=reason`, `RBX=source mode`, `RCX=trap number`, `RDX=trap PC`,
`RSI=selector`, `RDI=arg0`, `R8`-`R12` for trap arguments `1`-`5`, and
`R13`-`R14` for trap arguments `6`-`7`;
the same fields remain available through trap-status opcodes for debugging and
late inspection.  For an AArch64 handler, delivery uses
`x0=reason`, `x1=source mode`, `x2=trap number`, `x3=trap PC`, `x4=selector`,
and `x5`-`x12` for trap arguments `0`-`7`; for a RISC-V handler, it uses
`a0=reason`, `a1=source mode`, `a2=trap number`, `a3=trap PC`,
`a4=selector`, `a5`-`a7` for trap arguments `0`-`2`, and `t0`-`t2` for trap
arguments `3`-`5`, plus `t3`-`t4` for trap arguments `6`-`7`.
The handler can read the full packet with trap-status opcodes, apply
OS/runtime policy in software, place the result in the shared result register,
and execute the native trap-return instruction for its frontend:
`0f 24 62 ... POLY!` from x86, AArch64 `brk #0x7ff9`, or RISC-V custom
`0x0000407b`.  Trap delivery preserves the source frontend's integer and
scalar/vector FP register state across the handler, then commits only the
handler result register back to the source result register on trap return.  This
keeps the Bochs prototype's trap-vector path closer to a hardware interrupt or
firmware trap boundary instead of a hidden Linux/libc emulation call.
The `polyexec` trap-vector handler is the current userspace policy example:
its entry stub only adapts the architectural trap-packet register ABI to a C
dispatcher, and that dispatcher maps selected generic AArch64/RISC-V Linux
syscall numbers to ordinary x86_64 Linux syscall numbers before executing a
real x86 `syscall`.  That mapping is guest software, not Bochs CPU behavior.

## Supported Foreign Subset

The direct-fetch AArch64 path covers the generated/probed subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `adr`, `adrp`, `movz`, `movn`,
`movk`, `add`/`sub` immediate forms including `SP`, flag-setting `adds`/`subs`
immediate and shifted-register forms, shifted-register
`add`/`sub`/`adc`/`sbc`/`mul`/`madd`/`msub`/`umulh`/`smulh`/`udiv`/`sdiv`/`and`/`bic`/`orr`/`orn`/`eor`/`eon`/`ands`/`bics`, variable register shifts
`lsl`/`lsr`/`asr`/`ror`, extended-register `add`/`sub` forms,
unsigned bitfield aliases `uxtb`/`uxth`/`lsl`/`lsr`/`ubfx`,
signed bitfield aliases `sxtb`/`sxth`/`sxtw`/`asr`/`sbfx`,
bitfield move aliases `bfxil`/`bfi`,
one-source bit operations `rbit`/`rev16`/`rev32`/`rev`/`clz`/`cls`,
extract/rotate `extr` and logical shifted-register `ror` operands,
scalar AdvSIMD `ushr d`,
unconditional branch and call `b`/`bl`,
condition-code branch `b.cond`, register branch and call `br`/`blr`,
`cbz`/`cbnz`, bit-test branch `tbz`/`tbnz`,
conditional compare `ccmp`/`ccmn`,
conditional select `csel`/`csinc`/`csinv`/`csneg`,
logical-immediate `and`/`orr`/`eor`
and `tst`/`ands`, native `ret`, `dmb`/`dsb`/`isb`, scalar double
and float `fadd`/`fsub`/`fmul`/`fdiv`, fused multiply-add `fmadd`,
`fmsub`, `fnmadd`, and `fnmsub`, register `fmov`, unary
`fneg`/`fabs`/`fsqrt`, normal-number min/max `fminnm`/`fmaxnm`,
`fcmp`/`fcmpe` including zero-immediate compare, scalar FP `fcsel`,
32-bit and 64-bit signed and
unsigned FP-to-integer conversion for finite scalar float and double values,
scalar float/double `fcvt` narrowing and widening, signed and unsigned
integer-to-FP `scvtf`/`ucvtf` conversions from GPR and scalar FP/SIMD integer
sources, scalar FP/GPR bit moves, scalar FP pair loads/stores and scalar FP `ldr`/`str`,
generic byte/halfword/word/dword load-store forms plus scalar FP load-store
forms including register-offset indexed addressing, 64-bit
`stp`/`ldp` pair load-store forms, unscaled `ldur`/`stur` integer and scalar FP
forms plus single-register pre/post-indexed load-store writeback forms,
exclusive atomics `ldxr`/`ldaxr`/`stxr`/`stlxr` and `clrex`, LSE atomic
read-modify-write forms `swp`, `ldadd`, `ldclr`, `ldeor`, `ldset`,
`ldsmin`, `ldsmax`, `ldumin`, `ldumax`, and `cas` for decoded 8-, 16-, 32-,
and 64-bit compiler-emitted encodings, `svc`, and `brk`.

The direct-fetch RISC-V path covers the generated/probed RV64 subset used by
`polyprobe`, `polyapp`, `polyexec`, and `polybench`: `lui`, `auipc`, OP-IMM
`addi`, `xori`, `ori`, `andi`, `slli`, `srli`, and `srai`, RV64 word
arithmetic, compare/register-shift and division/remainder forms,
register-register `add`, `sub`, `mul`, `mulh`, `mulhsu`, `mulhu`, `xor`, `and`, and `or`,
`beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu`, `jal`, generic `jalr`, selected
byte/halfword/word/dword load-store forms, A-extension `lr`/`sc`, AMO
word/dword forms including signed/unsigned min/max, and compiler-emitted
byte/halfword LR.W/SC.W atomic loops including subtract and NAND,
`fence`, `fence.i`, `ecall`,
`ebreak`, custom-0 escape, and scalar double `fadd.d`/`fsub.d`/`fmul.d` over
`fa0`-`fa7`, plus scalar float `fadd.s`/`fsub.s`/`fmul.s` and scalar FP
division, square root, and fused multiply-add
`fdiv.s`/`fdiv.d`/`fsqrt.s`/`fsqrt.d`/`fmadd.s`/`fmadd.d`,
`fmsub.s`/`fmsub.d`, `fnmsub.s`/`fnmsub.d`, and
`fnmadd.s`/`fnmadd.d` on the same mapped
FP argument registers, FP min/max `fmin.s`/`fmin.d`/`fmax.s`/`fmax.d`,
FP compare `feq`/`flt`/`fle`, FP sign-injection
`fsgnj`/`fsgnjn`/`fsgnjx` including `fneg`/`fabs`/`fmv`, integer-to-FP bit moves
`fmv.w.x`/`fmv.d.x`, FP-to-integer bit moves `fmv.x.w`/`fmv.x.d`,
FP class tests `fclass.s`/`fclass.d`, signed and unsigned integer-to-FP
`fcvt.s/d.{w,wu,l,lu}`, signed and unsigned FP-to-integer conversions from
scalar float/double, scalar float/double `fcvt.s.d`/`fcvt.d.s`, and FP
`flw`/`fld`/`fsw`/`fsd` memory forms.  It also
decodes a first RV64C compatibility subset for common
compressed integer code: `c.addi4spn`, `c.ld`, `c.sd`, `c.addi`, `c.li`,
`c.lui`, `c.addi16sp`, `c.j`, `c.beqz`, `c.bnez`, `c.slli`, `c.lw`/`c.sw`,
`c.ldsp`/`c.lwsp`, `c.addiw`, `c.srli`/`c.srai`, `c.andi`,
`c.sub`/`c.xor`/`c.or`/`c.and`, `c.subw`/`c.addw`, `c.mv`, `c.jr`/`c.ret`,
`c.ebreak`, `c.jalr`, `c.add`, `c.sdsp`/`c.swsp`, and compressed double-FP
memory forms `c.fld`/`c.fsd` plus `c.fldsp`/`c.fsdsp`.
`polybench` also validates efficient neutral mixed-raw paths: direct
AArch64-to-RISC-V and RISC-V-to-AArch64 frontend switches, plus cross-ISA calls
where the caller enters the other foreign frontend and the callee returns with
ordinary native `ret`/`jalr` through a hardware cookie without routing through
x86.  The gate also covers scalar double FP cross-calls in both directions,
eight-register double FP argument pressure across `d0`-`d7`/`fa0`-`fa7`,
FP64 overflow stack-argument cross-calls that sum sixteen double arguments in
both directions, and mixed integer/FP cross-calls where an integer argument is
converted and folded into the FP return value across the opposite foreign frontend.  Shared-stack
neutral cross-call probes verify an AArch64 or RISC-V caller can allocate an
aligned stack slot, the opposite frontend can load it through its native `sp`,
and the caller can restore the same user stack before returning to x86.
Callee-saved probes verify AArch64 `x19` and RISC-V `s0` remain live across a
neutral call into the opposite frontend.  Pair-return probes verify `x0`/`x1`
and `a0`/`a1` both map back to the caller so ordinary two-word integer returns
survive a neutral cross-call.  Syscall probes verify a callee can execute a
native `svc` or `ecall`, return through the neutral hardware cookie, and leave
the syscall result mapped into the caller's result register.  Libcall probes
verify a callee can execute a native AArch64 `brk #1` or RISC-V `ebreak`
strlen trap inside a neutral cross-call while preserving the x86_64 first
argument pointer and returning the result through the caller's native result
register.  Descriptor-import probes also verify a neutral callee can call
hardware-style `strlen`, `strnlen`, `memset`, `memcpy`, and three-argument
`memcmp` import descriptors with ordinary AArch64 `blr` or RISC-V `jalr`,
restore its native link register, mutate a shared x86 buffer through the
foreign memory path, and return through the same cross-frontend hardware cookie.  The gate also covers
a nested AArch64 -> RISC-V -> AArch64 call chain.
Synthetic AArch64/RISC-V register banks, the current poly mode, and hidden
hardware-style continuation state for `PCALL`, x86 import returns, and neutral
foreign cross-calls are lazily saved and restored per guest `CR3`, user
`FSBASE`, and either an explicit userspace poly state key or an 8 MiB-aligned
user stack-region fallback key in the Bochs prototype.
A normal x86_64 Linux process switch does not share foreign registers or
continuation cookies with another address space. Runtime or OS code should set
an explicit key when it needs deterministic per-thread banks; the stack-region
fallback keeps common pthread stacks separate when no explicit key is selected.
The low overlapping return/scratch values still use the current x86 register
bridge; this is not yet a full XSAVE-backed foreign register ABI. The prototype
can explicitly export/import the keyed state through the fixed 4096-byte
`struct poly_xsave_state` layout, and CPUID state bit `11` points software at
the formal leaf `0x40000004` hardware layout. The state is still not wired into
the guest OS `XSAVE`/`XRSTOR` context-switch path. The current interrupt prototype
covers ordinary long-mode
`IRET64`, `SYSRET`, `SYSEXIT`, and Linux signal-return paths into raw
userspace; the final implementation still needs to activate the architectural
XSAVE-visible foreign state component by setting state bit `7` and reporting the
component id in `0x40000003.ECX`/`0x40000001.EDX`.

After the OS-neutral trap packet is recorded, Bochs now always uses the same
architectural trap path that silicon or FPGA logic would expose.  It either
enters the configured architectural trap vector or raises an x86 `#UD` for
foreign syscall/import traps / `#BP` for foreign breakpoint traps if no vector
is installed.  There is no compatibility switch that re-enables Linux syscall
emulation or string-helper libcalls inside the CPU model.
`nativecheck.elf` verifies the no-vector signal behavior with `SIGILL` for
foreign syscall/import traps and `SIGTRAP` for foreign breakpoint traps.
Syscall translation, breakpoint handling, dynamic-linker binding, and libc
helper policy belong in firmware, the guest OS, the loader, or a userspace
runtime trap handler.  The CPU-facing contract is only the packet fields, trap
vector delivery, and trap return state restoration.

## Validation Gates

Build the Docker image:

```bash
make image
```

Run the static architecture-contract gates:

```bash
make check-poly-import-ids
make check-poly-arch-contract
make check-poly-cpuid-contract
```

Run the baseline x86_64 Linux boot:

```bash
make boot
```

Run the default hardware-style poly smoke test with guest trap-vector packet
handling:

```bash
make boot-poly
```

Run the OS-neutral architectural trap-vector gate:

```bash
make boot-poly-arch-traps
```

Run the low-level raw-mode probe with guest trap-vector packet handling:

```bash
make boot-poly-probe-arch-traps
```

Run the manifest-backed foreign payload suite with guest-side trap-vector
handling:

```bash
make boot-poly-apps-arch-traps
```

Run the trap-vector policy through guest `binfmt_misc` foreign ELF execution:

```bash
make boot-poly-binfmt-arch-traps
```

Run the generated cross-ISA `PCALL`/library-interop suite, including
thread-bank and signal-resume stress:

```bash
make boot-poly-call-arch-traps
```

Run the cross-ISA benchmark suite with a guest-installed trap-vector handler:

```bash
make boot-poly-bench-arch-traps
```

Run the full architectural-trap gate, combining guest trap-vector syscall/break
and import handling across probe, manifest apps, focused direct ELF execution,
PCALL, benchmark, thread/signal, and binfmt tests:

```bash
make boot-poly-full-arch-traps
```

`make boot-poly-full` is the unsuffixed alias for the same full gate.

Expected success markers include:

- `BOOT_OK`
- `NATIVE_POLY_NO_VECTOR_SIGNALS_OK`
- `NATIVE_POLY_TRAP_VECTOR_OK`
- `POLY_PROBE_OK`
- `POLYAPP_OK`
- `POLYEXEC_OK`
- `POLYCALL_OK`
- `POLYTHREAD_OK`
- `POLYBENCH_OK`
- `POLYBINFMT_OK`

## Known Gaps

- Foreign execution is still a Bochs prototype, not full native hardware
  decode.  The current hot path has a CPUID-gated `0f 24 ... POLY!`
  opcode-family placeholder decoded through `BX_IA_POLYMODE`.
- AArch64 and RISC-V ISA support is limited to the tested generated subset.
- Syscall and breakpoint traps are recorded explicitly as OS-neutral packets.
  Full foreign Linux ABI compatibility depends on guest trap-vector policy,
  loader support, and dynamic-linker integration rather than Bochs CPU policy.
- Equal-speed or minimal-slowdown execution is a design target.  The current
  implementation demonstrates raw direct-fetch execution and multi-burst raw
  loops, but not full equal-speed execution across complete ISAs.
- Foreign ELF support is limited to the generated static payload shape used by
  `tools/mkpolyelf.c`, `tools/polyapp.c`, and `tools/polyexec.c`, but both
  `polyapp` and `polyexec` load variable-size executable images up to 1 MiB.
  `polyexec` now preserves later `PT_LOAD` contents at their loaded image
  offsets so split text/data payloads keep page-relative data references, and
  it can run generated `ET_DYN` symbol entries with `RELA`, `REL`, and `RELR`
  relative relocations. It still does not provide a full Linux ELF loader or
  dynamic linker.
  `polyexec` preserves raw executable bytes, accepting 4-byte-aligned AArch64
  and 2-byte-aligned RISC-V entry segments for compressed-code compatibility;
  `riscv-compressed-half.elf` and `riscv-compressed-jalr.elf` verify RISC-V
  entry sizes that are not multiples of 4 bytes, including both compressed
  `c.jr` and 32-bit `jalr` returns to a halfword-aligned escape.
