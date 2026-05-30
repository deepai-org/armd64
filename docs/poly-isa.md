# Poly ISA

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU. The compatibility target is existing native ABI code: SysV x86_64, AAPCS64, and RISC-V psABI. It is not a compiler-only PolyFast ABI.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `boot-poly-probe-arch-traps`, `boot-poly-call-arch-traps`, `boot-poly-thread-arch-traps`, `boot-poly-full-arch-traps`.

## Contract

| Area | Contract |
| --- | --- |
| x86_64 | Owns boot, privilege, paging, interrupts, faults, and TSO ordering. |
| Frontends | `0=x86_64`, `1=AArch64`, `2=RISC-V64`. |
| Fetch | Foreign modes fetch native 32-bit instructions from the shared virtual address space; no per-instruction `#UD` envelopes. |
| Control | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`. |
| Calls | `PCALL` switches frontend, branches, records cross-return state, and may apply a register-only ABI signature. |
| Returns | Native returns cross back through hardware return cookies. |
| State | Foreign architectural state is explicit XSAVE-style Poly state, prototype component `20`. |
| Traps | Syscalls, imports, breakpoints, illegal instructions, and faults emit OS-neutral trap packets. |
| ABI | Hardware remaps registers only, including the first eight native foreign ABI argument registers; stack, aggregate, variadic, syscall, libcall, and memory-side ABI work stays in software. |

## Prototype

Temporary Bochs encodings: x86_64 `0f 3a fc <subop>`, AArch64 `0xd503201f | ((subop & 0x7f) << 5)`, RISC-V64 `0x0000700b | ((subop & 0x7f) << 25)`. Prototype CPUID base: `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
