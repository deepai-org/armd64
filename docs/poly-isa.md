# Poly ISA

Poly is an x86_64 CPU-extension prototype that adds AArch64 and RISC-V64
user-mode frontends. The target is existing native ABI code and shared
libraries in one x86_64 virtual address space.

## Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful focused gates: `boot-poly-apps-arch-traps`,
`boot-poly-call-real-xsave-arch-traps`, `boot-poly-binfmt-arch-traps`.

## Contract

- x86_64 is the system ISA: boot, privilege, paging, interrupts, faults,
  syscalls, atomics, and global TSO ordering stay x86-owned.
- AArch64 and RISC-V64 are peer user frontends fetched from the same address
  space. AArch64 fetches fixed 32-bit instructions; RISC-V64 fetches 16/32-bit
  RVC instructions.
- Mode changes use decoded Poly control instructions, not fast-path `#UD`
  envelopes.
- Cross-ISA calls target native ABIs: x86_64 SysV, AAPCS64, and RISC-V psABI.
- Per-thread foreign state is architectural XSAVE-style state, not hidden
  emulator state.
- Hardware handles fixed-latency frontend switching, register signature
  aliasing, transition returns, and OS-neutral trap records.
- Runtime/loader code handles memory-shaped ABI work: stack arguments,
  aggregates, variadics, syscalls, libcalls, imports, and lazy binding.

## Prototype Encodings

These are Bochs encodings. Real silicon should allocate official opcode space
but keep the same fixed-latency semantics.

| Frontend | Control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Control subops include `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls,
`PLANDING`, `PTRAPRET`, and setup/query operations.

Detailed hardware and ABI direction lives in `docs/poly-isa-design-directions.md`.
