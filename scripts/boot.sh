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
POLY_PROBE_SRC="$ROOT_DIR/tools/polyprobe.c"
POLY_PROBE_BIN="$OUT_DIR/polyprobe"
POLY_APP_SRC="$ROOT_DIR/tools/polyapp.c"
POLY_APP_BIN="$OUT_DIR/polyapp"
POLY_EXEC_SRC="$ROOT_DIR/tools/polyexec.c"
POLY_EXEC_BIN="$OUT_DIR/polyexec"
POLY_APP_PAYLOAD_DIR="$ROOT_DIR/tools/polyapps"
POLY_ELF_GEN_SRC="$ROOT_DIR/tools/mkpolyelf.c"
POLY_ELF_GEN_BIN="$OUT_DIR/mkpolyelf"
POLY_ENABLED="${POLY_ENABLED:-0}"
RUN_POLY_PROBE="${RUN_POLY_PROBE:-0}"
RUN_POLY_APPS="${RUN_POLY_APPS:-0}"
BOCHS_BIOS_DIR=""
if [[ -d "$ROOT_DIR/bochs-src/bochs/bios" ]]; then
  BOCHS_BIOS_DIR="$ROOT_DIR/bochs-src/bochs/bios"
elif [[ -d "$ROOT_DIR/bochs-prepoly-src/bochs/bios" ]]; then
  BOCHS_BIOS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs/bios"
fi

download() {
  local url="$1"
  local dest="$2"
  if [[ ! -s "$dest" ]]; then
    curl -fsSL "$url" -o "$dest"
  fi
}

compile_poly_tool() {
  local src="$1"
  local bin="$2"
  local requested_compiler="$3"

  if [[ -x "$bin" && "$bin" -nt "$src" ]]; then
    return
  fi

  local compiler=""
  for candidate in "$requested_compiler" x86_64-linux-gnu-gcc gcc-x86-64-linux-gnu cc gcc; do
    if [[ -n "$candidate" ]] && command -v "$candidate" >/dev/null 2>&1; then
      compiler="$candidate"
      break
    fi
  done

  if [[ -z "$compiler" ]]; then
    if [[ -x "$bin" ]]; then
      return
    fi
    echo "No compiler available for $src and no prebuilt $bin found." >&2
    exit 1
  fi

  local -a compiler_args=(-O2 -static -s)
  if [[ "$compiler" == x86_64-linux-gnu-gcc || "$compiler" == gcc-x86-64-linux-gnu ]]; then
    compiler_args+=(--sysroot=/usr/x86_64-linux-gnu)
  fi
  "$compiler" "${compiler_args[@]}" "$src" -o "$bin"
}

build_poly_probe() {
  compile_poly_tool "$POLY_PROBE_SRC" "$POLY_PROBE_BIN" "${POLY_PROBE_CC:-}"
}

build_poly_app() {
  compile_poly_tool "$POLY_APP_SRC" "$POLY_APP_BIN" "${POLY_APP_CC:-}"
}

build_poly_exec() {
  compile_poly_tool "$POLY_EXEC_SRC" "$POLY_EXEC_BIN" "${POLY_EXEC_CC:-}"
}

build_poly_elf_generator() {
  if [[ -x "$POLY_ELF_GEN_BIN" && "$POLY_ELF_GEN_BIN" -nt "$POLY_ELF_GEN_SRC" ]]; then
    return
  fi

  local compiler=""
  for candidate in "${POLY_ELF_GEN_CC:-}" cc gcc; do
    if [[ -n "$candidate" ]] && command -v "$candidate" >/dev/null 2>&1; then
      compiler="$candidate"
      break
    fi
  done

  if [[ -z "$compiler" ]]; then
    echo "No native compiler available for $POLY_ELF_GEN_SRC." >&2
    exit 1
  fi

  "$compiler" -O2 "$POLY_ELF_GEN_SRC" -o "$POLY_ELF_GEN_BIN"
}

build_poly_elf_payloads() {
  build_poly_elf_generator
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-add.elf" 0xd2800540 0x91000400
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-svc.elf" 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-brk.elf" 0xd4200000
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-add.elf" 0x01100513 0x00550513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ecall.elf" 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ebreak.elf" 0x00100073
}

