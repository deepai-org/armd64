# Polycall Fixtures

These files are source fixtures for real cross-ISA shared-object tests. They
are compiled into guest `.so` files by `scripts/boot.sh`; generated outputs
belong under `out/` or the initramfs, not in this directory.

## Groups

- `aarch64_*`, `riscv64_*`: basic native ABI entry fixtures.
- `*_import_real.*`, `*_x86_*_import_real.*`: foreign code calling runtime or
  x86 helper imports.
- `*_needed_*`, `*_cross_*`, `*_preload_*`, `*_rpath_*`, `*_soname_*`:
  dynamic-linker dependency and symbol-resolution fixtures.
- `*_fp*`, `*_hfa*`, `*_hetero*`, `*_vec128*`, `*_sret*`: ABI shape fixtures.
- `*_pthread_*`, `*_alloc_*`, `*_env_*`, `*_process_*`, `*_tls_*`: runtime and
  libc-compatibility fixtures.

Prefer adding focused fixtures here over expanding regex-only contract checks.
