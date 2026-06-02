# Poly ISA Quick Reference

Bochs prototype for running existing precompiled x86_64, AArch64, and RISC-V64
user code in one x86_64 virtual address space. Design rationale lives in
[`poly-isa-design-directions.md`](poly-isa-design-directions.md).

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log out/bochs.log
```

## Contract

- x86_64 is still the system ISA: boot, privilege, paging, interrupts, faults,
  atomics, syscalls, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode decode frontends in the same address space.
- ISA switches are decoded control instructions, not `#UD` traps.
- AArch64 uses 32-bit aligned fetch. RISC-V64 supports 16/32-bit fetch.
- `PCALL` switches ISA and can apply cached native-ABI register aliases.
- Foreign state is per-thread XSAVE-style architectural state.
- Recoverable foreign traps write OS-neutral trap records.
- Software handles stack arguments, aggregates, variadics, lazy binding,
  syscalls, libcalls, and debugger policy.

## Temporary Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls, setup/query,
`PLANDING`, and `PTRAPRET`.
