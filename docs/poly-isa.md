# Poly ISA

Poly is a CPU extension prototype that adds user-mode AArch64 and RISC-V64
frontends to an x86_64 machine. The goal is to run existing precompiled native
ABI code and link cross-ISA libraries in one virtual address space. It is not a
new compiler-only ABI and not a per-instruction trap scheme.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
```

Faster focused runs:

```sh
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Differences From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, atomics, VM control, and global TSO memory ordering.
- AArch64 and RISC-V64 are peer user-mode frontends fetching real native
  instructions from the same x86_64 virtual address space.
- Foreign instructions are decoded directly; the fast path does not use
  per-instruction `#UD`/`ud2` envelopes.
- AArch64 fetch is 4-byte aligned. RISC-V fetch is 2-byte aligned for RVC.
- Foreign memory accesses use the same virtual memory and permissions as x86_64.
- Foreign syscalls, breakpoints, illegal instructions, and recoverable exits
  produce precise trap packets for runtime or OS policy.

## Control Operations

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

| Operation | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted runtime/system code. |
| `PSWITCH frontend, target` | Branch to another frontend without return. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PLANDING` | Validate an indirect Poly target when landing policy is enabled. |
| `PTRAPRET` | Resume after a precise Poly trap. |

These are decoded control instructions. The Bochs prototype currently encodes
x86 controls on a compact `0f 3a fc <subop>` page, AArch64 controls in a
reserved HINT subspace, and RISC-V controls in a custom-0 opcode family.
Temporary encodings live in `tools/include/polycpuid.h`.

## ABI Boundary

- Compatibility targets ordinary native ABIs: x86_64 SysV, AArch64 AAPCS64,
  and RISC-V psABI.
- Register-only cross-ISA calls can use ABI signature slots intended to map to
  fixed-latency register-alias/RAT remaps in hardware.
- Stack arguments, variadic calls, large aggregates, incompatible vector
  layouts, lazy binding, and loader policy stay in software thunks.
- Ordinary native returns are the goal: x86_64 `ret`, AArch64 `ret x30`, and
  RISC-V `ret`/`jalr x0, ra, 0` return through Poly transition state when the
  call crossed frontends.

## Architectural State

- Non-x86 foreign registers are explicit per-thread XSAVE-style architectural
  state. They are not CR3-scoped hidden emulator state.
- Poly state includes the active frontend, foreign GPR/FP state, ABI signature
  slots, trap packet state, transition-stack state, frontend TLS state, and
  landing policy.
- The silicon-facing contract is OS-neutral: the CPU exposes state and precise
  exits; runtimes and kernels choose policy.

## More Detail

- Design rationale and priorities: `docs/poly-isa-design-directions.md`
- Shared constants and temporary encodings: `tools/include/polycpuid.h`
