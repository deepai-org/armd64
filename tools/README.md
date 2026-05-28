# Tools

`tools/` is source-only. Generated ELFs, initramfs contents, disk images, logs,
and temporary build products belong under `out/`.

## Layout

| Path | Contents |
| --- | --- |
| `include/` | Shared userspace constants for CPUID leaves, traps, imports, and poly XSAVE state. |
| `runtime/` | Guest loaders and wrappers: `polycall`, `polyexec`, and binfmt glue. |
| `programs/` | x86_64 guest programs for probes, raw payloads, threads, signals, benchmarks, and native checks. |
| `build/` | Host-side helper sources, currently `mkpolyelf`. |
| `fixtures/polyapps/` | Hand-authored raw AArch64/RISC-V `.poly` payload descriptions. |
| `fixtures/polycall/` | Native-ABI shared-object fixtures for `polycall` compatibility tests. |
| `fixtures/polyexec/` | Process-mode foreign ELF fixtures for `polyexec` tests. |

## Runtime Pieces

- `runtime/polycall.c`: shared-object loader, dynamic-linker subset, ABI bridge,
  generated thunks, descriptor imports, and runtime compatibility helpers.
- `runtime/polyexec.c`: process-style runner for foreign ELF tests.
- `runtime/polybinfmt.sh`: guest-side wrapper used by binfmt boot tests.
- `build/mkpolyelf.c`: converts `fixtures/polyapps/*.poly` descriptions into
  guest ELFs during image construction.

## Fixture Naming

- `*_main_real.*`: primary shared object for a dependency-chain test.
- `*_dep_real.*`, `*_leaf_real.*`, `*_mid_real.*`: dependency objects.
- `*_import_real.*`: fixture that expects a runtime or x86 helper import.
- `*_cross_*`: cross-ISA dependency or callback fixture.
- `*_x86_*_import_real.*`: foreign fixture calling back into an x86 helper.
- `*_fp*`, `*_hfa*`, `*_hetero*`, `*_vec128*`, `*_sret*`: ABI shape fixtures.
- `*_pthread_*`, `*_alloc_*`, `*_env_*`, `*_process_*`, `*_tls_*`: runtime and
  libc-compatibility fixtures.

`fixtures/polyapps/*.poly` files are focused raw frontend, instruction-subset,
syscall, trap, and memory tests. Compatibility with precompiled shared
libraries is covered mainly by `fixtures/polycall/`.

## Validation

Prefer real boot tests over contract-script expansion:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

Use `make check-poly-import-ids` and related checks only as quick consistency
smoke tests. Those scripts live under `scripts/checks/`, not `tools/`.
