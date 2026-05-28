# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an x86_64 Linux process.

## Run

```bash
make image
make boot-poly-full-arch-traps
```

Requires Docker with `linux/arm64` support. Re-run `make image` after changing
`bochs-prepoly-src/`.

Focused targets: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-binfmt-arch-traps`, and
`make boot-poly-bench-arch-traps`.

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Delta From x86_64

The machine is still x86_64 for boot, paging, privilege, interrupts,
exceptions, syscalls, virtual memory, and TSO memory ordering. The extension
adds user-mode foreign execution:

- `PENTER.A64` and `PENTER.RV64` switch instruction fetch from x86_64 to raw
  AArch64 or RISC-V at the current guest `RIP`.
- `PCALL.*.SYSV` bridges ordinary x86_64 SysV calls to precompiled AAPCS64 or
  RISC-V psABI functions by mapping native registers, stack arguments, return
  registers, and return cookies.
- Foreign code uses the same x86_64 virtual address space, page faults,
  permissions, and TSO ordering as the process that called it.
- Foreign traps and syscalls are surfaced as architectural events for software.
  The CPU does not emulate Linux policy, libcalls, or named library functions.
- Non-aliased foreign registers are explicit xstate-like architectural state,
  not hidden CR3-scoped emulator state.

Foreign `ret` instructions can cross-return through CPU-created cookies, so the
goal is compatibility with normal precompiled foreign functions, not a new
compiler-only ABI.

Bochs currently encodes polyglot operations as `0f 24 <op> 50 4f 4c 59 21`.
That is a prototype opcode slot, not the intended final silicon encoding. Full
ISA details are in `docs/poly-isa.md`.
