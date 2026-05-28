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

## ISA Differences From x86_64

- x86_64 stays the system ISA for privilege, page tables, interrupts, faults,
  atomics, virtual memory, and TSO memory ordering.
- The poly extension adds frontend switches into raw AArch64 or RISC-V fetch.
  Foreign instructions are fetched directly; there is no per-instruction `#UD`
  envelope.
- Foreign code uses the same x86_64 virtual address space and page permissions.
- Cross-ISA calls bridge real ABIs: x86_64 SysV, AArch64 AAPCS64, and RISC-V
  psABI. This is compatibility glue, not a new `PolyFast` ABI.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and faults produce
  OS-neutral trap records carrying eight ABI arguments for runtime or OS
  policy.
- Foreign register state is intended to be explicit XSAVE-style architectural
  state. Hidden CR3-scoped emulator state is not the hardware contract.
- The Bochs prototype models hardware control instructions as a compact
  decoded `0f 24 <subop>` Poly Control Opcode Page. There is no magic trailer
  and no `#UD` envelope in the fast path.

Full architecture details are in `docs/poly-isa.md`.
