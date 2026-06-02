# Poly ISA

Poly adds raw AArch64 and RISC-V64 user frontends to an x86_64 CPU. The Bochs
prototype targets existing precompiled foreign code in one x86_64 address
space.

## Quick Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused targets: `boot-poly-apps-arch-traps`,
`boot-poly-call-real-xsave-arch-traps`, `boot-poly-binfmt-arch-traps`.

## x86_64 Differences

- x86_64 owns boot, privilege, paging, interrupts, syscalls, atomics, and TSO.
- AArch64 and RISC-V64 are user-mode instruction frontends. They fetch native
  32-bit instructions from the current `RIP`.
- Mode changes use decoded Poly control instructions, not `#UD` envelopes.
- Cross-ISA calls target real ABIs: SysV x86_64, AAPCS64, and RISC-V64 psABI.
- Foreign state is explicit per-thread XSAVE-style state.
- CPU work: switch frontends, rename registers, deliver trap packets.
- Runtime/loader work: syscalls, libcalls, lazy binding, ABI reshaping.
- Not CPU work: Linux, libc, linker semantics, user-memory call descriptors, or
  stack repacking.

## Temporary Controls

These Bochs encodings are placeholders for real allocated opcode space.

| ISA | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`, signature-slot calls,
`PTRAPRET`, and setup/query controls.

Deeper design notes: `docs/poly-isa-design-directions.md`.
