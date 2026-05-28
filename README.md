# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code in an
x86_64 Linux process.

## Run

Requirements: Docker with `linux/arm64` support and a writable checkout.

```bash
make image
```

```bash
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: plain x86_64 guest sanity boot.
- `make boot-poly-arch-traps`: CPUID, raw frontend, and trap checks.
- `make boot-poly-call-arch-traps`: cross-ISA call, loader, thread, and signal checks.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt checks.
- `make boot-poly-bench-arch-traps`: benchmark checks.
- `make clean`: remove generated state.

Logs are written to `out/serial.log` and `out/bochs.log`. A quick result check:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

After changing `bochs-prepoly-src/`, run `make image` again before boot tests.

## What Changes vs x86_64

The machine is still x86_64 for privileged execution: paging, rings,
interrupts, exceptions, virtual memory, TSO ordering, and the guest OS are
x86_64.

The ISA extension adds user-mode foreign execution:

- `PENTER.A64` / `PENTER.RV64`: switch from x86 decode to raw 32-bit AArch64 or
  RISC-V fetch at the current guest `RIP`.
- `PCALL.*.SYSV`: call precompiled AAPCS64 or RISC-V psABI code from x86_64
  SysV code through CPU-managed argument/result bridges.
- Native AArch64/RISC-V return instructions can cross-return through
  CPU-managed call cookies.
- Foreign syscalls, breakpoints, and illegal instructions create neutral trap
  records; the CPU does not emulate Linux libcalls.
- Foreign register state is exposed as explicit prototype xstate, not hidden
  emulator-only process state.

The current Bochs encoding uses `0f 24 <op> 50 4f 4c 59 21` as a temporary
stand-in for future dedicated x86 polyglot opcodes. Full ISA details are in
`docs/poly-isa.md`.
