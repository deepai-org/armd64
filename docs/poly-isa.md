# Poly ISA

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused tests:

```text
boot-poly-probe-arch-traps
boot-poly-call-arch-traps
boot-poly-thread-arch-traps
boot-poly-full-arch-traps
```

## Contract

Poly adds AArch64 and RISC-V64 userspace frontends to an x86_64 system CPU. It targets existing precompiled SysV x86_64, AAPCS64, and RISC-V psABI code, not a new compiler-only ABI.

| Area | Rule |
| --- | --- |
| System model | x86_64 owns boot, privilege, paging, interrupts, faults, and memory ordering. |
| Frontend IDs | `0=x86_64`, `1=AArch64`, `2=RISC-V64`. |
| Foreign fetch | AArch64/RISC-V fetch native 32-bit instructions directly from normal virtual memory. |
| Control ops | `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`. |
| Calls/returns | `PCALL` switches frontend and records return state; native returns cross back through return cookies. |
| Fast ABI mapping | Hardware may rename/remap registers only, using ABI signature slots. |
| Software ABI work | Stack args, aggregates, variadics, syscalls, libcalls, and memory layout are handled by software thunks/monitors. |
| State | Non-x86 registers are explicit XSAVE-style Poly state, prototype component `20`. |
| Traps | Syscalls, imports, breakpoints, illegal instructions, and faults produce OS-neutral trap packets. |

## Difference From x86_64

Poly does not replace x86_64. It adds selectable userspace instruction frontends sharing the same virtual address space and x86-owned system behavior. Foreign instructions are not encoded as per-instruction x86 `#UD` envelopes; once a foreign frontend is entered, the CPU fetches that ISA directly until a control transfer, return cookie, or trap packet exits it.

## Prototype Encodings

Temporary Bochs encodings, not final silicon encodings:

| Frontend | Temporary encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Prototype CPUID base: `0x40000000`.

Design rationale: `docs/poly-isa-design-directions.md`.
