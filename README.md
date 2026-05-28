# armd64

Bochs prototype for running existing AArch64 and RISC-V userspace code in an
x86_64 Linux process. CPU changes live in `bochs-prepoly-src/`.

## Run It

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Re-run `make image` after changing `bochs-prepoly-src/`.

Other useful targets:

- `make boot`: baseline x86_64 Linux boot.
- `make boot-poly-call-arch-traps`: cross-ISA calls, threads, and signals.
- `make boot-poly-full-arch-traps`: full current suite.

Check the serial log:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Delta From x86_64

The machine still boots and runs as x86_64. Rings, paging, virtual memory,
interrupts, exceptions, syscall entry, and memory ordering remain x86_64.

The extension adds raw AArch64/RISC-V frontend switching at `RIP`, ABI bridge
instructions for x86_64 SysV, AArch64 AAPCS64, and RISC-V psABI calls, shared
virtual memory/page faults, and xstate-like storage for non-x86 registers.

Prototype Bochs encoding: `0f 24 <op> 50 4f 4c 59 21`.

Foreign syscalls, breakpoints, illegal instructions, and faults exit through
architectural trap records. The CPU does not emulate Linux, libc, or library
calls.

Full details: `docs/poly-isa.md`.
