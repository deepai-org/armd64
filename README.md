# armd64

Bochs-based x86_64 VM prototype for running precompiled AArch64 and RISC-V64
userspace code in the same virtual address space.

The goal is compatibility with real native ABI objects and shared libraries,
not a new compiler-only ABI and not one trap per foreign instruction.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `BOOT_TIMEOUT_SECONDS=900 make <target>` for long gates.

Primary targets:

- `make boot`: plain x86_64 sanity boot.
- `make boot-poly`: shorter Poly smoke/regression run.
- `make boot-poly-focused-validation`: focused nativecheck architecture,
  neutral-switch, cross-ISA shared-library, syscall/import monitor,
  real-XSAVE call/thread/signal, and benchmark trap-delta validation.
- `make boot-poly-full-real-xsave-arch-traps`: broad regression with the guest
  Poly XCR0 module loaded, requiring direct XSAVE/XRSTOR state handling.

Focused targets exist for nativecheck, probe, app payload, neutral switching,
process loader, syscall/trap, call/thread/signal, benchmark, and binfmt
coverage. See the `boot-poly-*` targets in `Makefile` when debugging a
specific subsystem.

## Status

- The prototype boots an Alpine-based x86_64 initramfs under Bochs.
- It runs native x86 checks plus AArch64/RISC-V raw execution, foreign traps,
  direct AArch64<->RISC-V transitions, cross-ISA calls, threads, signals,
  benchmarks, and binfmt smoke tests.
- The compatibility subset includes integer, FP, fixed 128-bit vector,
  atomics, TLS, ELF relocation, dynamic linking, and import/syscall trap paths.
- It is not a complete implementation of every AArch64/RISC-V extension or
  every possible precompiled binary. Unsupported foreign instructions produce
  architectural trap records.
- Bochs is a functional ISA prototype. Few-cycle frontend switching is the
  hardware target, not something Bochs can prove cycle-accurately.

## ISA Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, virtual memory, syscalls, and global memory ordering.
- AArch64 and RISC-V64 are peer user-mode decode frontends fetched from the
  same `RIP` address space.
- Frontend switches are decoded control instructions. There is no
  per-instruction `#UD` envelope in the fast path.
- AArch64 uses fixed 32-bit fetch. RISC-V64 supports 16/32-bit fetch, including
  RVC.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI. This is compatibility glue, not a `PolyFast` ABI.
- Register-only calls can use ABI signature slots and integer/FP exchange
  windows suitable for hardware register renaming. Stack arguments,
  memory-shaped aggregates, variadics, lazy binding, libc policy, and syscall
  policy stay in software.
- Foreign register state is explicit per-thread XSAVE-style architectural
  state. Hidden CR3-scoped emulator state is not the hardware contract.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  OS-neutral trap records for a runtime or OS handler.
- AArch64 and RISC-V64 can switch or call each other directly without bouncing
  through x86_64.

## Docs

- Quick ISA reference: `docs/poly-isa.md`
- Hardware and ABI direction: `docs/poly-isa-design-directions.md`
