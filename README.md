# Bochs Polyglot CPU Boot Harness

This repository boots a small x86_64 Linux userspace under a modified Bochs and
uses that guest to exercise a prototype polyglot CPU extension. Standard x86_64
execution remains the boot and host ISA. When enabled, the Bochs fork adds a
CPUID-gated opcode family that can enter direct-fetch AArch64 or RISC-V modes,
call precompiled foreign functions through native ABI bridges, and route
foreign traps to guest userspace instead of embedding Linux policy in the CPU.

This is a prototype, not a complete native-speed implementation. The current
work validates the architecture shape, Linux boot path, raw foreign frontend
switching, mixed AArch64/RISC-V execution, foreign ELF loading, `PCALL`
library interop, thread/signal state isolation, and OS-neutral trap packets.
Full AArch64/RISC-V ISA coverage, equal-speed execution, and a full production
foreign dynamic linker are still open goals.

## Quick Start

Build the Docker image:

```bash
make image
```

Boot baseline x86_64 Linux in Bochs with poly CPUID hidden:

```bash
make boot
```

Run the default poly smoke test:

```bash
make boot-poly
```

Run the focused gates:

```bash
make boot-poly-arch-traps
make boot-poly-probe-arch-traps
make boot-poly-apps-arch-traps
make boot-poly-call-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-bench-arch-traps
make boot-poly-full-arch-traps
```

Useful outputs:

- `out/serial.log`: guest serial output and test pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.
- `tmp/`: initramfs and staging artifacts.

Clean generated artifacts:

```bash
make clean
```

## Main Test Gates

| Target | Purpose |
| --- | --- |
| `make boot` | Baseline x86_64 Linux boot. Verifies ordinary x86 userspace still works and poly CPUID is hidden. |
| `make boot-poly` | Default hardware-style smoke test: native check, low-level probe, manifest apps, direct ELF execution, and trap-vector execution. |
| `make boot-poly-arch-traps` | Runs generated AArch64/RISC-V ELF payloads where foreign syscalls and breakpoints exit as architectural trap packets handled by guest x86 userspace. |
| `make boot-poly-probe-arch-traps` | Low-level raw-mode probe with guest-installed trap-vector handling. |
| `make boot-poly-apps-arch-traps` | Manifest-backed raw payloads, including AArch64-to-RISC-V and RISC-V-to-AArch64 direct-switch app payloads. |
| `make boot-poly-call-arch-traps` | Cross-ISA `PCALL` and library interop suite: relocations, `DT_NEEDED`, TLS, FP/int ABI bridges, x86 import descriptors, thread-bank, and signal-resume checks. |
| `make boot-poly-binfmt-arch-traps` | Registers guest `binfmt_misc` entries and executes selected foreign ELF payloads directly from the x86_64 guest. |
| `make boot-poly-bench-arch-traps` | Mixed-mode raw loop and cross-ISA benchmark/probe coverage with guest trap-vector policy. |
| `make boot-poly-full-arch-traps` | Broad combined gate for the current prototype. |

The primary confidence signal is the real guest boot tests above. Static
contract checks are still available through the explicit `make check-*` targets
or by running boot targets with `RUN_CONTRACT_CHECKS=1`, but they are not on the
default boot path.

## Source Layout

- `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`: Bochs CPU-side poly opcode,
  raw frontend, trap, state, and ABI bridge implementation.
- `scripts/boot.sh`: builds the Alpine-based x86_64 initramfs/ISO and launches
  Bochs in Docker.
- `tools/polyprobe.c`: low-level raw mode and trap-vector probe.
- `tools/polyapp.c`: manifest-backed raw AArch64/RISC-V payload runner.
- `tools/polyexec.c`: direct foreign ELF runner, including selected `ET_DYN`
  symbol entry support.
- `tools/polycall.c`: user-space loader/runtime for calling foreign ELF
  functions through `PCALL`.
