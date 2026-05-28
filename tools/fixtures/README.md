# Fixtures

Fixtures are source inputs for real guest tests. They are intentionally kept
under `tools/fixtures/` rather than generated into `out/`, because
`scripts/boot.sh` compiles them into the initramfs for each boot run.

## Layout

- `polycall/`: shared-object fixtures loaded through `polycall`. These cover
  native ABI calls, dynamic relocations, dependency loading, TLS, IFUNC,
  symbol visibility, callbacks, FP/vector argument shapes, and x86 helper
  imports.
- `polyexec/`: process-mode foreign ELF fixtures loaded through `polyexec`.

## Naming

- `*_main_real.*`: primary shared object for a dependency-chain test.
- `*_dep_real.*`, `*_leaf_real.*`, `*_mid_real.*`: dependency objects.
- `*_import_real.*`: fixture that expects an x86 helper/runtime import.
- `*_cross_*`: cross-ISA dependency or callback fixture.
- `*_x86_*_import_real.*`: foreign fixture calling back into an x86 helper.

Generated `.so` files are not stored here; they are built into `out/`.
