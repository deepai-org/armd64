# armd64

Bochs-based prototype for running precompiled AArch64 and RISC-V userspace code from an x86_64 Linux process.

## Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Rebuild the image after changing `bochs-prepoly-src/`.

Useful shorter targets: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, `make boot-poly-bench-arch-traps`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Differences From x86_64

The base machine remains x86_64: boot, rings, paging, interrupts, exceptions, virtual memory, syscalls, and TSO memory ordering are still x86_64.

The polyglot extension adds user-mode foreign execution:

- `PENTER.A64` and `PENTER.RV64` switch the frontend to raw 32-bit AArch64 or RISC-V fetch/decode at the current guest `RIP`.
- `PCALL.*.SYSV` calls precompiled AAPCS64 or RISC-V psABI functions from x86_64 SysV code; foreign `ret` can cross-return through architectural cookies.
- Foreign instructions use the same x86_64 virtual address space, permissions, page faults, and TSO ordering.
- Foreign syscalls, traps, and faults are architectural events routed to software; the CPU does not emulate Linux policy or libcalls.
- Non-aliased foreign registers are explicit xstate-like architectural state, not hidden emulator state.

Prototype Bochs opcode encoding: `0f 24 <op> 50 4f 4c 59 21`.

Full ISA details are in `docs/poly-isa.md`.
