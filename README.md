# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside an x86_64 Linux process.

## Run

```bash
make image
make boot-poly-full-arch-traps
```

Requires Docker with `linux/arm64` support. Re-run `make image` after changing `bochs-prepoly-src/`.

Focused targets: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, `make boot-poly-bench-arch-traps`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Differences From x86_64

The CPU is still architecturally x86_64 for boot, paging, privilege, interrupts, exceptions, syscalls, virtual memory, and TSO memory ordering. The polyglot extension adds user-mode foreign execution:

- `PENTER.A64` and `PENTER.RV64` switch fetch/decode to raw 32-bit AArch64 or RISC-V instructions at the current guest `RIP`.
- `PCALL.*.SYSV` bridges x86_64 SysV calls to precompiled AAPCS64 or RISC-V psABI functions.
- Foreign code shares the caller's x86_64 virtual address space, page faults, permissions, and TSO ordering.
- Foreign syscalls and traps become architectural events for software; the CPU does not emulate Linux policy or libcalls.
- Non-aliased foreign registers are explicit xstate-like architectural state, not hidden emulator state.

Foreign `ret` instructions cross-return through CPU-created cookies. The goal is compatibility with normal precompiled foreign functions, not a new compiler-only ABI.

Bochs currently encodes polyglot operations as `0f 24 <op> 50 4f 4c 59 21`. This is a prototype opcode slot, not the final silicon encoding.

Full ISA details: `docs/poly-isa.md`.
