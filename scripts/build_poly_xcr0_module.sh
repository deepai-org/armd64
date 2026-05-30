#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ALPINE_X86_64_MAIN_URL="${ALPINE_X86_64_MAIN_URL:-https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64}"
HOST_UID="$(id -u)"
HOST_GID="$(id -g)"

docker run --rm -i \
  --platform=linux/amd64 \
  -v "$ROOT_DIR":/work \
  -w /work \
  -e ALPINE_X86_64_MAIN_URL="$ALPINE_X86_64_MAIN_URL" \
  -e HOST_UID="$HOST_UID" \
  -e HOST_GID="$HOST_GID" \
  alpine:latest \
  sh <<'EOF'
set -eu

apk add --no-cache \
  bash \
  bison \
  build-base \
  curl \
  elfutils-dev \
  flex \
  gmp-dev \
  make \
  mpc1-dev \
  mpfr-dev \
  openssl-dev \
  perl \
  tar \
  zstd >/dev/null

apk_index="cache/APKINDEX-x86_64.tar.gz"
mkdir -p cache out tmp
if [ ! -f "$apk_index" ]; then
  curl -fsSL "$ALPINE_X86_64_MAIN_URL/APKINDEX.tar.gz" -o "$apk_index"
fi

version="$(
  tar -xOzf "$apk_index" APKINDEX |
    awk 'BEGIN { RS = ""; FS = "\n" }
      {
        package = ""; version = "";
        for (i = 1; i <= NF; i++) {
          if ($i == "P:linux-virt-dev")
            package = "linux-virt-dev";
          else if ($i ~ /^V:/)
            version = substr($i, 3);
        }
        if (package == "linux-virt-dev") {
          print version;
          exit;
        }
      }'
)"
if [ -z "$version" ]; then
  echo "Unable to determine linux-virt-dev version from $apk_index" >&2
  exit 1
fi

dev_apk="cache/linux-virt-dev-$version.apk"
if [ ! -f "$dev_apk" ]; then
  curl -fsSL "$ALPINE_X86_64_MAIN_URL/linux-virt-dev-$version.apk" \
    -o "$dev_apk"
fi

kernel_apk="cache/linux-virt-$version.apk"
if [ ! -f "$kernel_apk" ]; then
  curl -fsSL "$ALPINE_X86_64_MAIN_URL/linux-virt-$version.apk" \
    -o "$kernel_apk"
fi

build_dir="tmp/poly-xcr0-module"
src_dir="$build_dir/module"
headers_dir="$build_dir/headers"
kernel_dir="$build_dir/kernel"
rm -rf "$build_dir"
mkdir -p "$src_dir" "$headers_dir" "$kernel_dir"
tar -xzf "$kernel_apk" -C "$kernel_dir" boot/vmlinuz-virt
cp "$kernel_dir/boot/vmlinuz-virt" cache/vmlinuz-virt
cp "$kernel_dir/boot/vmlinuz-virt" "cache/vmlinuz-virt-$version"
tar -xzf "$dev_apk" -C "$headers_dir"
headers="$(
  find "$headers_dir" -maxdepth 4 -type d -name 'linux-headers-*-virt' |
    head -n 1
)"
if [ -z "$headers" ]; then
  echo "Unable to find linux-virt headers in $dev_apk" >&2
  exit 1
fi

cp tools/kernel/poly_xcr0.c "$src_dir/poly_xcr0.c"
printf '%s\n' 'obj-m += poly_xcr0.o' > "$src_dir/Makefile"
make -C "$headers" M="/work/$src_dir" ARCH=x86_64 modules
cp "$src_dir/poly_xcr0.ko" out/poly_xcr0.ko
strip --strip-debug out/poly_xcr0.ko
chown "$HOST_UID:$HOST_GID" out/poly_xcr0.ko
chown "$HOST_UID:$HOST_GID" cache/vmlinuz-virt "cache/vmlinuz-virt-$version"
chown -R "$HOST_UID:$HOST_GID" "$build_dir"
EOF
