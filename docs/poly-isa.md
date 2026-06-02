# Poly ISA Quick Reference

Poly is a Bochs prototype for running existing x86_64, AArch64, and RISC-V64
user-mode code in one x86_64 process.

For hardware/ABI design rationale, see `docs/poly-isa-design-directions.md`.

## Run

```bash
make image
make BOOT_TIMEOUT_SECONDS=900 boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## How It Differs From x86_64

- x86_64 still owns boot, privilege, paging, interrupts, syscalls, atomics, and
  the effective TSO memory model.
- AArch64 and RISC-V64 are raw user frontends fetching native instructions from
  the same virtual address space.
- Mode changes are decoded Poly control instructions, not `#UD` traps or
  per-instruction envelopes.
- Cross-ISA calls target real native ABIs: SysV x86_64, AAPCS64, and RISC-V
  psABI.
- Foreign register state is explicit XSAVE-style per-thread state.
- Hardware may switch frontends and rename registers, but must not implement
  libc, translate syscalls, parse user descriptors, or repack stacks.
- Recoverable foreign events produce OS-neutral trap packets for the runtime.

## Prototype Controls

Current Bochs encodings are temporary; real hardware needs allocated opcode
space. Subops map to `PENTER`, `PSWITCH`, `PLANDING`, `PCALL`, `PCALL_SLOT`,
`PTRAPRET`, and setup/query controls.

| ISA | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |
