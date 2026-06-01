# Poly ISA Quick Reference

Poly extends x86_64 so existing AArch64 and RISC-V64 user-mode code can run in
the same virtual address space as x86 code.

## Run It

```sh
make image
make boot-poly-focused-validation
make BOOT_TIMEOUT_SECONDS=900 boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Architectural Contract

- x86_64 is still the system ISA: boot, paging, privilege, interrupts,
  syscalls, atomics, and TSO ordering stay x86-defined.
- AArch64 and RISC-V64 are ring-3 decode frontends over the same x86 virtual
  address space.
- ISA changes use decoded control instructions, not per-instruction `#UD`
  envelopes.
- Foreign state is per-thread XSAVE-style architectural state, not hidden
  process-global emulator state.
- Fast cross-ISA calls use ABI signature slots for register-only remapping.
- Stack arguments, aggregates, variadics, lazy binding, libc policy, and syscall
  policy are software/runtime responsibilities.
- Recoverable foreign exits produce OS-neutral trap packets.

## Encodings

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86 controls: `0f 3a fc xx` for `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`,
  `PCALL_IMM`, and `PTRAPRET`.
- AArch64 controls: reserved `HINT` encodings.
- RISC-V controls: `custom-0` encodings.

Long-form design rationale: `docs/poly-isa-design-directions.md`.
