# Tools

`tools/` is source-only. Build outputs, initramfs contents, disk images, logs,
and scratch files belong under `out/`.

## Layout

| Path | Purpose |
| --- | --- |
| `include/` | Shared CPUID, mode, trap, import, and XSAVE-style state constants. |
| `runtime/` | Guest loaders and wrappers: `polycall`, `polyexec`, and binfmt glue. |
| `programs/` | x86_64 guest probes, smoke tests, thread/signal tests, and benchmarks. |
| `build/` | Host-side build helpers. |
| `fixtures/polyapps/` | Hand-authored raw frontend/syscall/trap `.poly` payloads. |
| `fixtures/polycall/` | Native-ABI shared-object fixtures for linked-library compatibility. |
| `fixtures/polyexec/` | Foreign process-mode ELF fixtures. |

The large fixture directories are intentional test coverage. Do not reorganize
fixture paths without updating `scripts/boot.sh`.

## Main Files

- `runtime/polycall.c`: shared-object loader, ABI bridge, thunks, relocations,
  TLS, IFUNC, import traps, trampolines, and helper imports.
- `runtime/polyexec.c`: process-style foreign ELF runner.
- `runtime/polybinfmt.sh`: guest binfmt wrapper.
- `build/mkpolyelf.c`: converts `fixtures/polyapps/*.poly` into guest ELFs.

## Fixture Taxonomy

- `fixtures/polyapps/*.poly`: frontend, instruction, memory, syscall, and trap
  tests.
- `fixtures/polycall/*_real.*`: precompiled shared-object compatibility tests.
- `*_main_real.*`, `*_dep_real.*`, `*_leaf_real.*`, `*_mid_real.*`: dependency tests.
- `*_import_real.*`, `*_x86_*_import_real.*`: helper and x86 callback imports.
- `*_fp*`, `*_hfa*`, `*_hetero*`, `*_vec128*`, `*_sret*`, `*_longdouble*`, `*_int128*`: ABI shape coverage.
- `*_cross_*`: AArch64/RISC-V mixed dependency and callback coverage.
- `*_pthread_*`, `*_alloc_*`, `*_env_*`, `*_process_*`, `*_tls_*`: libc/runtime
  compatibility coverage.

## Validation

Prefer real boot tests:

- `make boot-poly-apps-arch-traps`: raw frontend apps, syscalls, traps, and
  generated ELF payloads.
- `make boot-poly-focused-validation`: nativecheck architecture controls,
  arch-trap exec, probe/control-plane, raw app payloads, neutral switching,
  process cross-ISA, syscall/import monitor, real-XSAVE call/thread/signal,
  binfmt dispatch, and benchmark trap-delta coverage.
- `make boot-poly-binfmt-arch-traps`: native x86 checks plus binfmt dispatch.
- `make boot-poly-full-real-xsave-arch-traps`: broad gate with the guest XCR0
  module loaded and real XSAVE/XRSTOR state required.

Use `scripts/checks/` only for quick consistency checks such as CPUID/import ID
drift. They are not substitutes for booting the guest and running real payloads.
