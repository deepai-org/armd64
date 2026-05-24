# Bochs Boot Harness

This repository is being bootstrapped toward a Bochs-based x86_64 Linux bring-up.

Current state:
- A Docker image installs Bochs, GRUB tooling, Xvfb, and BusyBox.
- `scripts/boot.sh` downloads a small x86_64 Linux kernel from Alpine Linux.
- The script builds a minimal initramfs that prints boot markers to serial output and powers off.
- Bochs is launched under Xvfb and the serial log is checked for `BOOT_OK`.

Run:

```bash
make image
make boot
```
