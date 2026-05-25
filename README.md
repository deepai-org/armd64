# Bochs Boot Harness

This repository is being bootstrapped toward a Bochs-based x86_64 Linux bring-up.

Current state:
- A Docker image installs Bochs, ISOLINUX, Xvfb, and BusyBox.
- `scripts/boot.sh` downloads a small x86_64 Linux kernel from Alpine Linux.
- The script builds a minimal initramfs from Alpine's x86_64 BusyBox, prints boot markers to serial output, optionally runs synthetic poly opcode probes and architecture-labelled payload files inside userspace, and halts cleanly.
- Bochs runs with the `nogui` display backend and the serial log is checked for `BOOT_OK`, `POLY_PROBE_OK`, and/or `POLYAPP_OK` depending on the requested gates.
- `tools/polyapps/*.poly` are payload artifacts containing AArch64 or RISC-V instruction words. `tools/polyapp.c` loads them in the x86_64 guest and executes them through the current poly UD-envelope scaffold.

Run:

```bash
make image
make boot
```

Run the current gated poly scaffold and guest payload launcher:

```bash
make boot-poly
```
