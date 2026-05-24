#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CACHE_DIR="${CACHE_DIR:-$ROOT_DIR/cache}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/out}"
TMP_DIR="${TMP_DIR:-$ROOT_DIR/tmp}"
ALPINE_BASE_URL="${ALPINE_BASE_URL:-https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/netboot}"
ALPINE_X86_64_MAIN_URL="${ALPINE_X86_64_MAIN_URL:-https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64}"
KERNEL_URL="${KERNEL_URL:-$ALPINE_BASE_URL/vmlinuz-virt}"

mkdir -p "$CACHE_DIR" "$OUT_DIR" "$TMP_DIR"

KERNEL_IMAGE="$CACHE_DIR/vmlinuz-virt"
INITRAMFS_IMAGE="$OUT_DIR/initramfs.cpio.gz"
ISO_ROOT="$TMP_DIR/iso-root"
ISO_IMAGE="$OUT_DIR/bochs-boot.iso"
SERIAL_LOG="$OUT_DIR/serial.log"
BOCHS_LOG="$OUT_DIR/bochs.log"
BOCHSRC="$TMP_DIR/bochsrc.txt"
BOCHS_RC="$TMP_DIR/bochs.rc"
CONSOLE_LOG="$OUT_DIR/bochs-console.log"

download() {
  local url="$1"
  local dest="$2"
  if [[ ! -s "$dest" ]]; then
    curl -fsSL "$url" -o "$dest"
  fi
}

build_initramfs() {
  rm -rf "$TMP_DIR/initramfs-root"
  mkdir -p "$TMP_DIR/initramfs-root"/{bin,sbin,etc,proc,sys,dev,usr/bin,usr/sbin}
  local busybox_version
  local busybox_apk
  local busybox_extract
  local apkindex_archive="$CACHE_DIR/APKINDEX-x86_64.tar.gz"
  download "$ALPINE_X86_64_MAIN_URL/APKINDEX.tar.gz" "$apkindex_archive"
  busybox_version="$({
    tar -xzOf "$apkindex_archive" APKINDEX | awk '
      /^P:busybox-static$/ {found=1; next}
      found && /^V:/ {sub(/^V:/, "", $0); print; exit}
    '
  } || true)"
  if [[ -z "$busybox_version" ]]; then
    echo "Unable to determine Alpine busybox-static version." >&2
    exit 1
  fi
  busybox_apk="$CACHE_DIR/busybox-static-$busybox_version.apk"
  download "$ALPINE_X86_64_MAIN_URL/busybox-static-$busybox_version.apk" "$busybox_apk"
  busybox_extract="$TMP_DIR/busybox-extract"
  rm -rf "$busybox_extract"
  mkdir -p "$busybox_extract"
  tar -xzf "$busybox_apk" -C "$busybox_extract" bin/busybox.static
  cp "$busybox_extract/bin/busybox.static" "$TMP_DIR/initramfs-root/bin/busybox"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/sh"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/sbin/poweroff"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/sbin/halt"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/mount"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/mknod"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/sleep"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/echo"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/cat"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/ls"

  cat > "$TMP_DIR/initramfs-root/init" <<'EOF'
#!/bin/busybox sh
set -eu

mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true

if [ ! -c /dev/console ]; then
  mknod -m 600 /dev/console c 5 1
fi

if [ ! -c /dev/ttyS0 ]; then
  mknod -m 620 /dev/ttyS0 c 4 64 || true
fi

echo "BOOT_OK: initramfs reached userspace" >/dev/console
echo "BOOT_OK: initramfs reached userspace" >/dev/ttyS0 2>/dev/null || true

sleep 1
poweroff -f || halt -f
EOF
  chmod +x "$TMP_DIR/initramfs-root/init"

  (cd "$TMP_DIR/initramfs-root" && find . -print0 | cpio --null -ov --format=newc) | gzip -9 > "$INITRAMFS_IMAGE"
}

build_iso() {
  rm -rf "$ISO_ROOT"
  mkdir -p "$ISO_ROOT/boot" "$ISO_ROOT/isolinux"
  cp "$KERNEL_IMAGE" "$ISO_ROOT/boot/vmlinuz-virt"
  cp "$INITRAMFS_IMAGE" "$ISO_ROOT/boot/initramfs.cpio.gz"
  local isolinux_bin
  local ldlinux_c32
  isolinux_bin="$(find /usr/lib -name isolinux.bin | head -n 1)"
  ldlinux_c32="$(find /usr/lib -name ldlinux.c32 | head -n 1)"
  if [[ -z "$isolinux_bin" || -z "$ldlinux_c32" ]]; then
    echo "Unable to locate isolinux boot files." >&2
    exit 1
  fi
  cp "$isolinux_bin" "$ISO_ROOT/isolinux/isolinux.bin"
  cp "$ldlinux_c32" "$ISO_ROOT/isolinux/ldlinux.c32"
  cat > "$ISO_ROOT/isolinux/isolinux.cfg" <<'EOF'
DEFAULT linux
TIMEOUT 0
PROMPT 0

LABEL linux
  KERNEL /boot/vmlinuz-virt
  APPEND initrd=/boot/initramfs.cpio.gz rdinit=/init init=/init console=ttyS0 console=tty0 loglevel=7 panic=1 noapic nolapic acpi=off
EOF
  xorriso -as mkisofs \
    -o "$ISO_IMAGE" \
    -b isolinux/isolinux.bin \
    -c isolinux/boot.cat \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -J -R -V BOCHS_LINUX \
    "$ISO_ROOT" >/dev/null
}

build_bochsrc() {
  cat > "$BOCHSRC" <<EOF
megs: 128
display_library: x
romimage: file=/usr/share/bochs/BIOS-bochs-latest
vgaromimage: file=/usr/share/bochs/VGABIOS-lgpl-latest
boot: cdrom
ata0-master: type=cdrom, path="$ISO_IMAGE", status=inserted
com1: enabled=1, mode=file, dev="$SERIAL_LOG"
log: "$BOCHS_LOG"
panic: action=report
error: action=report
info: action=report
debug: action=ignore
clock: sync=realtime
EOF
}

main() {
  download "$KERNEL_URL" "$KERNEL_IMAGE"
  build_initramfs
  build_iso
  : > "$SERIAL_LOG"
  : > "$BOCHS_LOG"
  : > "$CONSOLE_LOG"
  cat > "$BOCHS_RC" <<'EOF'
c
EOF
  build_bochsrc

  timeout 120s xvfb-run -a bochs -q -f "$BOCHSRC" -rc "$BOCHS_RC" >"$CONSOLE_LOG" 2>&1 || true

  if grep -q "BOOT_OK: initramfs reached userspace" "$SERIAL_LOG"; then
    echo "Boot smoke test passed."
    exit 0
  fi

  echo "Boot smoke test failed."
  echo "Serial log:"
  cat "$SERIAL_LOG" || true
  echo "Console log:"
  cat "$CONSOLE_LOG" || true
  echo "Bochs log:"
  cat "$BOCHS_LOG" || true
  exit 1
}

main "$@"
