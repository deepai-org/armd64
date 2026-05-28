# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside an x64 Linux process.

## Run

Requires Docker with `linux/arm64` support. Re-run `make image` after changing `bochs-prepoly-src/`.

```bash
make image
make boot-poly-full-arch-traps
```

Focused targets: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, `make boot-poly-bench-arch-traps`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Delta From x64

The base architecture remains x64: boot, rings, paging, virtual memory, interrupts, exceptions, syscall entry, and TSO memory ordering. The extension adds:

- `PENTER.A64` / `PENTER.RV64`: switch from x64 decode to raw 32-bit AArch64 or RISC-V decode at guest `RIP`.
- `PCALL.*.SYSV`: call precompiled AAPCS64 or RISC-V psABI code from x64 SysV code.
- Foreign code shares x64 virtual memory, page permissions, faults, and TSO ordering.
- Foreign traps, syscalls, and faults are delivered to software; no Linux/libc emulation in the CPU.
- Non-aliased foreign registers live in xstate-like architectural state.

Prototype Bochs opcode encoding: `0f 24 <op> 50 4f 4c 59 21`.

Full ISA details: `docs/poly-isa.md`.
