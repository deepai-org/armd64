# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an ordinary x86_64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Other useful targets: `make boot`, `make boot-poly`,
`make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`.

Logs: `out/serial.log` for guest output, `out/bochs.log` for Bochs output.

## ISA Delta

The guest remains x86_64 Linux: boot, paging, privilege levels, virtual memory,
ordinary x86_64 code, and the Linux syscall ABI stay x86_64. Added pieces:

- `CPUID 0x40000000+`: polyglot feature discovery.
- `0f 24 <op> "POLY!"`: prototype x86 opcode slot.
- `PENTER.A64` / `PENTER.RV64`: enter raw AArch64/RISC-V fetch.
- AArch64 `brk #0x7fff` / RISC-V custom-0 `0x0000000b`: exit raw mode.
- Direct AArch64<->RISC-V switch/call escapes.
- `PCALL.*.SYSV`: calls into precompiled AAPCS64/RISC-V psABI code.
- Shared x86_64 virtual memory, page faults, permissions, and x86-TSO behavior.
- OS-neutral trap packets for syscalls, breakpoints, illegal instructions,
  imports, and unsupported operations.
- XSAVE component 20 for foreign state. Stock guest Linux enumerates it but
  does not enable `XCR0[20]`.

Full ISA details live in `docs/poly-isa.md`.
