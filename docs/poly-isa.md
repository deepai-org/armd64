# Poly ISA Reference

Poly lets existing x86_64, AArch64, and RISC-V64 user-mode code run in one
x86-owned virtual address space. This file is the short ISA contract; design
rationale lives in `docs/poly-isa-design-directions.md`.

## Architectural Model

- x86_64 owns boot, privilege, paging, interrupts, atomics, syscalls, and TSO.
- AArch64 and RISC-V64 are ring-3 decode frontends over x86_64 memory.
- Foreign code is fetched directly, not wrapped per instruction in `#UD`.
- Foreign register state is per-thread XSAVE-style architectural state.
- Compatibility targets native ABIs: SysV x86_64, AAPCS64, and RISC-V psABI.

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

## Prototype Encodings

```text
x86_64:   0f 3a fc <subop>
AArch64:  0xd503201f | ((subop & 0x7f) << 5)   // reserved HINT
RISC-V64: 0x0000700b | ((subop & 0x7f) << 25)  // custom-0
```

Core subops:

| Subop | Operation |
| --- | --- |
| `0x03` | `PENTER mode` |
| `0x04` | `PSWITCH mode,target` |
| `0x05` | `PLANDING` |
| `0x2d` | `PCALL mode,target,signature` |
| `0x30..0x3c` | `PCALL` with immediate signature slot |
| `0x62` | `PTRAPRET` |
| `0x65..0x6e` | state key, ABI signature, monitor packet, landing policy |

Foreign frontends expose the same control surface.

## Calls, Returns, And Traps

`PCALL` is a hardware mode-switching call. It records caller state in a hardware
transition stack, installs a reserved return cookie, applies the selected
register-only ABI signature, and jumps to the target frontend.

Ordinary native returns to the cookie restore the caller frontend and PC:
x86_64 `ret`, AArch64 `ret x30`, and RISC-V64 `ret` / `jalr x0, ra, 0`.

ABI signatures only remap registers. Stack arguments, variadics, by-value
aggregates, lazy binding, libc policy, and syscall policy are handled by
loader/runtime thunks.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable exits produce OS-neutral trap packets for a ring-3 Poly monitor.
The kernel still owns scheduling, privilege transitions, signals, and hard
faults.
