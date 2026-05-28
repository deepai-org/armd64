# armd64

Bochs prototype for running existing AArch64 and RISC-V userspace code inside
an x86_64 Linux process.

## How To Run

Requires Docker with `linux/arm64` support.

```bash
make image
make boot-poly-full-arch-traps
```

Rebuild the image after changing `bochs-prepoly-src/`.

Useful test targets:

- `make boot`: baseline x86_64 Linux boot without polyglot CPU support.
- `make boot-poly-arch-traps`: raw AArch64/RISC-V frontend and trap smoke tests.
- `make boot-poly-call-arch-traps`: cross-ISA call, thread, and signal tests.
- `make boot-poly-binfmt-arch-traps`: foreign ELF loader/binfmt tests.
- `make boot-poly-bench-arch-traps`: microbenchmarks.
- `make boot-poly-full-arch-traps`: full current test suite.

Check the serial log after a run:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

## ISA Delta From x86_64

The machine is still x86_64 for boot, privilege rings, paging, virtual memory,
interrupt delivery, exceptions, syscall entry, and TSO memory ordering. The
polyglot extension adds hardware-visible frontend switching and ABI bridging:

- `PENTER.A64` and `PENTER.RV64` switch from x86_64 decode to raw foreign
  instruction fetch at `RIP`.
- Foreign frontends execute precompiled AArch64 AAPCS64 and RISC-V psABI code;
  this is not a new compiler-only ABI.
- `PCALL.*.SYSV` bridges common x86_64 SysV calls to native foreign ABI calls,
  including tested integer, FP, aggregate, vector, stack-argument, and sret
  cases.
- Foreign code uses the same x86_64 virtual address space, page permissions,
  fault path, and TSO memory contract.
- Foreign syscalls, breakpoints, illegal instructions, and faults exit through
  architectural trap records for software handling. The CPU does not emulate
  Linux, libc, or specific library calls.
- Non-aliased foreign registers are modeled as xstate-like architectural state,
  not hidden CR3-only process state.

Prototype Bochs encoding: `0f 24 <op> 50 4f 4c 59 21`.

Full ISA details: `docs/poly-isa.md`.
