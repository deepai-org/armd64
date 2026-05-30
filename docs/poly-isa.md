# Poly ISA Quick Reference

Poly is a Bochs prototype of one virtual address space with three user-code
frontends: x86_64, AArch64, and RISC-V64. x86_64 remains the system ISA.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused tests:

- `make boot-poly-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

## Architecture

| Area | Contract |
| --- | --- |
| System ISA | x86_64 owns boot, privilege, paging, faults, interrupts, VM control, atomics, and TSO ordering. |
| Foreign code | AArch64 and RISC-V64 fetch normal aligned 32-bit instructions from the same virtual address space. |
| Transitions | Decoded Poly control instructions switch/call frontends. The fast path is not a `#UD` trap envelope. |
| State | Foreign registers, trap packets, ABI signatures, transition frames, and landing policy are XSAVE-style architectural state. |
| ABI interop | Fast calls use register-only signature slots. Stack args, aggregates, variadics, lazy binding, libcalls, and syscall translation stay in software/runtime code. |

## Frontends

| ID | Frontend |
| --- | --- |
| `0` | x86_64 |
| `1` | AArch64 |
| `2` | RISC-V64 |

Core operations: `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.

Prototype constants and exact encodings live in
`tools/include/polycpuid.h`. Design rationale and hardware direction live in
`docs/poly-isa-design-directions.md`.
