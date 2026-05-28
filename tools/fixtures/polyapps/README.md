# Polyapps

This directory contains hand-authored raw AArch64 and RISC-V payload
descriptions. `tools/build/mkpolyelf.c` converts them into guest ELFs during
`make image`.

Each `.poly` file is source, not a generated binary. The generated `.elf`
outputs belong under `out/` and inside the boot initramfs.

These payloads are useful for focused frontend, instruction-subset, syscall,
trap, and memory tests. Compatibility with precompiled shared libraries is
covered mainly by `tools/fixtures/polycall/`.
