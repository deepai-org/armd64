# Tools Layout

`tools/` contains the guest/runtime programs and the source fixtures used by
`scripts/boot.sh`.

- `polyexec.c`: standalone foreign ELF loader and process-mode runner.
- `polycall.c`: foreign shared-object loader, ABI bridge, dynamic linker, and
  descriptor-backed import runtime.
- `polyapp.c`, `polyprobe.c`, `polybench.c`, `polythread.c`, `polysignal.c`,
  `nativecheck.c`: boot-test programs for raw execution, traps, calls,
  threading, signals, benchmarks, and x86 host sanity checks.
- `polycpuid.h`: shared userspace definition of the experimental CPUID and
  poly XSAVE ABI structures.
- `mkpolyelf.c`: helper for building synthetic raw `.poly` payloads.
- `polybinfmt.sh`: guest binfmt-style wrapper used by boot tests.
- `polyapps/`: hand-authored raw AArch64/RISC-V payload descriptions.
- `fixtures/polycall/`: compiler-produced AArch64/RISC-V shared-object and
  ABI-call fixtures consumed by `polycall`.
- `fixtures/polyexec/`: compiler-produced process-mode ELF fixtures consumed
  by `polyexec`.
- `contracts/`: older regex-based consistency checks. These are not the main
  validation path; prefer real boot tests such as
  `make boot-poly-binfmt-arch-traps` and `make boot-poly-call-arch-traps`.

Keep new real-code test fixtures under `fixtures/` instead of adding more files
to the `tools/` root.
