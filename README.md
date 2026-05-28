# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an ordinary x86_64 Linux guest.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: baseline x86_64 Linux boot.
- `make boot-poly`: quick polyglot smoke test.
- `make boot-poly-call-arch-traps`: cross-ISA calls, imports, signals, traps.
- `make boot-poly-binfmt-arch-traps`: foreign executable loading path.

Logs:

- `out/serial.log`: guest-visible test output.
- `out/bochs.log`: emulator/debug log.

## ISA Delta From x86_64

Normal x86_64 boot, paging, privilege levels, virtual memory, and Linux syscall
ABI remain x86_64. The added polyglot ISA pieces are:

- `CPUID 0x40000000+`: polyglot feature discovery.
- `0f 24 <op> "POLY!"`: prototype dedicated x86 opcode space.
- `PENTER.A64` / `PENTER.RV64`: enter raw 32-bit AArch64/RISC-V fetch.
- AArch64 `brk #0x7fff` / RISC-V custom-0 `0x0000000b`: exit raw mode.
- Direct AArch64<->RISC-V switch/call operations.
- ABI-aware `PCALL.*.SYSV` into precompiled AAPCS64/RISC-V psABI functions.
- Shared x86_64 virtual memory, page faults, permissions, and prototype TSO.
- Architectural trap packets for foreign syscalls, breakpoints, illegal
  instructions, imports, and unsupported operations.
- Prototype foreign state as XSAVE component 20. Current stock guest Linux
  enumerates it but does not enable `XCR0[20]`.

Full ISA details are in `docs/poly-isa.md`.
