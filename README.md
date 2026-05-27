# Bochs Polyglot CPU Boot Harness

This repo boots a small x86_64 Linux guest under a modified Bochs and exercises
a prototype polyglot CPU extension. x86_64 remains the boot ISA. When
`POLY_ENABLED=1`, the Bochs fork exposes CPUID-gated instructions for entering
raw AArch64 or RISC-V frontends and for calling precompiled foreign functions
from x86_64 userspace.

The goal is compatibility with existing precompiled AArch64/RISC-V code linked
into an x86_64 process. This is not a new compiler-only ABI.

## Run

Build or rebuild the Docker image:

```bash
make image
```

Run a baseline x86_64 boot with the poly CPU feature hidden:

```bash
make boot
```

Run the default poly smoke test:

```bash
make boot-poly
```

Run the main focused gates:

```bash
make boot-poly-arch-traps
make boot-poly-probe-arch-traps
make boot-poly-apps-arch-traps
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-bench-arch-traps
make boot-poly-full-arch-traps
```

Use `make image` after changing `bochs-prepoly-src/`; the boot targets run the
Bochs binary baked into the image.

Useful outputs:

- `out/serial.log`: guest serial output and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.
- `tmp/`: initramfs and staging artifacts.

Clean generated artifacts:

```bash
make clean
```

## Test Targets

| Target | Purpose |
| --- | --- |
| `make boot` | Baseline x86_64 Linux boot; poly CPUID is expected to be hidden. |
| `make boot-poly` | Default poly smoke test. |
| `make boot-poly-arch-traps` | Foreign syscall/breakpoint traps become guest-handled architectural trap packets. |
| `make boot-poly-probe-arch-traps` | Low-level raw-mode frontend and trap-vector probe. |
| `make boot-poly-apps-arch-traps` | Manifest-backed AArch64/RISC-V raw payloads and direct ISA switches. |
| `make boot-poly-call-arch-traps` | Cross-ISA `PCALL`, foreign library loading, TLS, FP/int ABI bridges, threads, and signals. |
| `make boot-poly-binfmt-arch-traps` | Guest `binfmt_misc` execution path for selected foreign ELFs. |
| `make boot-poly-bench-arch-traps` | Mixed-mode raw loop and cross-ISA benchmark/probe coverage. |
| `make boot-poly-full-arch-traps` | Broad combined gate for the current prototype. |

The real guest boot tests are the primary signal. Static contract scripts still
exist as explicit `make check-*` targets, but they are not the default path.

## How The ISA Differs From x86_64

Normal x86_64 instructions are unchanged. The extension adds a separate
CPUID-gated polyglot execution facility:

- x86_64 remains the boot, kernel, and default userspace frontend.
- `PENTER.A64` switches instruction fetch/decode to fixed-width raw AArch64.
- `PENTER.RV64` switches instruction fetch/decode to raw RISC-V, including the
  tested compressed 16-bit subset.
- `PEXIT` switches back to x86_64.
- `PCALL.*.SYSV` bridges an x86_64 SysV caller into an AArch64 AAPCS64 or
  RISC-V psABI callee.
- Foreign traps do not become hidden Linux syscalls or libcalls inside the CPU;
  they become explicit architectural trap packets handled by guest software.

The Bochs prototype currently uses this placeholder x86 opcode family:

```text
0f 24 <op> 50 4f 4c 59 21
```

The trailing bytes spell `POLY!`. A real silicon or FPGA implementation should
use dedicated non-exception opcodes, not a `#UD` trap path.

## Current Opcode Summary

