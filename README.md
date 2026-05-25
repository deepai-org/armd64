# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension.  The extension
keeps standard x86_64 execution as the host ISA, then adds synthetic userspace
mode-switch and foreign-instruction envelopes that let selected AArch64 and
RISC-V instruction streams run inside the x86_64 process.

This is an active scaffold, not a complete native-speed AArch64/RISC-V CPU.  The
current implementation validates the architecture shape, Linux boot path,
foreign ELF launch path, mixed-ISA transitions, deterministic syscalls, and
deterministic libcalls.  It does not yet implement full AArch64 or RISC-V ISA
coverage, real foreign Linux ABI passthrough, or equal-speed execution.

## Current State

- `scripts/boot.sh` downloads an Alpine Linux x86_64 kernel and BusyBox package,
  builds a minimal initramfs, creates a bootable ISO, and boots it in Bochs.
- The Docker image builds the local Bochs fork from
  `bochs-prepoly-src/bochs` and installs it as `bochs-poly`.
- The guest prints `BOOT_OK` on a clean baseline boot.
- With `POLY_ENABLED=1`, Bochs handles the polyglot userspace UD envelopes in
  `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`.
- `tools/polyprobe.c` validates direct mode switching, AArch64 and RISC-V
  call/return, nested cross-ISA call unwinding, instruction envelopes, mixed
  instruction streams, repeated mixed-mode switch stress, mixed libcalls, mixed
  syscalls, and status/counter markers.
- `tools/polyapp.c` runs manifest-backed generated foreign ELF64 payloads from
  `tools/polyapps/*.poly` by entering raw foreign mode, executing packed
  32-bit foreign instructions, and escaping back to x86_64.  The manifest path
  accepts variable-size executable segments up to 1 MiB.
- `tools/polyexec.c` runs generated foreign ELF64 payloads directly by path
  using the same raw-mode execution path, including executable segments larger
  than one raw burst.
- `tools/polybench.c` executes long raw AArch64 and RISC-V loops inside the
  guest, verifies that raw instruction counters advance across multiple
  fetch/decode bursts, and checks one mixed raw AArch64-to-RISC-V code blob.
- `tools/polybinfmt.sh` can register guest `binfmt_misc` entries so generated
  AArch64 and RISC-V ELF64 payloads execute directly from the x86_64 guest.

## ISA Changes From Standard x86_64

The Bochs fork treats selected userspace `#UD` byte sequences as polyglot CPU
operations when `POLY_ENABLED=1`.  Normal x86_64 instructions are unchanged.
The extension is intentionally encoded as invalid x86 byte sequences so an
unmodified CPU still traps.  The current handler accepts these envelopes only
from guest userspace.

All current poly operations are wrapped in fixed 8-byte envelopes:

| Operation | Bytes | Effect |
| --- | --- | --- |
| Switch to x86_64 mode | `64 0f 0b 58 4d 4f 44 45` | Sets current poly mode to x86_64. |
| Switch to AArch64 mode | `65 0f 0b 41 41 52 36 34` | Sets current poly mode to AArch64. |
| Switch to RISC-V mode | `66 0f 0b 52 49 53 43 56` | Sets current poly mode to RISC-V. |
| Switch to raw AArch64 mode | `65 0f 0b 52 41 57 36 34` | Sets current poly mode to raw AArch64; following bytes are fetched as fixed 32-bit AArch64 instructions. |
| Switch to raw RISC-V mode | `66 0f 0b 52 41 57 52 56` | Sets current poly mode to raw RISC-V; following bytes are fetched as fixed 32-bit RISC-V instructions. |
| Poly call AArch64 | `f2 0f 0b 43 41 4c 4c 41` | Enters AArch64 mode while pushing the caller mode for `poly ret`. |
| Poly call RISC-V | `f2 0f 0b 43 41 4c 4c 52` | Enters RISC-V mode while pushing the caller mode for `poly ret`. |
| Poly return | `f3 0f 0b 52 45 54 52 4e` | Pops and restores the caller mode. |
| Syscall status | `2e 0f 0b 53 59 53 43 <id>` | Returns syscall state in `RAX`: `0=current mode`, `1=last foreign syscall number`, `2=last foreign syscall mode`. |
| Libcall status | `3e 0f 0b 4c 49 42 43 <id>` | Returns libcall state in `RAX`: `0=current libcall status`, `1=last libcall number`, `2=last libcall mode`. |
| Switch/status counters | `4e 0f 0b 53 57 43 48 <id>` | Returns mode/counter state in `RAX`: `0=switches`, `1=current mode`, `2=foreign instruction envelopes`, `3=foreign syscalls`, `4=foreign libcalls`. |
| AArch64 instruction | `67 0f 0b <u32-le-insn> 00` | Decodes one supported AArch64 instruction when current mode is AArch64. |
| RISC-V instruction | `26 0f 0b <u32-le-insn> 00` | Decodes one supported RISC-V instruction when current mode is RISC-V. |

