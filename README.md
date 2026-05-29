# armd64

Bochs-based x86_64 VM prototype for running precompiled AArch64 and RISC-V
userspace code in the same x86_64 virtual address space.

The goal is compatibility with real native ABI objects and shared libraries,
not a new compiler-only ABI and not one trap per foreign instruction.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-binfmt-arch-traps
grep -a -E 'POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Other useful targets:

- `make boot`: plain x86_64 sanity check.
- `make boot-poly-arch-traps`: raw foreign trap/syscall tests.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-full-arch-traps`: broad regression run.

## Current Prototype Status

- The broad regression target boots an Alpine-based x86_64 initramfs under
  Bochs and runs native x86 checks plus AArch64/RISC-V raw execution,
  syscalls/traps, cross-ISA calls, threads, signals, benchmarks, and binfmt
  smoke tests.
- The Bochs prototype implements a large compatibility subset for compiled
  AArch64 and RISC-V userspace, including integer, FP, fixed 128-bit vector,
  atomics, TLS, ELF relocation, dynamic linking, and import/syscall trap paths.
- It is not yet a complete implementation of every AArch64/RISC-V extension or
  every possible precompiled binary. Unsupported foreign instructions still
  produce architectural trap records.
- The current performance evidence is Bochs-level switch and instruction-count
  benchmarking. "Few-cycle" frontend switching is the hardware target; Bochs is
  a functional ISA prototype, not a cycle-accurate silicon performance model.

## ISA Differences From x86_64

- x86_64 stays the system ISA for privilege, page tables, interrupts, faults,
  atomics, virtual memory, and TSO memory ordering.
- The poly extension adds frontend switches into raw AArch64 or RISC-V fetch.
  Foreign instructions are fetched directly; there is no per-instruction `#UD`
  envelope.
- Foreign code uses the same x86_64 virtual address space and page permissions.
- Cross-ISA calls bridge real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI. This is compatibility glue, not a new `PolyFast` ABI.
- Fast calls can use neutral native-register ABI signature slots and integer/FP
  exchange windows. Fixed 128-bit vector calls can also stay on the direct path
  when both ABIs use SIMD/vector registers. Stack, aggregate, variadic, ABI
  reshaping, and loader policy stays in software.
- Direct x86 calls expose the source frontend stack pointer in volatile `R11`
  so user-space thunks can marshal overflow stack arguments without CPU
  descriptor parsing.
- Descriptor-backed import calls are a Bochs/runtime compatibility path, not a
  CPUID-advertised silicon feature.
- Hot signature calls encode the slot in the control instruction where
  possible, so foreign callers do not need an extra temporary-register move
  before the frontend switch.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  OS-neutral trap records carrying eight ABI arguments for runtime or OS
  policy.
- Foreign register state is intended to be explicit XSAVE-style architectural
  state. Hidden CR3-scoped emulator state is not the hardware contract.
- Optional landing-pad policy can require marked indirect `PSWITCH`/`PCALL`
  targets; the policy is explicit Poly XSAVE state.
- The Bochs prototype models hardware control instructions as a compact
  decoded `0f 3a fc <subop>` Poly Control Opcode Page. There is no magic trailer
  and no `#UD` envelope in the fast path.
- Foreign-to-foreign and foreign-to-x86 transitions are also decoded control
  instructions: AArch64 uses a reserved HINT subspace and RISC-V uses one
  custom-0 opcode family. Breakpoint instructions remain traps, not fast mode
  switches.

Full architecture details are in `docs/poly-isa.md`.