- `tools/polybinfmt.sh`: guest `binfmt_misc` registration helper.
- `tools/polythread.c` and `tools/polysignal.c`: real pthread/signal stress
  checks for foreign state isolation and interrupt/signal resume.
- `docs/poly-isa.md`: longer silicon-oriented ISA notes.

## ISA Changes From Standard x86_64

Normal x86_64 instructions are unchanged. The extension is active only when the
Bochs fork is run with `POLY_ENABLED=1`, and the prototype handler accepts the
new operations only from guest userspace.

The current hot path uses a placeholder x86 opcode family:

```text
0f 24 <op> 50 4f 4c 59 21
```

The trailing bytes spell `POLY!`. In real silicon or FPGA this should become a
dedicated non-exception opcode allocation; it should not be implemented as a
`#UD` trap path.

### Execution Modes

- `x86_64`: normal boot and host mode.
- `raw AArch64`: fixed 32-bit AArch64 fetch/decode from `RIP`.
- `raw RISC-V`: RISC-V fetch/decode from `RIP`, including tested compressed
  16-bit instruction support.

Mode switches stop the current trace/block and route subsequent fetches to the
selected frontend. Raw modes use the same Bochs virtual memory path as x86, so
page faults and permissions are shared with the x86 guest process.

### x86 Opcode Family

| Operation | Bytes | Effect |
| --- | --- | --- |
| `PEXIT` | `0f 24 00 50 4f 4c 59 21` | Switch current frontend back to x86_64. |
| `PENTER.A64` | `0f 24 01 50 4f 4c 59 21` | Enter raw AArch64 at the next byte. |
| `PENTER.RV64` | `0f 24 02 50 4f 4c 59 21` | Enter raw RISC-V at the next byte. |
| `PCALL.A64.SYSV` | `0f 24 10 50 4f 4c 59 21` | x86 SysV call into AArch64. `R10` holds target, `R11` holds x86 return PC, optional `R13` holds foreign TLS base. |
| `PCALL.RV64.SYSV` | `0f 24 11 50 4f 4c 59 21` | x86 SysV call into RISC-V with the same target/return/TLS convention. |
| `PCALL.*.SRET` | `0f 24 12/13 ...` | Hidden-structure-return variants for AArch64/RISC-V native ABIs. |
| `PCALL` FP/aggregate variants | `0f 24 14-1f ...` | Focused bridges for common FP, homogeneous aggregate, heterogeneous aggregate, compact RISC-V aggregate, and FP overflow-stack cases. |
| `PIRET` | `0f 24 20 50 4f 4c 59 21` | Return from a descriptor-backed x86 helper import to the saved foreign return PC. |
| Trap vector ops | `0f 24 60-63 ...` | Install/read trap-vector mode and return from a guest-handled foreign trap. |
| State key ops | `0f 24 65-66 ...` | Set/read the userspace key used to select synthetic foreign register banks. |
| State export/import | `0f 24 67-68 ...` | Prototype fixed-layout export/import of current poly state into a guest buffer. |

The CPUID leaves under `0x40000000+` advertise the prototype feature bits,
opcode support, trap packet layout, import descriptor layout, and current state
contract. The current state leaf reports no real XCR0 component yet; foreign
state is still prototype-managed by Bochs.

### Native Foreign Escapes

Raw foreign modes do not require x86 envelopes per instruction.

| Foreign instruction | Effect |
| --- | --- |
| AArch64 `brk #0x7fff` | Exit raw AArch64 to x86_64. |
| AArch64 `brk #0x7ffe` | Switch directly from raw AArch64 to raw RISC-V. |
| AArch64 `brk #0x7ffd` and nearby variants | Cross-call from AArch64 to RISC-V with selected ABI bridge forms. |
| RISC-V custom-0 `0x0000000b` | Exit raw RISC-V to x86_64. |
| RISC-V custom-1 `0x0000002b` | Switch directly from raw RISC-V to raw AArch64. |
| RISC-V custom-2/custom-3 variants | Cross-call from RISC-V to AArch64 with selected ABI bridge forms. |