Raw foreign modes are the migration path away from one `#UD` envelope per
foreign instruction.  They still use the x86_64 switch envelope to enter from
x86 code, but then Bochs bypasses x86 decode and fetches fixed 32-bit foreign
instructions directly from `RIP`.  AArch64 `brk #0x7fff` and RISC-V custom-0
instruction `0x0000000b` escape back to x86_64 at the next byte.  Raw foreign
fetch is only active at guest CPL3; kernel, interrupt, and exception paths
continue through normal x86_64 decode even if the current userspace poly mode is
raw AArch64 or raw RISC-V.  Raw fetch is also bound to the guest CR3 active at
the raw-mode switch, so unrelated userspace tasks do not inherit raw decoding
after a scheduler switch or a fault in the raw-mode task.
The current raw run loop batches up to 64 raw foreign instructions before
returning to the outer Bochs event loop, while still checking async events and
mode exits between individual raw instructions.

The current register bridge is deliberately small:

- x86_64 `RAX` carries the foreign return value and maps to AArch64 `x0` or
  RISC-V `a0` for supported operations.
- x86_64 `RDI` is used as the shared scratch/base pointer and default AArch64
  `x2` / RISC-V `a2` value until a payload explicitly writes that foreign
  register.
- Bochs tracks array-backed synthetic foreign integer registers internally:
  AArch64 `x1`-`x30` with `x31` as zero, and RISC-V `x1`-`x31` with `x0` as
  zero.  The current syscall ABI uses AArch64 `x1`-`x5`, `x8` and RISC-V
  `a1`-`a5`, `a7`.

## Supported Foreign Subset

The current legacy AArch64 decoder supports the generated/probed subset used by
the tests: move-wide immediate `movz`, `movn`, and `movk`, decoded `add`/`sub`
immediate forms, decoded register-register `add`/`sub`/`mul`/`eor`/`and`/`orr`
over the synthetic register file, unconditional branch, `cbz`/`cbnz`, `ret`,
selected 64-bit `str`/`ldr`, `svc`, and `brk`.

The current legacy RISC-V decoder supports the generated/probed RV64 subset used by
the tests: decoded U-type `lui` and `auipc`, decoded OP-IMM `addi`, `xori`,
`ori`, `andi`, `slli`, `srli`, and `srai`, decoded register-register `add`,
`sub`, `mul`, `xor`, `and`, `or` over the synthetic register file,
`beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu`, `jal`, `jalr` return, selected 64-bit
`sd`/`ld`, `ecall`, and `ebreak`.

The raw-mode direct-fetch path covers the generated/probed subset used by
`polyapp`, `polyexec`, and `polybench`: the arithmetic, branch, return,
generic `jalr`, RV64 word arithmetic, generic byte/halfword/word/dword raw
AArch64 and RISC-V load-store forms, syscall, and libcall forms listed above,
plus native escapes.
`polyprobe` still exercises the legacy instruction envelopes for low-level
compatibility and uses raw mode for the direct-fetch smoke test.
`polybench` also validates the current efficient mixed-raw path: raw AArch64
escapes to x86_64 with `brk #0x7fff`, the next bytes immediately enter raw
RISC-V with the `RAWRV` envelope, and the RISC-V stream escapes with custom-0.

Foreign Linux syscall handling is deterministic and shared between AArch64
`svc` and RISC-V `ecall`.  Supported syscall numbers currently include:

- Scalar/process syscalls: `getpid`, `getppid`, `getuid`, `geteuid`, `getgid`,
  `getegid`, `gettid`, `getpgid(0)`, `getsid(0)`, and `exit`.
- File-style syscalls: `getcwd`, `read`, `write`, `openat`, `close`, `lseek`,
  and `uname`.
- Memory/time-style syscalls: `clock_gettime`, `getrusage`, `getcpu`,
  `gettimeofday`, `sysinfo`, and `mmap`.

The shared syscall dispatcher carries six foreign Linux ABI arguments for both
foreign architectures; current `mmap6` payloads verify argument registers beyond
`arg2` reach the dispatcher.

Foreign library calls are deterministic traps rather than real dynamic libc
calls:

- AArch64 uses `brk #id`.
- RISC-V uses `a7=id; ebreak`.
- Supported ids are `1=strlen`, `2=memfill`, `3=memcmp`, and `4=memcpy`.

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
- `POLYBENCH_OK`
- `POLYBINFMT_OK`

## Known Gaps

- Foreign execution is a Bochs UD-envelope prototype, not full native hardware
  decode.
- AArch64 and RISC-V ISA support is limited to the tested generated subset.
- Syscalls and libcalls return deterministic scaffold results, not complete host
  or guest Linux ABI behavior.
- Equal-speed or minimal-slowdown execution is a design target.  The current
  implementation demonstrates raw direct-fetch execution and multi-burst raw
  loops, but not full equal-speed execution across complete ISAs.
- Foreign ELF support is limited to the generated static payload shape used by
  `tools/mkpolyelf.c`, `tools/polyapp.c`, and `tools/polyexec.c`, but both
  `polyapp` and `polyexec` load variable-size executable segments up to 1 MiB.
