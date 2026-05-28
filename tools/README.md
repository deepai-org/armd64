# Tools

`tools/` is source-only. Generated ELFs, initramfs contents, disk images, logs,
and temporary build products belong under `out/`.

## Directory Map

| Path | Purpose |
| --- | --- |
| `include/` | Shared userspace constants for CPUID, traps, imports, and poly XSAVE state. |
| `runtime/` | Guest runtime loaders and wrappers: `polycall`, `polyexec`, and binfmt glue. |
| `programs/` | Small x86_64 guest programs for probes, raw payloads, threads, signals, and benchmarks. |
| `polyapps/` | Hand-authored raw AArch64/RISC-V payload descriptions consumed by `mkpolyelf`. |
| `fixtures/polycall/` | Native-ABI shared-object fixtures for `polycall` compatibility tests. |
| `fixtures/polyexec/` | Process-mode foreign ELF fixtures for `polyexec` tests. |
| `build/` | Host-side source for build helpers. |
| `contracts/` | Coarse consistency checks. Keep runnable, but do not treat as primary validation. |

## Primary Runtime Pieces

- `runtime/polycall.c`: shared-object loader, dynamic-linker subset, ABI bridge,
  generated thunks, and descriptor import runtime.
- `runtime/polyexec.c`: process-style runner for foreign ELF tests.
- `runtime/polybinfmt.sh`: guest-side wrapper used by binfmt boot tests.
- `build/mkpolyelf.c`: converts `.poly` payload descriptions into guest ELFs.

## Validation

Prefer real boot tests over contract-script expansion:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

Use `make check-poly-import-ids` and related contract checks only as quick
consistency smoke tests.