Cross-ISA calls use native return cookies in AArch64 `x30` or RISC-V `ra`, so
ordinary foreign return instructions can return across the mode boundary.

### Register And ABI Bridge

The bridge targets compatibility with ordinary precompiled AArch64/RISC-V code,
not a custom compiler ABI.

- x86 `RAX` carries the primary return value and maps to AArch64 `x0` or
  RISC-V `a0` on return.
- x86 SysV arguments in `RDI`, `RSI`, `RDX`, `RCX`, `R8`, and `R9` map to
  AArch64 `x0`-`x5` or RISC-V `a0`-`a5` for `PCALL`.
- Additional integer arguments are read from the x86 SysV stack and presented
  according to AAPCS64 or RISC-V psABI rules.
- x86 `XMM0`-`XMM7` low lanes map to AArch64 scalar `s/d0`-`s/d7` and RISC-V
  `fa0`-`fa7` for the currently implemented scalar FP subset.
- The bridge saves x86 `RSP`, exposes a separate 16-byte-aligned foreign stack
  window during `PCALL`, and restores x86 `RSP` when the native foreign return
  hits its return cookie.
- The user-space `polycall` stub preserves x86 SysV callee-saved GPRs without
  changing `RSP`, keeping stack-argument offsets stable for hardware bridge
  decoding.

### Traps, Syscalls, And OS Neutrality

Foreign `svc`, `ecall`, breakpoints, unsupported instructions, and selected
exceptions are represented as explicit architectural trap packets. The CPU
records source mode, reason, number/selector, PC, resume PC, and argument
registers, then transfers to a guest-installed x86 trap vector.

The Bochs CPU model should not implement Linux syscalls or libc helpers as
hidden policy. Guest userspace handlers translate the packet into normal x86
Linux syscalls or runtime helper calls, then resume the original raw frontend
with the trap-return operation.

### State, Threads, And Interrupts

The prototype maintains non-aliased foreign registers in synthetic banks keyed
by guest address-space/thread-related state plus an explicit userspace key or a
stack-region fallback. `polythread` and `polysignal` exercise this across real
guest pthreads, timer signals, and raw-mode resume paths.

For a hardware implementation, this needs to become an architectural state
contract: CPUID feature discovery, an XCR0/XSAVE component or equivalent OS
save area, and precise interrupt/`IRET` semantics for resuming the interrupted
foreign frontend.

### Memory Model

The prototype defines the hybrid memory model as x86 TSO. AArch64 barriers and
RISC-V fences are decoded as ordering-preserving no-ops or mapped to stronger
x86-compatible behavior. Foreign loads, stores, atomics, and page faults use
the same Bochs virtual memory machinery as x86.

### Dynamic Loading And Runtime

`polycall` is the current user-space loader/runtime for precompiled foreign
shared objects. It handles selected ELF64 payloads, dynamic relocations,
`DT_NEEDED` dependency loading, TLS, constructors/destructors, IFUNC-style
cases, x86 import descriptors, libc-style imports, FP/int ABI variants, and
atomic helper descriptors. This is intentionally software-defined runtime
policy, not hidden CPU behavior.

`polyexec` and `polybinfmt` cover direct foreign executable entry paths. The
`binfmt_misc` path lets selected generated AArch64/RISC-V ELFs execute from the
x86_64 guest as ordinary programs.

## Known Gaps

- AArch64 and RISC-V instruction coverage is still a tested subset, not full
  architecture coverage.
- Equal-speed execution is a design target, not a demonstrated result.
- The opcode family is a Bochs prototype placeholder, not a finalized silicon
  encoding.
- Foreign state is not yet exposed as a real XSAVE/XCR0 component.
- The foreign dynamic loader is focused on generated/precompiled test fixtures;
  it is not a complete Linux dynamic linker.
- Same-ISA `DT_NEEDED` foreign dependencies are covered; true cross-foreign
  shared-library dependencies still need explicit loader/runtime design.
- Hardware interrupt and exception contracts are documented directionally, but
  not finalized as a production ISA specification.
