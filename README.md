# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code from an x64 Linux process.

## Run

```bash
make image
make boot-poly-full-arch-traps
```

Requires Docker with `linux/arm64` support. Rebuild the image after changing `bochs-prepoly-src/`.

Useful shorter targets: `make boot`, `make boot-poly-arch-traps`, `make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, and `make boot-poly-bench-arch-traps`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Differences From x64

The base machine is still x64: boot, rings, paging, interrupts, exceptions, virtual memory, syscalls, and TSO memory ordering remain x64.

The extension adds:

- `PENTER.A64` / `PENTER.RV64`: switch to raw AArch64 or RISC-V fetch/decode at guest `RIP`.
- `PCALL.*.SYSV`: call precompiled AAPCS64 or RISC-V psABI functions from x64 SysV code; ordinary foreign returns cross back through architectural cookies.
- Shared x64 virtual address space, permissions, page faults, and TSO ordering for foreign code.
- Architectural trap/syscall/fault delivery to software; no Linux or libcall emulation in the CPU.
- Explicit xstate-like storage for non-aliased foreign registers.

Prototype Bochs opcode encoding: `0f 24 <op> 50 4f 4c 59 21`.

Full ISA details are in `docs/poly-isa.md`.
