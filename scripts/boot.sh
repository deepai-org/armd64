#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CACHE_DIR="${CACHE_DIR:-$ROOT_DIR/cache}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/out}"
TMP_DIR="${TMP_DIR:-$ROOT_DIR/tmp}"
ALPINE_BASE_URL="${ALPINE_BASE_URL:-https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/netboot}"
ALPINE_X86_64_MAIN_URL="${ALPINE_X86_64_MAIN_URL:-https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64}"
KERNEL_URL="${KERNEL_URL:-$ALPINE_BASE_URL/vmlinuz-virt}"
MODLOOP_URL="${MODLOOP_URL:-$ALPINE_BASE_URL/modloop-virt}"

mkdir -p "$CACHE_DIR" "$OUT_DIR" "$TMP_DIR"

APKINDEX_ARCHIVE="$CACHE_DIR/APKINDEX-x86_64.tar.gz"
KERNEL_IMAGE="$CACHE_DIR/vmlinuz-virt"
MODLOOP_IMAGE="$CACHE_DIR/modloop-virt"
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
POLY_BINFMT_SRC="$ROOT_DIR/tools/polybinfmt.sh"
NATIVE_CHECK_SRC="$ROOT_DIR/tools/nativecheck.c"
NATIVE_CHECK_BIN="$OUT_DIR/nativecheck"
POLY_APP_PAYLOAD_DIR="$ROOT_DIR/tools/polyapps"
POLY_ELF_GEN_SRC="$ROOT_DIR/tools/mkpolyelf.c"
POLY_ELF_GEN_BIN="$OUT_DIR/mkpolyelf"
POLY_ENABLED="${POLY_ENABLED:-0}"
RUN_POLY_PROBE="${RUN_POLY_PROBE:-0}"
RUN_POLY_APPS="${RUN_POLY_APPS:-0}"
RUN_POLY_BINFMT="${RUN_POLY_BINFMT:-0}"
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

prepare_alpine_index() {
  download "$ALPINE_X86_64_MAIN_URL/APKINDEX.tar.gz" "$APKINDEX_ARCHIVE"
}

apk_package_version() {
  local package="$1"
  tar -xzOf "$APKINDEX_ARCHIVE" APKINDEX | awk -v package="$package" '
    $0 == "P:" package {found=1; next}
    found && /^V:/ {sub(/^V:/, "", $0); print; exit}
    /^$/ {found=0}
  '
}

download_kernel() {
  download "$KERNEL_URL" "$KERNEL_IMAGE"
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

build_native_check() {
  compile_poly_tool "$NATIVE_CHECK_SRC" "$NATIVE_CHECK_BIN" "${NATIVE_CHECK_CC:-}"
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-add.elf" 0xd2800f60 0x91002400
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-regadd.elf" 0xd2800c80 0xd28002e1 0x8b010000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mul.elf" 0xd28000e0 0xd28000c1 0x9b017c00
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-branch.elf" 0xd2800520 0x14000002 0xd2800020 0x91000400
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-condbranch.elf" 0xd2800000 0xb5000040 0x91000400 0xd2800000 0xb4000040 0xd2800c60 0x91000c00 0xb4000040 0x91001400 0xb5000040 0xd2800c60 0x91014c00
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-loop.elf" 0xd2800060 0xd1000400 0xb5ffffe0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ret.elf" 0xd28006e0 0xd65f03c0 0xd2800020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mem.elf" 0xd28009a0 0xf9000040 0xd2800000 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-strlen.elf" 0xd4200020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memfill.elf" 0xd2800080 0xd2800821 0xd4200040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-write.elf" 0xd2800020 0x91000041 0xd28000a2 0xd2800808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-exit.elf" 0xd28000e0 0xd2800ba8 0xd4000001 0xd2800c60
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-svc.elf" 0xd40000e1
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-brk.elf" 0xd42000a0
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-add.elf" 0x01f00513 0xffc50513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-regadd.elf" 0x06400513 0x01700593 0x00b50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mul.elf" 0x00700513 0x00600593 0x02b50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-branch.elf" 0x02900513 0x00000463 0x00100513 0x00150513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-condbranch.elf" 0x00000513 0x00051463 0x00150513 0x00000513 0x00050463 0x06300513 0x00350513 0x00050463 0x00550513 0x00051463 0x06300513 0x05350513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-loop.elf" 0x00300513 0xfff50513 0xfe051ee3
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ret.elf" 0x03700513 0x00008067 0x00100513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mem.elf" 0x04d00513 0x00a63023 0x00000513 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-strlen.elf" 0x00100893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memfill.elf" 0x00400513 0x05200593 0x00200893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-write.elf" 0x00100513 0x00060593 0x00500613 0x04000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-exit.elf" 0x00700513 0x05d00893 0x00000073 0x06300513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ecall.elf" 0x00700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ebreak.elf" 0x00500893 0x00100073
  chmod +x "$TMP_DIR/initramfs-root/usr/lib/polyapps"/*.elf
}

build_binfmt_module() {
  if [[ "$RUN_POLY_BINFMT" != "1" ]]; then
    return
  fi

  if ! command -v unsquashfs >/dev/null 2>&1; then
    echo "unsquashfs is required to extract binfmt_misc from Alpine modloop." >&2
    exit 1
  fi

  local modloop_extract="$TMP_DIR/modloop-extract"
  local module_path
  download "$MODLOOP_URL" "$MODLOOP_IMAGE"
  rm -rf "$modloop_extract"
  mkdir -p "$modloop_extract"
  unsquashfs -q -d "$modloop_extract" "$MODLOOP_IMAGE" 'modules/*/kernel/fs/binfmt_misc.ko*'
  module_path="$(find "$modloop_extract" -path '*/binfmt_misc.ko*' | head -n 1)"
  if [[ -z "$module_path" ]]; then
    echo "Unable to extract binfmt_misc module from $MODLOOP_IMAGE." >&2
    exit 1
  fi

  mkdir -p "$TMP_DIR/initramfs-root/lib/modules/poly"
  case "$module_path" in
    *.gz) gzip -dc "$module_path" > "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko" ;;
    *.xz) xz -dc "$module_path" > "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko" ;;
    *.ko) cp "$module_path" "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko" ;;
    *)
      echo "Unsupported binfmt_misc module compression: $module_path" >&2
      exit 1
      ;;
  esac
}

