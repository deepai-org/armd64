# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an x86_64 Linux process.

## Run

Requirements: Docker with `linux/arm64` support and a writable checkout.

```bash
make image
make boot-poly-full-arch-traps
```

Useful targets:

- `make boot`: plain x86_64 guest sanity boot.
- `make boot-poly-arch-traps`: CPUID, raw frontend, and trap checks.
- `make boot-poly-call-arch-traps`: cross-ISA call, loader, thread, and signal checks.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt checks.
- `make boot-poly-bench-arch-traps`: benchmark checks.
- `make clean`: remove generated state.

Logs are written to `out/serial.log` and `out/bochs.log`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

After changing `bochs-prepoly-src/`, rebuild the image before boot tests:

```bash
make image
```

## ISA Delta vs x86_64

The base machine remains x86_64. Boot, privileged execution, paging,
interrupts, exceptions, virtual memory, syscalls, and TSO memory ordering still
belong to the x86_64 guest OS.

The extension adds user-mode foreign execution:

- `PENTER.A64` / `PENTER.RV64` switch the frontend from x86 decode to raw
  32-bit AArch64 or RISC-V fetch at the current guest `RIP`.
- `PCALL.*.SYSV` calls precompiled AAPCS64 or RISC-V psABI functions from
  x86_64 SysV code through CPU-defined register, stack, and return bridges.
- Native foreign returns can cross-return through CPU-created return cookies.
- Foreign traps and syscalls are exposed as architectural events for software;
  the CPU does not emulate libcalls or Linux policy.
- Foreign registers are explicit xstate-like CPU state, not hidden CR3-scoped
  emulator state.
- Foreign memory accesses use the same x86_64 virtual memory, page faults,
  permissions, and ordering model as normal user code.

The Bochs prototype currently encodes polyglot operations as
`0f 24 <op> 50 4f 4c 59 21`. That is a temporary stand-in for future dedicated
x86 polyglot opcodes. Full ISA details live in `docs/poly-isa.md`.