| Operation | Bytes | Effect |
| --- | --- | --- |
| `PEXIT` | `0f 24 00 50 4f 4c 59 21` | Return to x86_64 fetch. |
| `PENTER.A64` | `0f 24 01 50 4f 4c 59 21` | Enter raw AArch64 at the next byte. |
| `PENTER.RV64` | `0f 24 02 50 4f 4c 59 21` | Enter raw RISC-V at the next byte. |
| `PCALL.A64.SYSV` | `0f 24 10 50 4f 4c 59 21` | Call an AArch64 native-ABI target. |
| `PCALL.RV64.SYSV` | `0f 24 11 50 4f 4c 59 21` | Call a RISC-V native-ABI target. |
| `PCALL.*.SRET` | `0f 24 12/13 ...` | Native hidden-structure-return bridge forms. |
| `PCALL` FP/aggregate variants | `0f 24 14-1f ...` | Focused scalar FP and aggregate ABI bridge forms. |
| `PIRET` | `0f 24 20 50 4f 4c 59 21` | Return from an x86 helper import to saved foreign PC. |
| Trap vector ops | `0f 24 60-63 ...` | Install/read a guest trap vector and resume from traps. |
| State key ops | `0f 24 65-66 ...` | Set/read the userspace foreign-state bank key. |
| State export/import | `0f 24 67-68 ...` | Export/import prototype poly CPU state to a guest buffer. |

## Native Foreign Escapes

Raw foreign modes do not wrap each instruction in an x86 envelope. Foreign code
runs as normal 32-bit instruction words until it branches, traps, returns, or
uses one of the native escape encodings below.

| Foreign instruction | Effect |
| --- | --- |
| AArch64 `brk #0x7fff` | Exit raw AArch64 to x86_64. |
| AArch64 `brk #0x7ffe` | Switch directly from raw AArch64 to raw RISC-V. |
| AArch64 `brk #0x7ffd` and variants | Cross-call from AArch64 to RISC-V. |
| RISC-V custom-0 `0x0000000b` | Exit raw RISC-V to x86_64. |
| RISC-V custom-1 `0x0000002b` | Switch directly from raw RISC-V to raw AArch64. |
| RISC-V custom-2/custom-3 variants | Cross-call from RISC-V to AArch64. |

Cross-ISA calls use native return cookies in AArch64 `x30` or RISC-V `ra`, so
ordinary foreign return instructions can cross back through the mode boundary.

## ABI And State Contract

The bridge targets ordinary native ABIs:

- x86 SysV integer args in `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to
  AArch64 `x0`-`x5` or RISC-V `a0`-`a5`.
- Stack arguments are read from the x86 SysV stack and presented according to
  AAPCS64 or RISC-V psABI rules.
- x86 `RAX` receives the primary integer return value from AArch64 `x0` or
  RISC-V `a0`.
- x86 `XMM0`-`XMM7` low lanes bridge to AArch64 scalar `s/d0`-`s/d7` and
  RISC-V `fa0`-`fa7` for the implemented scalar FP subset.
- Non-aliased foreign registers are prototype-managed by Bochs today. A
  hardware implementation needs an architectural save/restore contract such as
  CPUID plus an XCR0/XSAVE component.

Foreign memory operations use the same virtual-memory path as x86_64. The
prototype defines the mixed memory model as x86 TSO, so AArch64 barriers and
RISC-V fences decode to x86-compatible ordering behavior.

## Key Files

- `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`: Bochs CPU-side poly opcode,
  frontend, trap, state, and ABI bridge implementation.
- `scripts/boot.sh`: builds the guest image and launches Bochs in Docker.
- `tools/polyprobe.c`: low-level raw mode and trap-vector probe.
- `tools/polyapp.c`: manifest-backed raw payload runner.
- `tools/polyexec.c`: direct foreign ELF runner.
- `tools/polycall.c`: user-space loader/runtime for calling foreign ELF
  functions through `PCALL`.
- `tools/polythread.c` and `tools/polysignal.c`: pthread/signal checks for
  foreign state isolation and interrupted raw-mode resume.
- `docs/poly-isa.md`: longer architecture and silicon-oriented ISA notes.

## Known Limits

- AArch64 and RISC-V instruction coverage is still a tested subset.
- Equal-speed execution is a design target, not a demonstrated result.
- The opcode family is a Bochs placeholder, not a finalized silicon encoding.
- Foreign state is not yet a real XSAVE/XCR0 component.
- `polycall` is not a complete Linux dynamic linker.
