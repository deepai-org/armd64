# Tools Layout

`tools/` is split by role. Keep the root directory limited to this README and
subdirectories.

## Runtime

- `runtime/polyexec.c`: standalone foreign ELF/process runner.
- `runtime/polycall.c`: shared-object loader, dynamic linker, ABI bridge, and
  descriptor-backed import runtime.
- `runtime/polybinfmt.sh`: guest wrapper used by binfmt-style boot tests.

## Programs

- `programs/polyprobe.c`: CPUID, traps, state, and frontend probes.
- `programs/polyapp.c`: raw `.poly` payload runner.
- `programs/polybench.c`: transition and execution microbenchmarks.
- `programs/polythread.c`: thread/state isolation tests.
- `programs/polysignal.c`: signal and interrupted-foreign-mode tests.
- `programs/nativecheck.c`: x86 host sanity checks.

## Inputs

- `include/polycpuid.h`: shared userspace CPUID and poly XSAVE ABI constants.
- `polyapps/`: hand-authored raw AArch64/RISC-V payload descriptions.
- `fixtures/polycall/`: precompiled-code fixtures consumed by `polycall`.
- `fixtures/polyexec/`: process-mode ELF fixtures consumed by `polyexec`.
- `build/mkpolyelf.c`: helper for converting `.poly` descriptions into ELF
  payloads.

## Legacy Checks

- `contracts/`: older regex consistency checks. They are kept runnable, but
  real validation should use boot tests such as
  `make boot-poly-binfmt-arch-traps` and `make boot-poly-call-arch-traps`.
