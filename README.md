# Bochs Boot Harness

This repository is being bootstrapped toward a Bochs-based x86_64 Linux bring-up.

Current state:
- A Docker image installs Bochs, ISOLINUX, Xvfb, and BusyBox.
- `scripts/boot.sh` downloads a small x86_64 Linux kernel from Alpine Linux.
- The script builds a minimal initramfs from Alpine's x86_64 BusyBox, prints boot markers to serial output, optionally runs synthetic poly opcode probes and architecture-labelled payload files inside userspace, and halts cleanly.
- Bochs runs with the `nogui` display backend and the serial log is checked for `BOOT_OK`, `POLY_PROBE_OK`, and/or `POLYAPP_OK` depending on the requested gates.
- `tools/polyapps/*.poly` are payload manifests containing expected AArch64 or RISC-V results plus synthetic syscall/libcall checks. `scripts/boot.sh` generates matching minimal ELF64 payloads with `tools/mkpolyelf.c`.
- `tools/polyapp.c` runs in the x86_64 guest, validates the foreign ELF64 machine type and executable entry segment, then executes the payload's 32-bit instruction words through the current poly UD-envelope scaffold.
- `tools/polyexec.c` is a direct x86_64 guest launcher for generated AArch64 and RISC-V ELF64 payload paths, without a `.poly` manifest.
- Current generated foreign ELF payloads cover tiny arithmetic programs, syscall-instruction programs using AArch64 `svc #0` and RISC-V `ecall`, and libcall-instruction programs using AArch64 `brk #0` and RISC-V `ebreak`.
- Cross-architecture syscall and library-call handling is currently a deterministic scaffold: syscall markers report the active poly mode, and libcall markers report `0x4c000000 | (mode << 8) | id`.

Run:

```bash
make image
make boot
```

Run the current gated poly scaffold and guest payload launcher:

```bash
make boot-poly
```
