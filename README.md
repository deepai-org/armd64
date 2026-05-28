# armd64

Bochs prototype for running precompiled AArch64 and RISC-V userspace code inside
an x86_64 Linux process.

The goal is compatibility with real foreign objects and shared libraries, not a
new compiler-only ISA or ABI.

## Run

Requirements:

- Docker with `linux/arm64` support.
- The repo mounted writable so the boot scripts can create `cache/`, `out/`,
  and `tmp/`.

Build the Bochs image:

```bash
make image
```

Run the broad test suite:

```bash
make boot-poly-full-arch-traps
```

Common targets:

- `make boot`: plain x86_64 guest sanity boot.
- `make boot-poly-arch-traps`: CPUID, raw frontend, and trap checks.
- `make boot-poly-call-arch-traps`: cross-ISA call, loader, thread, and signal checks.
- `make boot-poly-binfmt-arch-traps`: foreign executable/binfmt checks.
- `make boot-poly-bench-arch-traps`: benchmark checks.
- `make clean`: remove generated state.

Logs:

- `out/serial.log`: guest console and pass/fail markers.
- `out/bochs.log`: Bochs trace/debug output.

Useful marker check:

```bash
grep -E 'BOOT_OK|NATIVE_CHECK_OK|POLYCALL_OK|POLYTHREAD_OK|POLYSIGNAL_OK|FAIL|Kernel panic' out/serial.log
```

Re-run `make image` after changing `bochs-prepoly-src/`; the boot targets use
the Docker image, not a host Bochs binary.

## What Changes vs x86_64

The guest machine remains x86_64 for privileged architecture:

- x86_64 paging, privilege rings, interrupts, exceptions, and virtual memory.
- x86_64 TSO memory ordering.
- x86_64 Linux as the guest OS.
- CPU-visible foreign state exposed through CPUID/XSAVE-style contracts.

The prototype adds user-mode polyglot execution:

- A temporary Bochs opcode family, `0f 24 <op> 50 4f 4c 59 21`, stands in for
  future dedicated x86 polyglot opcodes.
- `PENTER.A64` and `PENTER.RV64` switch the frontend from x86 variable-length
  decode to raw foreign fetch.
- AArch64 and RISC-V instructions are fetched directly from the same guest
  address space as x86 code.
- Native foreign return instructions return through CPU-managed cross-ISA call
  cookies.
- `PCALL.*.SYSV` bridges x86_64 SysV callers to precompiled AAPCS64 or RISC-V
  psABI callees.
- Foreign syscall, breakpoint, and illegal-instruction events produce neutral
  trap records. The CPU does not emulate Linux libcalls.
- Foreign architectural state is explicit prototype xstate, not hidden
  per-process emulator state.

Full ISA details are in `docs/poly-isa.md`.