build_initramfs() {
  rm -rf "$TMP_DIR/initramfs-root"
  mkdir -p "$TMP_DIR/initramfs-root"/{bin,sbin,etc,proc,sys,dev,usr/bin,usr/sbin,usr/lib/polyapps}
  build_poly_probe
  build_poly_app
  build_poly_exec
  build_native_check
  local busybox_version
  local busybox_apk
  local busybox_extract
  prepare_alpine_index
  busybox_version="$({
    apk_package_version busybox-static
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
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/mkdir"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/mknod"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/sbin/insmod"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/sleep"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/echo"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/cat"
  ln -sf /bin/busybox "$TMP_DIR/initramfs-root/bin/ls"
  cp "$POLY_PROBE_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyprobe"
  cp "$POLY_APP_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyapp"
  cp "$POLY_EXEC_BIN" "$TMP_DIR/initramfs-root/usr/bin/polyexec"
  cp "$POLY_BINFMT_SRC" "$TMP_DIR/initramfs-root/usr/bin/polybinfmt"
  cp "$NATIVE_CHECK_BIN" "$TMP_DIR/initramfs-root/usr/bin/nativecheck.elf"
  chmod +x "$TMP_DIR/initramfs-root/usr/bin/polybinfmt"
  cp "$POLY_APP_PAYLOAD_DIR"/*.poly "$TMP_DIR/initramfs-root/usr/lib/polyapps/"
  build_poly_elf_payloads
  build_binfmt_module

  cat > "$TMP_DIR/initramfs-root/init" <<EOF
#!/bin/busybox sh
set -eu
RUN_POLY_PROBE="$RUN_POLY_PROBE"
RUN_POLY_APPS="$RUN_POLY_APPS"
RUN_POLY_BINFMT="$RUN_POLY_BINFMT"

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
    /usr/bin/polyexec \
    /usr/lib/polyapps/aarch64-add.elf=132 \
    /usr/lib/polyapps/aarch64-regadd.elf=123 \
    /usr/lib/polyapps/aarch64-mul.elf=42 \
    /usr/lib/polyapps/aarch64-branch.elf=42 \
    /usr/lib/polyapps/aarch64-condbranch.elf=91 \
    /usr/lib/polyapps/aarch64-loop.elf=0 \
    /usr/lib/polyapps/aarch64-ret.elf=55 \
    /usr/lib/polyapps/aarch64-mem.elf=77 \
    /usr/lib/polyapps/aarch64-strlen.elf=5 \
    /usr/lib/polyapps/aarch64-memfill.elf=4 \
    /usr/lib/polyapps/aarch64-write.elf=5 \
    /usr/lib/polyapps/aarch64-exit.elf=7 \
    /usr/lib/polyapps/aarch64-brk.elf=0x4c000105 \
    /usr/lib/polyapps/aarch64-svc.elf=0x53000701 \
    /usr/lib/polyapps/riscv-add.elf=27 \
    /usr/lib/polyapps/riscv-regadd.elf=123 \
    /usr/lib/polyapps/riscv-mul.elf=42 \
    /usr/lib/polyapps/riscv-branch.elf=42 \
    /usr/lib/polyapps/riscv-condbranch.elf=91 \
    /usr/lib/polyapps/riscv-loop.elf=0 \
    /usr/lib/polyapps/riscv-ret.elf=55 \
    /usr/lib/polyapps/riscv-mem.elf=77 \
    /usr/lib/polyapps/riscv-strlen.elf=5 \
    /usr/lib/polyapps/riscv-memfill.elf=4 \
    /usr/lib/polyapps/riscv-write.elf=5 \
    /usr/lib/polyapps/riscv-exit.elf=7 \
    /usr/lib/polyapps/riscv-ebreak.elf=0x4c000205 \
    /usr/lib/polyapps/riscv-ecall.elf=0x53000702 >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_BINFMT" = "1" ]; then
  echo "POLYBINFMT_START" >/dev/ttyS0
  if [ -f /lib/modules/poly/binfmt_misc.ko ]; then
    insmod /lib/modules/poly/binfmt_misc.ko >/dev/ttyS0 2>&1 || true
  fi
  mkdir -p /proc/sys/fs/binfmt_misc || {
    echo "POLYBINFMT_FAIL: mkdir binfmt_misc" >/dev/ttyS0
    exit 1
  }
  mount -t binfmt_misc binfmt_misc /proc/sys/fs/binfmt_misc 2>/dev/null || true
  if [ ! -w /proc/sys/fs/binfmt_misc/register ]; then
    echo "POLYBINFMT_FAIL: binfmt_misc unavailable" >/dev/ttyS0
    exit 1
  fi
  echo ':poly-aarch64:M:18:\xb7::/usr/bin/polybinfmt:' 2>/dev/ttyS0 > /proc/sys/fs/binfmt_misc/register || {
    echo "POLYBINFMT_FAIL: register aarch64" >/dev/ttyS0
    exit 1
  }
  echo ':poly-riscv:M:18:\xf3::/usr/bin/polybinfmt:' 2>/dev/ttyS0 > /proc/sys/fs/binfmt_misc/register || {
    echo "POLYBINFMT_FAIL: register riscv" >/dev/ttyS0
    exit 1
  }
  echo "POLYBINFMT_REGISTERED" >/dev/ttyS0
  /usr/bin/nativecheck.elf >/dev/ttyS0 2>&1 || {
    echo "POLYBINFMT_FAIL: native x86 elf" >/dev/ttyS0
    exit 1
  }
  for foreign in \
    /usr/lib/polyapps/aarch64-add.elf \
    /usr/lib/polyapps/aarch64-regadd.elf \
    /usr/lib/polyapps/aarch64-mul.elf \
    /usr/lib/polyapps/aarch64-branch.elf \
    /usr/lib/polyapps/aarch64-condbranch.elf \
    /usr/lib/polyapps/aarch64-loop.elf \
    /usr/lib/polyapps/aarch64-ret.elf \
    /usr/lib/polyapps/aarch64-mem.elf \
    /usr/lib/polyapps/aarch64-strlen.elf \
    /usr/lib/polyapps/aarch64-memfill.elf \
    /usr/lib/polyapps/aarch64-write.elf \
    /usr/lib/polyapps/aarch64-exit.elf \
    /usr/lib/polyapps/aarch64-brk.elf \
    /usr/lib/polyapps/aarch64-svc.elf \
    /usr/lib/polyapps/riscv-add.elf \
    /usr/lib/polyapps/riscv-regadd.elf \
    /usr/lib/polyapps/riscv-mul.elf \
    /usr/lib/polyapps/riscv-branch.elf \
    /usr/lib/polyapps/riscv-condbranch.elf \
    /usr/lib/polyapps/riscv-loop.elf \
    /usr/lib/polyapps/riscv-ret.elf \
    /usr/lib/polyapps/riscv-mem.elf \
    /usr/lib/polyapps/riscv-strlen.elf \
    /usr/lib/polyapps/riscv-memfill.elf \
    /usr/lib/polyapps/riscv-write.elf \
    /usr/lib/polyapps/riscv-exit.elf \
    /usr/lib/polyapps/riscv-ebreak.elf \
    /usr/lib/polyapps/riscv-ecall.elf
  do
    echo "POLYBINFMT_STEP: \$foreign" >/dev/ttyS0
    "\$foreign" >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: exec \$foreign" >/dev/ttyS0
      exit 1
    }
  done
  echo "POLYBINFMT_OK" >/dev/ttyS0
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
  download_kernel
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
      if [[ "$RUN_POLY_BINFMT" == "1" ]]; then
        if ! grep -q "POLYBINFMT_OK" "$SERIAL_LOG"; then
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
