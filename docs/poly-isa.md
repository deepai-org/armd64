# Poly ISA Quick Reference

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU. The goal is compatibility with existing precompiled code using real ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.

Poly is not a new compiler-only ABI. Fast paths exist, but they must still preserve native ABI behavior.

## Running Tests

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets:

```text
boot-poly-probe-arch-traps
boot-poly-call-arch-traps
boot-poly-thread-arch-traps
boot-poly-full-arch-traps
```

## Architectural Contract

| Area | Rule |
| --- | --- |
| System ISA | x86_64 owns boot, privilege, paging, interrupts, faults, and the memory model. |
| Frontend IDs | `0=x86_64`, `1=AArch64`, `2=RISC-V64`. |
| Foreign fetch | AArch64/RISC-V modes fetch native 32-bit instructions from normal virtual memory. |
| No envelopes | Foreign instructions are not wrapped in per-instruction x86 `#UD` envelopes. |
| Control ops | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`. |
| Calls | `PCALL` switches frontend, branches, records return state, and may apply a register-only ABI signature. |
| Returns | Native return instructions cross back through hardware return cookies. |
| State | Non-x86 state is explicit XSAVE-style Poly state, prototype component `20`. |
| Traps | Syscalls, imports, breakpoints, illegal instructions, and faults produce OS-neutral trap packets. |
| ABI boundary | Hardware may rename/remap registers only. Stack, aggregate, variadic, syscall, libcall, and memory-layout work stays in software. |

## Prototype Encodings

These encodings are temporary Bochs encodings, not final silicon encodings:

| Frontend | Temporary encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Prototype CPUID base: `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