build_initramfs() {
  rm -rf "$TMP_DIR/initramfs-root"
  mkdir -p "$TMP_DIR/initramfs-root"/{bin,sbin,etc,proc,sys,dev,usr/bin,usr/sbin,usr/lib/polyapps}
  build_poly_probe
  build_poly_app
  build_poly_exec
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
  cp "$POLY_PROBE_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyprobe"
  cp "$POLY_APP_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyapp"
  cp "$POLY_EXEC_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyexec"
  cp "$POLY_APP_PAYLOAD_DIR"/*.poly "$TMP_DIR/initramfs-root/usr/lib/polyapps/"
  build_poly_elf_payloads

  cat > "$TMP_DIR/initramfs-root/init" <<EOF
#!/bin/busybox sh
set -eu
RUN_POLY_PROBE="$RUN_POLY_PROBE"
RUN_POLY_APPS="$RUN_POLY_APPS"

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

if [ "$RUN_POLY_PROBE" = "1" ]; then
  /usr/bin/polyprobe >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_APPS" = "1" ]; then
  /usr/bin/polyapp /usr/lib/polyapps/*.poly >/dev/ttyS0 2>&1
  /usr/bin/polyexec /usr/lib/polyapps/*.elf >/dev/ttyS0 2>&1
fi

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
  local vga_romimage
  local bios_romimage=""
  if [[ -n "$BOCHS_BIOS_DIR" ]]; then
    bios_romimage="$BOCHS_BIOS_DIR/BIOS-bochs-latest"
  fi
  if [[ -z "$bios_romimage" || ! -s "$bios_romimage" ]]; then
    bios_romimage="$(find /usr/share /usr/local/share -path '*/BIOS-bochs-latest' -o -path '*/BIOS-bochs-latest.bin' | head -n 1)"
  fi
  if [[ -z "$bios_romimage" || ! -s "$bios_romimage" ]]; then
    echo "Unable to locate system BIOS image." >&2
    exit 1
  fi

  if [[ -n "$BOCHS_BIOS_DIR" ]]; then
    vga_romimage="$BOCHS_BIOS_DIR/VGABIOS-lgpl/VGABIOS-lgpl-latest.bin"
  fi
  if [[ -z "$vga_romimage" || ! -s "$vga_romimage" ]]; then
    vga_romimage="$(find /usr/share /usr/local/share -path '*/VGABIOS-lgpl-latest.bin' -o -path '*/vgabios-stdvga.bin' | head -n 1)"
  fi
  if [[ -z "$vga_romimage" ]]; then
    echo "Unable to locate VGA BIOS image." >&2
    exit 1
  fi

  cat > "$BOCHSRC" <<EOF
megs: 128
display_library: nogui
romimage: file=$bios_romimage
vgaromimage: file=$vga_romimage
boot: cdrom
ata0-master: type=cdrom, path="$ISO_IMAGE", status=inserted
com1: enabled=1, mode=file, dev="$SERIAL_LOG"
cpu: poly_enabled=$POLY_ENABLED
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

  local bochs_cmd
  if [[ -n "${BOCHS_BIN:-}" ]]; then
    bochs_cmd="$BOCHS_BIN"
  elif [[ -x /usr/local/bin/bochs-poly ]]; then
    bochs_cmd=/usr/local/bin/bochs-poly
  elif command -v bochs >/dev/null 2>&1; then
    bochs_cmd="$(command -v bochs)"
  else
    echo "No bochs binary found." >&2
    exit 1
  fi

  local -a bochs_args=(-q -f "$BOCHSRC")
  if "$bochs_cmd" --help 2>&1 | grep -q -- ' -rc '; then
    bochs_args+=(-rc "$BOCHS_RC")
  fi

  "$bochs_cmd" "${bochs_args[@]}" >"$CONSOLE_LOG" 2>&1 &
  local bochs_pid=$!
  local deadline=$((SECONDS + 120))
  local success=0
  while (( SECONDS < deadline )); do
    if grep -q "BOOT_OK: initramfs reached userspace" "$SERIAL_LOG"; then
      if [[ "$RUN_POLY_PROBE" == "1" ]]; then
        if ! grep -q "POLY_PROBE_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_APPS" == "1" ]]; then
        if ! grep -q "POLYAPP_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYEXEC_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      success=1
      break
    fi

    if ! kill -0 "$bochs_pid" 2>/dev/null; then
      break
    fi
    sleep 1
  done

  if kill -0 "$bochs_pid" 2>/dev/null; then
    kill "$bochs_pid" 2>/dev/null || true
  fi
  wait "$bochs_pid" 2>/dev/null || true

  if (( success )); then
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
