# Tools

Keep `tools/` source-only. Generated binaries, guest images, logs, and
temporary build products belong under `out/`.

## Runtime Path

- `runtime/polycall.c`: foreign shared-object loader, dynamic-linker subset,
  native ABI bridge, generated thunks, and descriptor import runtime.
- `runtime/polyexec.c`: foreign ELF/process runner for raw process-mode tests.
- `runtime/polybinfmt.sh`: guest-side binfmt wrapper used by boot tests.

## Test Programs

- `programs/polyprobe.c`: CPUID, trap, state, and frontend probes.
- `programs/polyapp.c`: raw `.poly` payload runner.
- `programs/polybench.c`: transition/execution microbenchmarks.
- `programs/polythread.c`: thread and foreign-state isolation tests.
- `programs/polysignal.c`: signal and interrupted-foreign-mode tests.
- `programs/nativecheck.c`: x86_64 guest sanity checks.

## Inputs

- `include/polycpuid.h`: userspace CPUID, trap, import, and poly XSAVE
  constants shared with the Bochs implementation.
- `polyapps/`: small hand-authored raw AArch64/RISC-V payload descriptions.
- `fixtures/polycall/`: native ABI shared-object fixtures for `polycall`.
- `fixtures/polyexec/`: process-mode ELF fixtures for `polyexec`.
- `build/mkpolyelf.c`: host helper that converts `.poly` descriptions into
  guest ELF payloads.

## Checks

- `contracts/`: coarse consistency checks. Keep them runnable, but prefer real
  boot tests for validation: `make boot-poly-binfmt-arch-traps`,
  `make boot-poly-call-arch-traps`, and `make boot-poly-full-arch-traps`.
