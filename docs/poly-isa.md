# Poly ISA Quick Reference

Poly is an x86_64 CPU extension prototype that adds AArch64 and RISC-V64
user-mode decode frontends. The compatibility target is existing native ABI
code and shared libraries in one x86_64 virtual address space.

## Run The Gate

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Focused targets: `boot-poly-apps-arch-traps`,
`boot-poly-call-real-xsave-arch-traps`, `boot-poly-binfmt-arch-traps`.

## ISA Contract

- x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts,
  syscalls, atomics, and global TSO memory ordering.
- AArch64 and RISC-V64 are user frontends fetched from the same `RIP` address
  space. AArch64 is fixed 32-bit fetch; RISC-V64 supports 16/32-bit RVC fetch.
- Frontend changes are decoded Poly control instructions, not fast-path `#UD`
  envelopes.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AAPCS64, and RISC-V64
  psABI. This is compatibility glue, not a new compiler-only ABI.
- Foreign state is explicit per-thread XSAVE-style state.
- CPU work: frontend switching, register rename signatures, transition return
  tracking, and trap packets.
- Runtime/loader work: stack arguments, aggregates, variadics, syscalls,
  libcalls, lazy binding, linker policy, and other memory-shaped ABI cases.

## Control Encodings

These are Bochs prototype encodings. A real CPU would allocate official opcode
space with the same fixed-latency semantics.

| ISA | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`, signature-slot calls,
`PTRAPRET`, and setup/query controls.

## Trap Scope

- Unsupported foreign instructions produce architectural trap records.
- Foreign `svc`/`ecall`, breakpoints, illegal instructions, and import exits
  are OS-neutral trap packets for a runtime or OS handler.
- Bochs is a functional ISA prototype, not a cycle-accurate performance model.

Deeper hardware/ABI notes: `docs/poly-isa-design-directions.md`.
