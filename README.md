# armd64

Bochs-based prototype for running precompiled AArch64 and RISC-V code inside an
x86_64 Linux process. The guest OS is still ordinary x86_64 Linux; userspace
enters foreign code through explicit polyglot ISA instructions.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Other useful targets:

- `make boot`: baseline x86_64 guest.
- `make boot-poly`: quick polyglot smoke test.
- `make boot-poly-call-arch-traps`: cross-ISA calls/imports/signals.
- `make boot-poly-binfmt-arch-traps`: foreign executable path.

Logs are written to `out/serial.log` and `out/bochs.log`.

## ISA Differences

Normal x86_64 execution, Linux boot, paging, privilege levels, and virtual
memory stay unchanged. The prototype adds:

- Discovery through private `CPUID` leaves at `0x40000000+`.
- Dedicated prototype x86 opcodes: `0f 24 <op> "POLY!"`, not `UD2` envelopes.
- Raw AArch64/RISC-V entry with `PENTER.A64` and `PENTER.RV64`.
- Raw exit with AArch64 `brk #0x7fff` or RISC-V custom-0 `0x0000000b`.
- ABI-aware `PCALL.*.SYSV` calls into precompiled AAPCS64/RISC-V psABI code.
- Shared x86_64 virtual memory, faults, permissions, and prototype TSO ordering.
- Architectural traps for foreign syscalls, breakpoints, illegal instructions,
  and unsupported operations.
- Prototype foreign state component 20 via `CPUID`/`XCR0`/`XSAVE`; stock guest
  Linux enumerates it but does not enable `XCR0[20]`.

Detailed design notes live in `docs/poly-isa.md`.
