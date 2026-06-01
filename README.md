# armd64

Bochs-based x86_64 VM prototype for running precompiled AArch64 and RISC-V
userspace code in the same x86_64 virtual address space.

The goal is compatibility with real native ABI objects and shared libraries,
not a new compiler-only ABI and not one trap per foreign instruction.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|NATIVE_CHECK_OK|NATIVE_POLY_REAL_XSAVE_OK|POLY_PROBE_OK|POLYAPP_OK|POLY_NEUTRAL_OK|POLY_EXEC_CROSS_OK|POLY_EXEC_SYSCALL_OK|POLY_EXEC_BLOCK_OK|POLY_ARCH_TRAP_EXEC_OK|POLYCALL_OK|POLYTHREAD_REAL_XSAVE_CONTEXT_OK|POLYTHREAD_REAL_XSAVE_NO_KEY_OK|POLYSIGNAL_REAL_XSAVE_CONTEXT_OK|POLYSIGNAL_REAL_XSAVE_NO_KEY_OK|POLYBENCH_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `BOOT_TIMEOUT_SECONDS=900 make <target>` for long focused gates.

Other useful targets:

- `make boot`: plain x86_64 sanity check.
- `make boot-poly`: shorter raw execution, neutral-switch, process-loader, and
  syscall/trap regression run.
- `make boot-poly-arch-traps`: raw foreign trap/syscall tests.
- `make boot-poly-real-xsave-arch-traps`: raw trap/syscall tests with the guest
  Poly XCR0 module loaded.
- `make boot-poly-probe-arch-traps`: low-level Poly ISA and CPUID probes.
- `make boot-poly-apps-arch-traps`: hand-authored raw frontend app payloads.
- `make boot-poly-neutral-arch-traps`: direct AArch64<->RISC-V generic switch
  and call tests.
- `make boot-poly-exec-cross-arch-traps`: focused real ELF/shared-library
  cross-frontend process-loader tests.
- `make boot-poly-exec-syscall-arch-traps`: focused real process syscall tests
  through OS-neutral trap packets.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-call-real-xsave-arch-traps`: focused cross-ISA call, thread,
  and signal run with the guest Poly XCR0 module loaded, so real
  XSAVE/XRSTOR-backed context switching is required.
- `make boot-poly-thread-arch-traps`: focused thread/signal state tests.
- `make boot-poly-bench-arch-traps`: switch and raw-instruction-count
  benchmarks.
- `make boot-poly-binfmt-arch-traps`: loader/binfmt smoke tests.
- `make boot-poly-full-arch-traps`: broad regression run, including focused
  process cross-call and syscall gates.
- `make boot-poly-full-real-xsave-arch-traps`: broad regression run with the
  guest Poly XCR0 module loaded so direct XSAVE/XRSTOR is required.

## Current Prototype Status

- The broad regression target boots an Alpine-based x86_64 initramfs under
  Bochs and runs native x86 checks plus AArch64/RISC-V raw execution,
  syscalls/traps, direct AArch64<->RISC-V generic transitions, cross-ISA calls,
  threads, signals, benchmarks, and binfmt smoke tests.
- The Bochs prototype implements a large compatibility subset for compiled
  AArch64 and RISC-V userspace, including integer, FP, fixed 128-bit vector,
  atomics, TLS, ELF relocation, dynamic linking, and import/syscall trap paths.
- It is not yet a complete implementation of every AArch64/RISC-V extension or
  every possible precompiled binary. Unsupported foreign instructions still
  produce architectural trap records.
- The current performance evidence is Bochs-level switch and instruction-count
  benchmarking. "Few-cycle" frontend switching is the hardware target; Bochs is
  a functional ISA prototype, not a cycle-accurate silicon performance model.
- Bochs exposes Poly as XSAVE component 20 and implements XSAVE/XRSTOR handlers.
  The default broad tests still use explicit Poly state save/restore controls
  for prototype scheduling and signal coverage. `make
  boot-poly-full-real-xsave-arch-traps` additionally builds and inserts a small
  guest kernel module that enables the Poly XCR0 component for direct
  XSAVE/XRSTOR validation in the broad regression run.
- The public CPUID contract exposes XSAVE state plus an explicit architectural
  state-key control. Bochs still uses CR3/FSBASE and a stack-region fallback
  internally to isolate prototype register banks, but those emulator fallback
  selectors are not public CPUID state bits and are not part of the
  silicon-facing ABI.

## ISA Differences From x86_64

- x86_64 stays the system ISA for privilege, page tables, interrupts, faults,
  atomics, virtual memory, and TSO memory ordering.
- The poly extension adds generic frontend-ID controls for entering,
  switching, and calling x86_64, AArch64, and RISC-V frontends. AArch64 and
  RISC-V can switch or call each other directly without bouncing through x86.
  Foreign instructions are fetched directly; there is no per-instruction `#UD`
  envelope.
- Frontend fetch uses native ISA widths: AArch64 is 4-byte aligned, while
  RISC-V uses 2-byte alignment so compressed RVC instructions are legal.
- Foreign code uses the same x86_64 virtual address space and page permissions.
- Cross-ISA calls bridge real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI. This is compatibility glue, not a new `PolyFast` ABI.
- Fast calls can use neutral native-register ABI signature slots and integer/FP
  exchange windows. Fixed 128-bit vector calls and compact scalar aggregates can
  also stay on the direct path when they map to register-only signatures.
  Signature slots expose fixed register-map IDs for hardware rename/RAT
  implementations. Stack arguments, large or memory-shaped aggregates,
  variadics, ABI reshaping, and loader policy stay in software.
- Hidden structure-return pointers can use a dedicated register-only SRET
  signature slot when no stack reshaping is required. Structure-return cases
  that need stack argument staging or aggregate layout work still use thunks.
- AArch64 HFA-of-float returns/arguments use precise register-only signature
  features. AArch64 HFA-of-double returns use precise signature features to
  fetch the foreign FP result registers, then ordinary x86_64 SysV callers use
  a small hidden-sret post-copy because those structs are memory-returned on
  x86_64. HFA arguments that are stack-backed on the source ABI still use
  thunks because they require memory-shaped ABI work.
- Direct x86 calls expose the source frontend stack pointer in volatile `R11`
  so user-space thunks can marshal overflow stack arguments without CPU
  descriptor parsing. For foreign-to-x86 TLS, the direct call transition
  installs the process TLS base as x86 `FSBASE` only for the callee window and
  restores the caller/runtime `FSBASE` on return, avoiding generated x86 TLS
  wrappers on register-only calls.
- The current Bochs runtime keeps direct `PCALL` stack handling and
  structure-return stack argument handling in software thunks.
- Runtime-backed import calls are a Bochs compatibility path with a private
  loader table layout. CPUID keeps descriptor hardware support
  reserved/forbidden; reserved import targets trap instead of making the CPU
  parse user-memory descriptors.
- Hot signature calls encode the slot in the control instruction using fixed
  subopcode ranges, so callers do not need an extra temporary-register move or
  a decoder-visible immediate fetch before the frontend switch.
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

Quick ISA reference: `docs/poly-isa.md`. Design rationale and future directions: `docs/poly-isa-design-directions.md`.
