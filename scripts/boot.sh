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
POLY_CALL_SRC="$ROOT_DIR/tools/polycall.c"
POLY_CALL_BIN="$OUT_DIR/polycall"
POLY_THREAD_SRC="$ROOT_DIR/tools/polythread.c"
POLY_THREAD_BIN="$OUT_DIR/polythread"
POLY_SIGNAL_SRC="$ROOT_DIR/tools/polysignal.c"
POLY_SIGNAL_BIN="$OUT_DIR/polysignal"
POLY_BENCH_SRC="$ROOT_DIR/tools/polybench.c"
POLY_BENCH_BIN="$OUT_DIR/polybench"
POLY_BINFMT_SRC="$ROOT_DIR/tools/polybinfmt.sh"
NATIVE_CHECK_SRC="$ROOT_DIR/tools/nativecheck.c"
NATIVE_CHECK_BIN="$OUT_DIR/nativecheck"
AARCH64_POLYCALL_REAL_SRC="$ROOT_DIR/tools/aarch64_polycall_real.c"
RISCV64_POLYCALL_REAL_SRC="$ROOT_DIR/tools/riscv64_polycall_real.c"
POLYCALL_STATE_SRC="$ROOT_DIR/tools/polycall_state.c"
POLYCALL_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_import_real.c"
POLYCALL_LIBC_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_libc_import_real.c"
POLYCALL_IMPORT_VALUE_REAL_SRC="$ROOT_DIR/tools/polycall_import_value_real.c"
POLYCALL_WEAK_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_weak_import_real.c"
POLYCALL_FUNCPTR_REAL_SRC="$ROOT_DIR/tools/polycall_funcptr_real.c"
POLYCALL_PAIR_REAL_SRC="$ROOT_DIR/tools/polycall_pair_real.c"
POLYCALL_SRET_REAL_SRC="$ROOT_DIR/tools/polycall_sret_real.c"
POLYCALL_CTOR_REAL_SRC="$ROOT_DIR/tools/polycall_ctor_real.c"
POLYCALL_COND_REAL_SRC="$ROOT_DIR/tools/polycall_cond_real.c"
POLYCALL_SELECT_VARIANTS_REAL_SRC="$ROOT_DIR/tools/polycall_select_variants_real.c"
POLYCALL_CBZ_REAL_SRC="$ROOT_DIR/tools/polycall_cbz_real.c"
POLYCALL_BITBRANCH_REAL_SRC="$ROOT_DIR/tools/polycall_bitbranch_real.c"
POLYCALL_UBFM_REAL_SRC="$ROOT_DIR/tools/polycall_ubfm_real.c"
POLYCALL_SBFM_REAL_SRC="$ROOT_DIR/tools/polycall_sbfm_real.c"
POLYCALL_SIGNED_EXT_REAL_SRC="$ROOT_DIR/tools/polycall_signed_ext_real.c"
POLYCALL_SIGNED_LOAD_REAL_SRC="$ROOT_DIR/tools/polycall_signed_load_real.c"
POLYCALL_INT_DIV_REAL_SRC="$ROOT_DIR/tools/polycall_int_div_real.c"
POLYCALL_INT_MADD_REAL_SRC="$ROOT_DIR/tools/polycall_int_madd_real.c"
POLYCALL_INT_HIGHMUL_REAL_SRC="$ROOT_DIR/tools/polycall_int_highmul_real.c"
POLYCALL_INT_CARRY_REAL_SRC="$ROOT_DIR/tools/polycall_int_carry_real.c"
POLYCALL_INT_VARSHIFT_REAL_SRC="$ROOT_DIR/tools/polycall_int_varshift_real.c"
POLYCALL_INT_LOGIC_REAL_SRC="$ROOT_DIR/tools/polycall_int_logic_real.c"
POLYCALL_INT_BITOPS_REAL_SRC="$ROOT_DIR/tools/polycall_int_bitops_real.c"
POLYCALL_INT_ROTATE_REAL_SRC="$ROOT_DIR/tools/polycall_int_rotate_real.c"
POLYCALL_INT_CCMP_REAL_SRC="$ROOT_DIR/tools/polycall_int_ccmp_real.c"
POLYCALL_POSTINDEX_MEM_AARCH64_SRC="$ROOT_DIR/tools/polycall_postindex_mem_aarch64.c"
POLYCALL_ATOMIC_AARCH64_SRC="$ROOT_DIR/tools/polycall_atomic_aarch64.c"
POLYCALL_ATOMIC_RISCV_SRC="$ROOT_DIR/tools/polycall_atomic_riscv.c"
POLYCALL_UNSCALED_MEM_REAL_SRC="$ROOT_DIR/tools/polycall_unscaled_mem_real.c"
POLYCALL_INDEXED_MEM_REAL_SRC="$ROOT_DIR/tools/polycall_indexed_mem_real.c"
POLYCALL_CALLEE_REAL_SRC="$ROOT_DIR/tools/polycall_callee_real.c"
POLYCALL_FP64_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_real.c"
POLYCALL_FPAIR_REAL_SRC="$ROOT_DIR/tools/polycall_fpair_real.c"
POLYCALL_FPAIR_ARG_REAL_SRC="$ROOT_DIR/tools/polycall_fpair_arg_real.c"
POLYCALL_MIXED_ARGS_REAL_SRC="$ROOT_DIR/tools/polycall_mixed_args_real.c"
POLYCALL_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_import_real.c"
POLYCALL_FP64_CALLEE_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_callee_real.c"
POLYCALL_FP64_COND_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_cond_real.c"
POLYCALL_FP64_DIV_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_div_real.c"
POLYCALL_FP64_UNARY_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_unary_real.c"
POLYCALL_FP64_ABS_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_abs_real.c"
POLYCALL_FP64_SQRT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_sqrt_real.c"
POLYCALL_FP64_FMA_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_fma_real.c"
POLYCALL_FP64_FMA_VARIANTS_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_fma_variants_real.c"
POLYCALL_FP64_MINMAX_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_minmax_real.c"
POLYCALL_FP64_SELECT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_select_real.c"
POLYCALL_FP64_INDEXED_MEM_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_indexed_mem_real.c"
POLYCALL_FP64_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_convert_real.c"
POLYCALL_FP64_SIGNED_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_signed_convert_real.c"
POLYCALL_FP64_I32_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_i32_convert_real.c"
POLYCALL_FP64_U32_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_u32_convert_real.c"
POLYCALL_FP_MIXED_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_fp_mixed_convert_real.c"
POLYCALL_INT_FP_CONVERT_REAL_SRC="$ROOT_DIR/tools/polycall_int_fp_convert_real.c"
POLYCALL_FP32_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_real.c"
POLYCALL_FP32_ABS_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_abs_real.c"
POLYCALL_FP32_SQRT_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_sqrt_real.c"
POLYCALL_FP32_FMA_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_fma_real.c"
POLYCALL_FP32_FMA_VARIANTS_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_fma_variants_real.c"
POLYCALL_FP32_MINMAX_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_minmax_real.c"
POLYCALL_FP32_SELECT_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_select_real.c"
POLYCALL_FP32_MEM_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_mem_real.c"
POLY_APP_PAYLOAD_DIR="$ROOT_DIR/tools/polyapps"
POLY_ELF_GEN_SRC="$ROOT_DIR/tools/mkpolyelf.c"
POLY_ELF_GEN_BIN="$OUT_DIR/mkpolyelf"
POLY_ENABLED="${POLY_ENABLED:-0}"
RUN_POLY_PROBE="${RUN_POLY_PROBE:-0}"
RUN_POLY_APPS="${RUN_POLY_APPS:-0}"
RUN_POLY_EXEC="${RUN_POLY_EXEC:-$RUN_POLY_APPS}"
RUN_POLY_CALL="${RUN_POLY_CALL:-$RUN_POLY_APPS}"
RUN_POLY_THREAD="${RUN_POLY_THREAD:-$RUN_POLY_CALL}"
RUN_POLY_SIGNAL="${RUN_POLY_SIGNAL:-$RUN_POLY_THREAD}"
RUN_POLY_BENCH="${RUN_POLY_BENCH:-0}"
RUN_POLY_BINFMT="${RUN_POLY_BINFMT:-0}"
RUN_NATIVE_CHECK="${RUN_NATIVE_CHECK:-0}"
EXPECT_POLY_CPUID="${EXPECT_POLY_CPUID:-0}"
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

  local -a compiler_args=(-O2 -static -s -fno-stack-protector)
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

build_poly_call() {
  compile_poly_tool "$POLY_CALL_SRC" "$POLY_CALL_BIN" "${POLY_CALL_CC:-}"
}

build_poly_thread() {
  if [[ -x "$POLY_THREAD_BIN" && "$POLY_THREAD_BIN" -nt "$POLY_THREAD_SRC" ]]; then
    return
  fi

  local compiler=""
  for candidate in "${POLY_THREAD_CC:-}" x86_64-linux-gnu-gcc gcc-x86-64-linux-gnu; do
    if [[ -n "$candidate" ]] && command -v "$candidate" >/dev/null 2>&1; then
      compiler="$candidate"
      break
    fi
  done

  if [[ -z "$compiler" ]]; then
    echo "No x86_64 compiler available for $POLY_THREAD_SRC." >&2
    exit 1
  fi

  "$compiler" -O2 -static -s -fno-stack-protector -pthread \
    --sysroot=/usr/x86_64-linux-gnu "$POLY_THREAD_SRC" -o "$POLY_THREAD_BIN"
}

build_poly_signal() {
  compile_poly_tool "$POLY_SIGNAL_SRC" "$POLY_SIGNAL_BIN" "${POLY_SIGNAL_CC:-}"
}

build_poly_bench() {
  compile_poly_tool "$POLY_BENCH_SRC" "$POLY_BENCH_BIN" "${POLY_BENCH_CC:-}"
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
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$AARCH64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$AARCH64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-gnu-hash-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STATE_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-state.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LIBC_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-libc-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_VALUE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-value-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_WEAK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-weak-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FUNCPTR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-funcptr-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pair-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SRET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sret-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ctor-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cond-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SELECT_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-select-variants-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CBZ_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cbz-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BITBRANCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-bitbranch-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_UBFM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ubfm-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SBFM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sbfm-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SIGNED_EXT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-signed-ext-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SIGNED_LOAD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-signed-load-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_DIV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-div-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_MADD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-madd-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_HIGHMUL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-highmul-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_CARRY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-carry-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_VARSHIFT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-varshift-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_LOGIC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-logic-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_BITOPS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-bitops-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_ROTATE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-rotate-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_CCMP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-ccmp-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_POSTINDEX_MEM_AARCH64_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-postindex-mem.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -mno-outline-atomics \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOMIC_AARCH64_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-atomic.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOMIC_AARCH64_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-atomic-outline.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=armv8.1-a+lse \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOMIC_AARCH64_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-atomic-lse.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_UNSCALED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-unscaled-mem-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INDEXED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-indexed-mem-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-callee-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MIXED_ARGS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-mixed-args-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-callee-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-cond-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_DIV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-div-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_UNARY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-unary-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-abs-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-math-errno -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-sqrt-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_FMA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-fma-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_FMA_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-fma-variants-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_MINMAX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-minmax-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SELECT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-select-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_INDEXED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-indexed-mem-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SIGNED_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-signed-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_I32_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-i32-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_U32_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-u32-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP_MIXED_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp-mixed-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_FP_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int-fp-convert-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-abs-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-math-errno -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-sqrt-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_FMA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-fma-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_FMA_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-fma-variants-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_MINMAX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-minmax-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_SELECT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-select-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-mem-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$RISCV64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$RISCV64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-gnu-hash-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STATE_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-state.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LIBC_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-libc-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_VALUE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-value-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_WEAK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-weak-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FUNCPTR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-funcptr-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pair-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SRET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sret-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ctor-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cond-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SELECT_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-select-variants-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CBZ_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cbz-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BITBRANCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-bitbranch-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_UBFM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ubfm-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SBFM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sbfm-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SIGNED_EXT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-signed-ext-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SIGNED_LOAD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-signed-load-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_DIV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-div-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_MADD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-madd-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_HIGHMUL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-highmul-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_CARRY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-carry-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_VARSHIFT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-varshift-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_LOGIC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-logic-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_BITOPS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-bitops-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_ROTATE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-rotate-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_CCMP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-ccmp-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOMIC_RISCV_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-atomic.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_UNSCALED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-unscaled-mem-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INDEXED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-indexed-mem-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-callee-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair-arg-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MIXED_ARGS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-mixed-args-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-callee-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-cond-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_DIV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-div-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_UNARY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-unary-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-abs-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-math-errno -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-sqrt-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_FMA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-fma-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_FMA_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-fma-variants-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_MINMAX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-minmax-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SELECT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-select-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_INDEXED_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-indexed-mem-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_SIGNED_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-signed-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_I32_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-i32-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_U32_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-u32-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP_MIXED_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp-mixed-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT_FP_CONVERT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int-fp-convert-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-abs-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-math-errno -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-sqrt-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_FMA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-fma-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_FMA_VARIANTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-fma-variants-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_MINMAX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-minmax-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_SELECT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-select-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_MEM_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-mem-real.so"
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-add.elf" 0xd2800f60 0x91002400
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-regadd.elf" 0xd2800c80 0xd28002e1 0x8b010000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-movwide.elf" 0xd2a24680 0xf28acf00 0xf2d35780 0x929fffe1 0xca010000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mul.elf" 0xd28000e0 0xd28000c1 0x9b017c00
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-logical.elf" 0xd2801e00 0xd2800781 0xca010000 0x8a010000 0xaa010000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-shifted.elf" 0xd28000a0 0xd2800061 0x928000e2 0x8b010803 0xcb010464 0xaa0113e5 0xca4304a6 0x8a8204c7 0x8b060080 0x8b070000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-regmix.elf" 0xd2800140 0xd28000a1 0xd2800062 0x8b020020 0xca020000 0xaa010000 0x8a020000 0x9b017c00 0xcb020000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-branch.elf" 0xd2800520 0x14000002 0xd2800020 0x91000400
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-condbranch.elf" 0xd2800000 0xb5000040 0x91000400 0xd2800000 0xb4000040 0xd2800c60 0x91000c00 0xb4000040 0x91001400 0xb5000040 0xd2800c60 0x91014c00
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-loop.elf" 0xd2800060 0xd1000400 0xb5ffffe0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ret.elf" 0xd28006e0 0xd65f03c0 0xd2800020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sum.elf" 0x8b010000 0x8b020000 0x8b030000 0x8b040000 0x8b050000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sum8.elf" 0x8b010000 0x8b020000 0x8b030000 0x8b040000 0x8b050000 0x8b060000 0x8b070000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sum9.elf" 0x8b010000 0x8b020000 0x8b030000 0x8b040000 0x8b050000 0x8b060000 0x8b070000 0xf94003e8 0x8b080000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-frame.elf" 0xd10043ff 0xf9400be8 0x8b080000 0xf90007e0 0xf94007e8 0x910043ff 0x8b010108 0x8b020108 0x8b030108 0x8b040108 0x8b050108 0x8b060108 0x8b070100 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-native-frame.elf" 0xa9bf7bfd 0x910003fd 0x8b010000 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-bl.elf" 0xa9bf7bfd 0x910003fd 0x94000003 0xa8c17bfd 0xd65f03c0 0x8b010000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-adrp.elf" 0x90000000 0x91004000 0xf9400000 0xd65f03c0 0x0000002a 0x00000000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cond.elf" 0xf100041f 0x54000061 0xeb00003f 0x5400006c 0xd2800020 0xd65f03c0 0xd2800540 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-split-load.elf" --split-data64 0x7b 0xd0000000 0x91000000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dynrel.elf" --dyn-relative64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relr.elf" --dyn-relr64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf" --dyn-relr-bitmap64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-irelative.elf" --dyn-irelative64 0x7b --export-at poly_entry 12 0xd0000000 0x91002000 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dynsym.elf" --dyn-symbol64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dyntab.elf" --dyn-symbol64 0x7b 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dyntab-entry.elf" --dyn-symbol64 0x7b --export-dyntab-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-jumprel.elf" --dyn-jump-slot64 0x7b 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import.elf" --dyn-import64 poly_import_value 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-func.elf" --dyn-import-func64 poly_import_add 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-mul.elf" --dyn-import-func64 poly_import_mul 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-x86.elf" --dyn-import-func64 poly_import_x86_add 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mem.elf" 0xd28009a0 0xf9000020 0xd2800000 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memwidth.elf" 0x928001a5 0x39000025 0x39400026 0x92800025 0xb9000425 0xb9400427 0xd2824685 0x79001025 0x79401028 0x8b0700c0 0x8b080000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-strlen.elf" 0xd4200020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memfill.elf" 0xd2800080 0xd2800822 0xd4200040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcmp.elf" 0xd2800083 0x91000422 0xd4200060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcpy.elf" 0xd2800080 0x91000422 0xd4200080
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-read.elf" 0xd2800000 0x91000021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-write.elf" 0xd2800020 0x91000021 0xd28000a2 0xd2800808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-writev.elf" 0xaa0103e3 0xf9000023 0xd28000a4 0xf9000424 0xd2800020 0xd2800022 0xd2800848 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-lseek.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xd28000e1 0xd2800002 0xd28007c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0x91002021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read-close.elf" 0x91000026 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xf90000c0 0x910020c1 0xd2800082 0xd28007e8 0xd4000001 0xf94000c0 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-gettime.elf" 0xd2800000 0x91000021 0xd2800e28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpgid.elf" 0xd2800000 0xd2801368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsid.elf" 0xd2800000 0xd2801388 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrusage.elf" 0xd2800000 0x91000021 0xd28014a8 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getcpu.elf" 0x91000020 0x91002021 0xd2801508 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-gettimeofday.elf" 0x91000020 0xd2800001 0xd2801528 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sysinfo.elf" 0x91000020 0xd2801668 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap.elf" 0xd2800000 0xd2801bc8 0xd4000001 0xcb020000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap6.elf" 0xd2800000 0xd2800201 0xd2800062 0xd2800443 0xd28000a4 0xd28000e5 0xd2801bc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap-store.elf" 0xd2800000 0xd2801bc8 0xd4000001 0xd28009a1 0xf9000001 0xf9400000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpid.elf" 0xd2801588 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getppid.elf" 0xd28015a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getuid.elf" 0xd28015c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-geteuid.elf" 0xd28015e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getgid.elf" 0xd2801608 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getegid.elf" 0xd2801628 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-gettid.elf" 0xd2801648 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getcwd.elf" 0x91000020 0xd2800201 0xd2800228 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fp-int-move.elf" 0xd2a82800 0x1e270000 0x1e260000 0x9e670001 0x9e660020 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fp32-to-int.elf" 0xd2a82800 0x1e270000 0x1e380000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fp32-to-int64.elf" 0xd2a82800 0x1e270000 0x9e390000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-uname.elf" 0x91000020 0xd2801408 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-exit.elf" 0xd28000e0 0xd2800ba8 0xd4000001 0xd2800c60
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-svc.elf" 0xd40000e1
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-brk.elf" 0xd42000a0
  local -a aarch64_long=(0xd2800000)
  for _ in $(seq 1 80); do
    aarch64_long+=(0x91000400)
  done
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-long.elf" "${aarch64_long[@]}"
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-add.elf" 0x01f00513 0xffc50513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed.elf" 0x05194555 0x00018082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-word.elf" 0x0020717d 0xc008456d 0xc02a4008 0x61414502 0x00008082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-alu.elf" 0x25794531 0x05068105 0x858955c1 0x462989bd 0x8e15468d 0x8e558e35 0x9e298e75 0x85329e15 0x00008082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-fp.elf" 0x4541717d 0xe02a050a 0x00202502 0x200ca008 0x6502a02e 0x00016141 0x00008082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-int-move.elf" 0x80000537 0x02a50513 0xf0050553 0xe0050553 0xf20505d3 0xe2058553 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-class.elf" 0xf0000553 0xe0051553 0xf20005d3 0xe20595d3 0x00b50533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp32-to-int.elf" 0x41400537 0xf0050553 0xc0051553 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-upper.elf" 0x12345537 0x67850513 0x00000597 0x00000617 0x40b60633 0x00c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-immops.elf" 0x00500513 0x00351513 0x00356513 0x00154513 0x03f57513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-wordops.elf" 0xfff00293 0x0022831b 0x01f3139b 0x01e3d41b 0x41e3d49b 0x0094053b 0x00700593 0x02b5863b 0x00c5053b 0x40b5053b
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-shiftcmp.elf" 0xfff00293 0x0002a313 0x0012b393 0x00300413 0x008414b3 0x0084d533 0x4082d5b3 0x0082a633 0x0082b6b3 0x0084173b 0x008757bb 0x4082d83b 0x00650533 0x00c50533 0x00e50533 0x00f50533 0x01050533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-divrem.elf" 0x06400293 0x00700313 0xf9c00893 0x0262c3b3 0x0262e433 0x0262d4b3 0x0262f533 0x0268c5b3 0x0268e633 0x0268c6bb 0x0268e73b 0x0262d7bb 0x0262f83b 0x00750533 0x00850533 0x00950533 0x00b50533 0x00c50533 0x00d50533 0x00e50533 0x00f50533 0x01050533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-shifts.elf" 0x04000513 0x00255513 0x00151513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-srai.elf" 0xff800513 0x40155513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-regadd.elf" 0x06400513 0x01700593 0x00b50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mul.elf" 0x00700513 0x00600593 0x02b50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-logical.elf" 0x0f000513 0x03c00593 0x00b54533 0x00b57533 0x00b56533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-regmix.elf" 0x00a00513 0x00500593 0x00300613 0x00c58533 0x00c54533 0x00b56533 0x00c57533 0x02b50533 0x40c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-branch.elf" 0x02900513 0x00000463 0x00100513 0x00150513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-jal.elf" 0x00100513 0x008005ef 0x06450513 0x0080066f 0x06450513 0x40b60633 0x00c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-jalr.elf" 0x00100513 0x00000597 0x01058593 0x00058667 0x06450513 0x00000697 0x40c68633 0x00c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-branchcmp.elf" 0x00000513 0x00500593 0x00900613 0x00c5c463 0x06450513 0x00150513 0x00b65463 0x06450513 0x00250513 0xfff00593 0x00100613 0x00c5c463 0x06450513 0x00450513 0x00c5d463 0x00850513 0x00b65463 0x06450513 0x01050513 0x00b66463 0x06450513 0x02050513 0x00c5f463 0x06450513 0x04050513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-condbranch.elf" 0x00000513 0x00051463 0x00150513 0x00000513 0x00050463 0x06300513 0x00350513 0x00050463 0x00550513 0x00051463 0x06300513 0x05350513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-loop.elf" 0x00300513 0xfff50513 0xfe051ee3
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ret.elf" 0x03700513 0x00008067 0x00100513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sum.elf" 0x00b50533 0x00c50533 0x00d50533 0x00e50533 0x00f50533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sum8.elf" 0x00b50533 0x00c50533 0x00d50533 0x00e50533 0x00f50533 0x01050533 0x01150533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sum9.elf" 0x00b50533 0x00c50533 0x00d50533 0x00e50533 0x00f50533 0x01050533 0x01150533 0x00013283 0x00550533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-frame.elf" 0xff010113 0x00b50533 0x00c50533 0x00d50533 0x00e50533 0x00f50533 0x01050533 0x01150533 0x01013283 0x00550533 0x00a13423 0x00813503 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-split-load.elf" --split-data64 0x7b 0x00002517 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dynrel.elf" --dyn-relative64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relr.elf" --dyn-relr64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relr-bitmap.elf" --dyn-relr-bitmap64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-irelative.elf" --dyn-irelative64 0x7b --export-at poly_entry 12 0x00002517 0x00850513 0x00008067 0x00002517 0xff453503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dynsym.elf" --dyn-symbol64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dyntab.elf" --dyn-symbol64 0x7b 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dyntab-entry.elf" --dyn-symbol64 0x7b --export-dyntab-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-jumprel.elf" --dyn-jump-slot64 0x7b 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import.elf" --dyn-import64 poly_import_value 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-func.elf" --dyn-import-func64 poly_import_add 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-mul.elf" --dyn-import-func64 poly_import_mul 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-x86.elf" --dyn-import-func64 poly_import_x86_add 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mem.elf" 0x04d00513 0x00a5b023 0x00000513 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memwidth.elf" 0xff200293 0x00558023 0x0005c303 0x00058383 0x00730333 0xffe00293 0x0055a223 0x0045ee03 0x0045ae83 0x01de0e33 0x01c30533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-strlen.elf" 0x00100893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memfill.elf" 0x00400513 0x05200613 0x00200893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcmp.elf" 0x00400693 0x00158613 0x00300893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcpy.elf" 0x00400513 0x00158613 0x00400893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-read.elf" 0x00000513 0x00058593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-write.elf" 0x00100513 0x00058593 0x00500613 0x04000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-writev.elf" 0x00058293 0x0055b023 0x00500313 0x0065b423 0x00100513 0x00100613 0x04200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-lseek.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00700593 0x00000613 0x03e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00858593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read-close.elf" 0x00058813 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00a83023 0x00880593 0x00400613 0x03f00893 0x00000073 0x00083503 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-gettime.elf" 0x00000513 0x00058593 0x07100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpgid.elf" 0x00000513 0x09b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsid.elf" 0x00000513 0x09c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrusage.elf" 0x00000513 0x00058593 0x0a500893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getcpu.elf" 0x00058513 0x00858593 0x0a800893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-gettimeofday.elf" 0x00058513 0x00000593 0x0a900893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sysinfo.elf" 0x00058513 0x0b300893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap.elf" 0x00000513 0x0de00893 0x00000073 0x40c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap6.elf" 0x00000513 0x01000593 0x00300613 0x02200693 0x00500713 0x00700793 0x0de00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap-store.elf" 0x00000513 0x0de00893 0x00000073 0x04d00593 0x00b53023 0x00053503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpid.elf" 0x0ac00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getppid.elf" 0x0ad00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getuid.elf" 0x0ae00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-geteuid.elf" 0x0af00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getgid.elf" 0x0b000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getegid.elf" 0x0b100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-gettid.elf" 0x0b200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getcwd.elf" 0x00058513 0x01000593 0x01100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-uname.elf" 0x00058513 0x0a000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-exit.elf" 0x00700513 0x05d00893 0x00000073 0x06300513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ecall.elf" 0x00700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ebreak.elf" 0x00500893 0x00100073
  local -a riscv_long=(0x00000513)
  for _ in $(seq 1 80); do
    riscv_long+=(0x00150513)
  done
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-long.elf" "${riscv_long[@]}"
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
  build_poly_call
  build_poly_thread
  build_poly_signal
  build_poly_bench
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
  cp "$POLY_CALL_BIN" "$TMP_DIR/initramfs-root/usr/bin/polycall"
  cp "$POLY_THREAD_BIN" "$TMP_DIR/initramfs-root/usr/bin/polythread"
  cp "$POLY_SIGNAL_BIN" "$TMP_DIR/initramfs-root/usr/bin/polysignal"
  cp "$POLY_BENCH_BIN" "$TMP_DIR/initramfs-root/usr/bin/polybench"
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
RUN_POLY_EXEC="$RUN_POLY_EXEC"
RUN_POLY_CALL="$RUN_POLY_CALL"
RUN_POLY_THREAD="$RUN_POLY_THREAD"
RUN_POLY_SIGNAL="$RUN_POLY_SIGNAL"
RUN_POLY_BENCH="$RUN_POLY_BENCH"
RUN_POLY_BINFMT="$RUN_POLY_BINFMT"
RUN_NATIVE_CHECK="$RUN_NATIVE_CHECK"
EXPECT_POLY_CPUID="$EXPECT_POLY_CPUID"

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

if [ "$RUN_NATIVE_CHECK" = "1" ]; then
  EXPECT_POLY_CPUID="$EXPECT_POLY_CPUID" /usr/bin/nativecheck.elf >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_PROBE" = "1" ]; then
  /usr/bin/polyprobe >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_APPS" = "1" ]; then
  /usr/bin/polyapp /usr/lib/polyapps/*.poly >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_EXEC" = "1" ]; then
    /usr/bin/polyexec \
    /usr/lib/polyapps/aarch64-add.elf=132 \
    /usr/lib/polyapps/aarch64-regadd.elf=123 \
    /usr/lib/polyapps/aarch64-movwide.elf=0xffff6543edcb5678 \
    /usr/lib/polyapps/aarch64-mul.elf=42 \
    /usr/lib/polyapps/aarch64-logical.elf=60 \
    /usr/lib/polyapps/aarch64-shifted.elf=123 \
    /usr/lib/polyapps/aarch64-regmix.elf=12 \
    /usr/lib/polyapps/aarch64-branch.elf=42 \
    /usr/lib/polyapps/aarch64-condbranch.elf=91 \
    /usr/lib/polyapps/aarch64-loop.elf=0 \
    /usr/lib/polyapps/aarch64-ret.elf=55 \
    /usr/lib/polyapps/aarch64-mem.elf=77 \
    /usr/lib/polyapps/aarch64-memwidth.elf=0x100001324 \
    /usr/lib/polyapps/aarch64-strlen.elf=5 \
    /usr/lib/polyapps/aarch64-memfill.elf=4 \
    /usr/lib/polyapps/aarch64-memcmp.elf=1 \
    /usr/lib/polyapps/aarch64-memcpy.elf=4 \
    /usr/lib/polyapps/aarch64-read.elf=4 \
    /usr/lib/polyapps/aarch64-write.elf=5 \
    /usr/lib/polyapps/aarch64-writev.elf=5 \
    /usr/lib/polyapps/aarch64-openat.elf=3 \
    /usr/lib/polyapps/aarch64-openat-lseek.elf=7 \
    /usr/lib/polyapps/aarch64-openat-read.elf=4 \
    /usr/lib/polyapps/aarch64-openat-read-close.elf=0 \
    /usr/lib/polyapps/aarch64-clock-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-getpgid.elf=4242 \
    /usr/lib/polyapps/aarch64-getsid.elf=4242 \
    /usr/lib/polyapps/aarch64-getrusage.elf=321 \
    /usr/lib/polyapps/aarch64-getcpu.elf=12 \
    /usr/lib/polyapps/aarch64-gettimeofday.elf=246 \
    /usr/lib/polyapps/aarch64-sysinfo.elf=98765 \
    /usr/lib/polyapps/aarch64-mmap.elf=0 \
    /usr/lib/polyapps/aarch64-mmap6.elf=65 \
    /usr/lib/polyapps/aarch64-mmap-store.elf=77 \
    /usr/lib/polyapps/aarch64-getpid.elf=4242 \
    /usr/lib/polyapps/aarch64-getppid.elf=4241 \
    /usr/lib/polyapps/aarch64-getuid.elf=1000 \
    /usr/lib/polyapps/aarch64-geteuid.elf=1000 \
    /usr/lib/polyapps/aarch64-getgid.elf=1000 \
    /usr/lib/polyapps/aarch64-getegid.elf=1000 \
    /usr/lib/polyapps/aarch64-gettid.elf=4243 \
    /usr/lib/polyapps/aarch64-getcwd.elf=6 \
    /usr/lib/polyapps/aarch64-fp-int-move.elf=0x41400000 \
    /usr/lib/polyapps/aarch64-fp32-to-int.elf=12 \
    /usr/lib/polyapps/aarch64-fp32-to-int64.elf=12 \
    /usr/lib/polyapps/aarch64-uname.elf=0 \
    /usr/lib/polyapps/aarch64-exit.elf=7 \
    /usr/lib/polyapps/aarch64-brk.elf=0x4c000305 \
    /usr/lib/polyapps/aarch64-svc.elf=0x53000703 \
    /usr/lib/polyapps/aarch64-long.elf=80 \
    /usr/lib/polyapps/riscv-add.elf=27 \
    /usr/lib/polyapps/riscv-compressed.elf=27 \
    /usr/lib/polyapps/riscv-compressed-word.elf=27 \
    /usr/lib/polyapps/riscv-compressed-alu.elf=42 \
    /usr/lib/polyapps/riscv-compressed-fp.elf=64 \
    /usr/lib/polyapps/riscv-fp-int-move.elf=0xffffffff8000002a \
    /usr/lib/polyapps/riscv-fp-class.elf=32 \
    /usr/lib/polyapps/riscv-fp32-to-int.elf=12 \
    /usr/lib/polyapps/riscv-upper.elf=0x1234567c \
    /usr/lib/polyapps/riscv-immops.elf=42 \
    /usr/lib/polyapps/riscv-wordops.elf=42 \
    /usr/lib/polyapps/riscv-shiftcmp.elf=31 \
    /usr/lib/polyapps/riscv-divrem.elf=16 \
    /usr/lib/polyapps/riscv-shifts.elf=32 \
    /usr/lib/polyapps/riscv-srai.elf=0xfffffffffffffffc \
    /usr/lib/polyapps/riscv-regadd.elf=123 \
    /usr/lib/polyapps/riscv-mul.elf=42 \
    /usr/lib/polyapps/riscv-logical.elf=60 \
    /usr/lib/polyapps/riscv-regmix.elf=12 \
    /usr/lib/polyapps/riscv-branch.elf=42 \
    /usr/lib/polyapps/riscv-jal.elf=9 \
    /usr/lib/polyapps/riscv-jalr.elf=5 \
    /usr/lib/polyapps/riscv-branchcmp.elf=127 \
    /usr/lib/polyapps/riscv-condbranch.elf=91 \
    /usr/lib/polyapps/riscv-loop.elf=0 \
    /usr/lib/polyapps/riscv-ret.elf=55 \
    /usr/lib/polyapps/riscv-mem.elf=77 \
    /usr/lib/polyapps/riscv-memwidth.elf=0x1000000e0 \
    /usr/lib/polyapps/riscv-strlen.elf=5 \
    /usr/lib/polyapps/riscv-memfill.elf=4 \
    /usr/lib/polyapps/riscv-memcmp.elf=1 \
    /usr/lib/polyapps/riscv-memcpy.elf=4 \
    /usr/lib/polyapps/riscv-read.elf=4 \
    /usr/lib/polyapps/riscv-write.elf=5 \
    /usr/lib/polyapps/riscv-writev.elf=5 \
    /usr/lib/polyapps/riscv-openat.elf=3 \
    /usr/lib/polyapps/riscv-openat-lseek.elf=7 \
    /usr/lib/polyapps/riscv-openat-read.elf=4 \
    /usr/lib/polyapps/riscv-openat-read-close.elf=0 \
    /usr/lib/polyapps/riscv-clock-gettime.elf=0 \
    /usr/lib/polyapps/riscv-getpgid.elf=4242 \
    /usr/lib/polyapps/riscv-getsid.elf=4242 \
    /usr/lib/polyapps/riscv-getrusage.elf=321 \
    /usr/lib/polyapps/riscv-getcpu.elf=12 \
    /usr/lib/polyapps/riscv-gettimeofday.elf=246 \
    /usr/lib/polyapps/riscv-sysinfo.elf=98765 \
    /usr/lib/polyapps/riscv-mmap.elf=0 \
    /usr/lib/polyapps/riscv-mmap6.elf=65 \
    /usr/lib/polyapps/riscv-mmap-store.elf=77 \
    /usr/lib/polyapps/riscv-getpid.elf=4242 \
    /usr/lib/polyapps/riscv-getppid.elf=4241 \
    /usr/lib/polyapps/riscv-getuid.elf=1000 \
    /usr/lib/polyapps/riscv-geteuid.elf=1000 \
    /usr/lib/polyapps/riscv-getgid.elf=1000 \
    /usr/lib/polyapps/riscv-getegid.elf=1000 \
    /usr/lib/polyapps/riscv-gettid.elf=4243 \
    /usr/lib/polyapps/riscv-getcwd.elf=6 \
    /usr/lib/polyapps/riscv-uname.elf=0 \
    /usr/lib/polyapps/riscv-exit.elf=7 \
    /usr/lib/polyapps/riscv-ebreak.elf=0x4c000405 \
    /usr/lib/polyapps/riscv-ecall.elf=0x53000704 \
    /usr/lib/polyapps/riscv-long.elf=80 >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_CALL" = "1" ]; then
    /usr/bin/polycall \
    /usr/lib/polyapps/aarch64-pcall-sum.elf=21 \
    /usr/lib/polyapps/riscv-pcall-sum.elf=21 \
    /usr/lib/polyapps/aarch64-pcall-sum8.elf=36 \
    /usr/lib/polyapps/riscv-pcall-sum8.elf=36 \
    /usr/lib/polyapps/aarch64-pcall-sum9.elf=45 \
    /usr/lib/polyapps/aarch64-pcall-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-state.so#poly_entry=83 \
    /usr/lib/polyapps/aarch64-pcall-import-real.so#poly_entry=145 \
    /usr/lib/polyapps/aarch64-pcall-libc-import-real.so#poly_entry=421 \
    /usr/lib/polyapps/aarch64-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/aarch64-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/aarch64-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/aarch64-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/aarch64-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/aarch64-pcall-ctor-real.so#poly_entry=245 \
    /usr/lib/polyapps/aarch64-pcall-cond-real.so#poly_entry=115 \
    /usr/lib/polyapps/aarch64-pcall-select-variants-real.so#poly_entry=266 \
    /usr/lib/polyapps/aarch64-pcall-cbz-real.so#poly_entry=183 \
    /usr/lib/polyapps/aarch64-pcall-bitbranch-real.so#poly_entry=132 \
    /usr/lib/polyapps/aarch64-pcall-ubfm-real.so#poly_entry=524355 \
    /usr/lib/polyapps/aarch64-pcall-sbfm-real.so#poly_entry=4 \
    /usr/lib/polyapps/aarch64-pcall-signed-ext-real.so#poly_entry=29 \
    /usr/lib/polyapps/aarch64-pcall-signed-load-real.so#poly_entry=0xfffffffffffea228 \
    /usr/lib/polyapps/aarch64-pcall-int-div-real.so#poly_entry=65 \
    /usr/lib/polyapps/aarch64-pcall-int-madd-real.so#poly_entry=4294967334 \
    /usr/lib/polyapps/aarch64-pcall-int-highmul-real.so#poly_entry=9265686011975198326 \
    /usr/lib/polyapps/aarch64-pcall-int-carry-real.so#poly_entry=0x8000000000000021 \
    /usr/lib/polyapps/aarch64-pcall-int-varshift-real.so#poly_entry=0xe5d48b633e422ba5 \
    /usr/lib/polyapps/aarch64-pcall-int-logic-real.so#poly_entry=0x21d9737d81792d5e \
    /usr/lib/polyapps/aarch64-pcall-int-bitops-real.so#poly_entry=0xe5caa38822572301 \
    /usr/lib/polyapps/aarch64-pcall-int-rotate-real.so#poly_entry=0xbc1e4a9e37a5682e \
    /usr/lib/polyapps/aarch64-pcall-int-ccmp-real.so#poly_entry=5 \
    /usr/lib/polyapps/aarch64-pcall-postindex-mem.so#poly_entry=68 \
    /usr/lib/polyapps/aarch64-pcall-atomic.so#poly_entry=350 \
    /usr/lib/polyapps/aarch64-pcall-atomic-outline.so#poly_entry=350 \
    /usr/lib/polyapps/aarch64-pcall-atomic-lse.so#poly_entry=350 \
    /usr/lib/polyapps/aarch64-pcall-unscaled-mem-real.so#poly_entry=0xffffffffffffffc1 \
    /usr/lib/polyapps/aarch64-pcall-indexed-mem-real.so#poly_entry=41 \
    /usr/lib/polyapps/aarch64-pcall-callee-real.so#poly_entry=420 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
    fpair:/usr/lib/polyapps/aarch64-pcall-fpair-real.so#poly_entry=0x40268000400b0000 \
    fpairarg:/usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so#poly_entry=0x4026800000000000 \
    mixedargs:/usr/lib/polyapps/aarch64-pcall-mixed-args-real.so#poly_entry=0x40a9320000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-import-real.so#poly_entry=0x402b800000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-callee-real.so#poly_entry=0x4040400000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-cond-real.so#poly_entry=0xbfe8000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-div-real.so#poly_entry=0xc002aaaaaaaaaaab \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-unary-real.so#poly_entry=0x400e000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-abs-real.so#poly_entry=0x401b000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-sqrt-real.so#poly_entry=0x4016e6238502484c \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-fma-real.so#poly_entry=0x4019800000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-fma-variants-real.so#poly_entry=0xc019800000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-minmax-real.so#poly_entry=0x4012000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-select-real.so#poly_entry=0x4002000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-indexed-mem-real.so#poly_entry=0x401e000000000000 \
    /usr/lib/polyapps/aarch64-pcall-fp64-convert-real.so#poly_entry=4 \
    /usr/lib/polyapps/aarch64-pcall-fp64-signed-convert-real.so#poly_entry=12 \
    /usr/lib/polyapps/aarch64-pcall-fp64-i32-convert-real.so#poly_entry=4 \
    /usr/lib/polyapps/aarch64-pcall-fp64-u32-convert-real.so#poly_entry=4 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp-mixed-convert-real.so#poly_entry=0x4025000000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-int-fp-convert-real.so#poly_entry=0x4004000000000000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-real.so#poly_entry=0x41340000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-abs-real.so#poly_entry=0x40d80000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-sqrt-real.so#poly_entry=0x40b7311c \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-fma-real.so#poly_entry=0x40cc0000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-fma-variants-real.so#poly_entry=0xc0cc0000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-minmax-real.so#poly_entry=0x40900000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-select-real.so#poly_entry=0x40100000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-mem-real.so#poly_entry=0x3f400000 \
    /usr/lib/polyapps/riscv-pcall-sum9.elf=45 \
    /usr/lib/polyapps/riscv-pcall-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-state.so#poly_entry=83 \
    /usr/lib/polyapps/riscv-pcall-import-real.so#poly_entry=145 \
    /usr/lib/polyapps/riscv-pcall-libc-import-real.so#poly_entry=421 \
    /usr/lib/polyapps/riscv-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/riscv-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/riscv-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/riscv-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/riscv-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/riscv-pcall-ctor-real.so#poly_entry=245 \
    /usr/lib/polyapps/riscv-pcall-cond-real.so#poly_entry=115 \
    /usr/lib/polyapps/riscv-pcall-select-variants-real.so#poly_entry=266 \
    /usr/lib/polyapps/riscv-pcall-cbz-real.so#poly_entry=183 \
    /usr/lib/polyapps/riscv-pcall-bitbranch-real.so#poly_entry=132 \
    /usr/lib/polyapps/riscv-pcall-ubfm-real.so#poly_entry=524355 \
    /usr/lib/polyapps/riscv-pcall-sbfm-real.so#poly_entry=4 \
    /usr/lib/polyapps/riscv-pcall-signed-ext-real.so#poly_entry=29 \
    /usr/lib/polyapps/riscv-pcall-signed-load-real.so#poly_entry=0xfffffffffffea228 \
    /usr/lib/polyapps/riscv-pcall-int-div-real.so#poly_entry=65 \
    /usr/lib/polyapps/riscv-pcall-int-madd-real.so#poly_entry=4294967334 \
    /usr/lib/polyapps/riscv-pcall-int-highmul-real.so#poly_entry=9265686011975198326 \
    /usr/lib/polyapps/riscv-pcall-int-carry-real.so#poly_entry=0x8000000000000021 \
    /usr/lib/polyapps/riscv-pcall-int-varshift-real.so#poly_entry=0xe5d48b633e422ba5 \
    /usr/lib/polyapps/riscv-pcall-int-logic-real.so#poly_entry=0x21d9737d81792d5e \
    /usr/lib/polyapps/riscv-pcall-int-bitops-real.so#poly_entry=0xe5caa38822572301 \
    /usr/lib/polyapps/riscv-pcall-int-rotate-real.so#poly_entry=0xbc1e4a9e37a5682e \
    /usr/lib/polyapps/riscv-pcall-int-ccmp-real.so#poly_entry=5 \
    /usr/lib/polyapps/riscv-pcall-atomic.so#poly_entry=350 \
    /usr/lib/polyapps/riscv-pcall-unscaled-mem-real.so#poly_entry=0xffffffffffffffc1 \
    /usr/lib/polyapps/riscv-pcall-indexed-mem-real.so#poly_entry=41 \
    /usr/lib/polyapps/riscv-pcall-callee-real.so#poly_entry=420 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
    fpair:/usr/lib/polyapps/riscv-pcall-fpair-real.so#poly_entry=0x40268000400b0000 \
    fpairarg:/usr/lib/polyapps/riscv-pcall-fpair-arg-real.so#poly_entry=0x4026800000000000 \
    mixedargs:/usr/lib/polyapps/riscv-pcall-mixed-args-real.so#poly_entry=0x40a9320000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-import-real.so#poly_entry=0x402b800000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-callee-real.so#poly_entry=0x4040400000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-cond-real.so#poly_entry=0xbfe8000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-div-real.so#poly_entry=0xc002aaaaaaaaaaab \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-unary-real.so#poly_entry=0x400e000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-abs-real.so#poly_entry=0x401b000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-sqrt-real.so#poly_entry=0x4016e6238502484c \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-fma-real.so#poly_entry=0x4019800000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-fma-variants-real.so#poly_entry=0xc019800000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-minmax-real.so#poly_entry=0x4012000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-select-real.so#poly_entry=0x4002000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-indexed-mem-real.so#poly_entry=0x401e000000000000 \
    /usr/lib/polyapps/riscv-pcall-fp64-convert-real.so#poly_entry=4 \
    /usr/lib/polyapps/riscv-pcall-fp64-signed-convert-real.so#poly_entry=12 \
    /usr/lib/polyapps/riscv-pcall-fp64-i32-convert-real.so#poly_entry=4 \
    /usr/lib/polyapps/riscv-pcall-fp64-u32-convert-real.so#poly_entry=4 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp-mixed-convert-real.so#poly_entry=0x4025000000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-int-fp-convert-real.so#poly_entry=0x4004000000000000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-real.so#poly_entry=0x41340000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-abs-real.so#poly_entry=0x40d80000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-sqrt-real.so#poly_entry=0x40b7311c \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-fma-real.so#poly_entry=0x40cc0000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-fma-variants-real.so#poly_entry=0xc0cc0000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-minmax-real.so#poly_entry=0x40900000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-select-real.so#poly_entry=0x40100000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-mem-real.so#poly_entry=0x3f400000 \
    /usr/lib/polyapps/aarch64-pcall-frame.elf=45 \
    /usr/lib/polyapps/aarch64-pcall-native-frame.elf=3 \
    /usr/lib/polyapps/aarch64-pcall-bl.elf=3 \
    /usr/lib/polyapps/aarch64-pcall-adrp.elf=42 \
    /usr/lib/polyapps/aarch64-pcall-cond.elf=42 \
    /usr/lib/polyapps/aarch64-pcall-split-load.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dyntab.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-dyntab-entry.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-import.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-import-func.elf=103 \
    /usr/lib/polyapps/aarch64-pcall-import-mul.elf=102 \
    /usr/lib/polyapps/aarch64-pcall-import-x86.elf=203 \
    /usr/lib/polyapps/riscv-pcall-frame.elf=45 \
    /usr/lib/polyapps/riscv-pcall-split-load.elf=123 \
    /usr/lib/polyapps/riscv-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dyntab.elf=123 \
    /usr/lib/polyapps/riscv-pcall-dyntab-entry.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-pcall-import.elf=123 \
    /usr/lib/polyapps/riscv-pcall-import-func.elf=103 \
    /usr/lib/polyapps/riscv-pcall-import-mul.elf=102 \
    /usr/lib/polyapps/riscv-pcall-import-x86.elf=203 >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_THREAD" = "1" ]; then
  /usr/bin/polythread >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_SIGNAL" = "1" ]; then
  /usr/bin/polysignal >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_BENCH" = "1" ]; then
  /usr/bin/polybench >/dev/ttyS0 2>&1
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
    /usr/lib/polyapps/aarch64-movwide.elf \
    /usr/lib/polyapps/aarch64-mul.elf \
    /usr/lib/polyapps/aarch64-logical.elf \
    /usr/lib/polyapps/aarch64-shifted.elf \
    /usr/lib/polyapps/aarch64-regmix.elf \
    /usr/lib/polyapps/aarch64-branch.elf \
    /usr/lib/polyapps/aarch64-condbranch.elf \
    /usr/lib/polyapps/aarch64-loop.elf \
    /usr/lib/polyapps/aarch64-ret.elf \
    /usr/lib/polyapps/aarch64-mem.elf \
    /usr/lib/polyapps/aarch64-memwidth.elf \
    /usr/lib/polyapps/aarch64-strlen.elf \
    /usr/lib/polyapps/aarch64-memfill.elf \
    /usr/lib/polyapps/aarch64-memcmp.elf \
    /usr/lib/polyapps/aarch64-memcpy.elf \
    /usr/lib/polyapps/aarch64-read.elf \
    /usr/lib/polyapps/aarch64-write.elf \
    /usr/lib/polyapps/aarch64-writev.elf \
    /usr/lib/polyapps/aarch64-openat.elf \
    /usr/lib/polyapps/aarch64-openat-lseek.elf \
    /usr/lib/polyapps/aarch64-openat-read.elf \
    /usr/lib/polyapps/aarch64-openat-read-close.elf \
    /usr/lib/polyapps/aarch64-clock-gettime.elf \
    /usr/lib/polyapps/aarch64-getpgid.elf \
    /usr/lib/polyapps/aarch64-getsid.elf \
    /usr/lib/polyapps/aarch64-getrusage.elf \
    /usr/lib/polyapps/aarch64-getcpu.elf \
    /usr/lib/polyapps/aarch64-gettimeofday.elf \
    /usr/lib/polyapps/aarch64-sysinfo.elf \
    /usr/lib/polyapps/aarch64-mmap.elf \
    /usr/lib/polyapps/aarch64-mmap6.elf \
    /usr/lib/polyapps/aarch64-mmap-store.elf \
    /usr/lib/polyapps/aarch64-getpid.elf \
    /usr/lib/polyapps/aarch64-getppid.elf \
    /usr/lib/polyapps/aarch64-getuid.elf \
    /usr/lib/polyapps/aarch64-geteuid.elf \
    /usr/lib/polyapps/aarch64-getgid.elf \
    /usr/lib/polyapps/aarch64-getegid.elf \
    /usr/lib/polyapps/aarch64-gettid.elf \
    /usr/lib/polyapps/aarch64-getcwd.elf \
    /usr/lib/polyapps/aarch64-fp-int-move.elf \
    /usr/lib/polyapps/aarch64-fp32-to-int.elf \
    /usr/lib/polyapps/aarch64-fp32-to-int64.elf \
    /usr/lib/polyapps/aarch64-uname.elf \
    /usr/lib/polyapps/aarch64-exit.elf \
    /usr/lib/polyapps/aarch64-brk.elf \
    /usr/lib/polyapps/aarch64-svc.elf \
    /usr/lib/polyapps/aarch64-long.elf \
    /usr/lib/polyapps/riscv-add.elf \
    /usr/lib/polyapps/riscv-compressed.elf \
    /usr/lib/polyapps/riscv-compressed-word.elf \
    /usr/lib/polyapps/riscv-compressed-alu.elf \
    /usr/lib/polyapps/riscv-compressed-fp.elf \
    /usr/lib/polyapps/riscv-fp-int-move.elf \
    /usr/lib/polyapps/riscv-fp-class.elf \
    /usr/lib/polyapps/riscv-fp32-to-int.elf \
    /usr/lib/polyapps/riscv-upper.elf \
    /usr/lib/polyapps/riscv-immops.elf \
    /usr/lib/polyapps/riscv-wordops.elf \
    /usr/lib/polyapps/riscv-shiftcmp.elf \
    /usr/lib/polyapps/riscv-divrem.elf \
    /usr/lib/polyapps/riscv-shifts.elf \
    /usr/lib/polyapps/riscv-srai.elf \
    /usr/lib/polyapps/riscv-regadd.elf \
    /usr/lib/polyapps/riscv-mul.elf \
    /usr/lib/polyapps/riscv-logical.elf \
    /usr/lib/polyapps/riscv-regmix.elf \
    /usr/lib/polyapps/riscv-branch.elf \
    /usr/lib/polyapps/riscv-jal.elf \
    /usr/lib/polyapps/riscv-jalr.elf \
    /usr/lib/polyapps/riscv-branchcmp.elf \
    /usr/lib/polyapps/riscv-condbranch.elf \
    /usr/lib/polyapps/riscv-loop.elf \
    /usr/lib/polyapps/riscv-ret.elf \
    /usr/lib/polyapps/riscv-mem.elf \
    /usr/lib/polyapps/riscv-memwidth.elf \
    /usr/lib/polyapps/riscv-strlen.elf \
    /usr/lib/polyapps/riscv-memfill.elf \
    /usr/lib/polyapps/riscv-memcmp.elf \
    /usr/lib/polyapps/riscv-memcpy.elf \
    /usr/lib/polyapps/riscv-read.elf \
    /usr/lib/polyapps/riscv-write.elf \
    /usr/lib/polyapps/riscv-writev.elf \
    /usr/lib/polyapps/riscv-openat.elf \
    /usr/lib/polyapps/riscv-openat-lseek.elf \
    /usr/lib/polyapps/riscv-openat-read.elf \
    /usr/lib/polyapps/riscv-openat-read-close.elf \
    /usr/lib/polyapps/riscv-clock-gettime.elf \
    /usr/lib/polyapps/riscv-getpgid.elf \
    /usr/lib/polyapps/riscv-getsid.elf \
    /usr/lib/polyapps/riscv-getrusage.elf \
    /usr/lib/polyapps/riscv-getcpu.elf \
    /usr/lib/polyapps/riscv-gettimeofday.elf \
    /usr/lib/polyapps/riscv-sysinfo.elf \
    /usr/lib/polyapps/riscv-mmap.elf \
    /usr/lib/polyapps/riscv-mmap6.elf \
    /usr/lib/polyapps/riscv-mmap-store.elf \
    /usr/lib/polyapps/riscv-getpid.elf \
    /usr/lib/polyapps/riscv-getppid.elf \
    /usr/lib/polyapps/riscv-getuid.elf \
    /usr/lib/polyapps/riscv-geteuid.elf \
    /usr/lib/polyapps/riscv-getgid.elf \
    /usr/lib/polyapps/riscv-getegid.elf \
    /usr/lib/polyapps/riscv-gettid.elf \
    /usr/lib/polyapps/riscv-getcwd.elf \
    /usr/lib/polyapps/riscv-uname.elf \
    /usr/lib/polyapps/riscv-exit.elf \
    /usr/lib/polyapps/riscv-ebreak.elf \
    /usr/lib/polyapps/riscv-ecall.elf \
    /usr/lib/polyapps/riscv-long.elf
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
  local fatal_pattern='Kernel panic|Segmentation fault|segfault|Oops|general protection|BUG:|poly_raw: unhandled|POLY[A-Z_]*_FAIL'
  while (( SECONDS < deadline )); do
    if grep -Eiq "$fatal_pattern" "$SERIAL_LOG" "$BOCHS_LOG" 2>/dev/null; then
      success=-1
      break
    fi

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
      fi
      if [[ "$RUN_POLY_EXEC" == "1" ]]; then
        if ! grep -q "POLYEXEC_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_CALL" == "1" ]]; then
        if ! grep -q "POLYCALL_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_THREAD" == "1" ]]; then
        if ! grep -q "POLYTHREAD_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_SIGNAL" == "1" ]]; then
        if ! grep -q "POLYSIGNAL_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_BENCH" == "1" ]]; then
        if ! grep -q "POLYBENCH_OK" "$SERIAL_LOG"; then
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
      if [[ "$RUN_NATIVE_CHECK" == "1" ]]; then
        if ! grep -q "NATIVE_CHECK_OK" "$SERIAL_LOG"; then
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

  if (( success > 0 )); then
    echo "Boot smoke test passed."
    exit 0
  fi

  if (( success < 0 )); then
    echo "Boot smoke test failed due to a fatal guest or emulator log pattern."
  else
    echo "Boot smoke test failed."
  fi
  echo "Serial log:"
  cat "$SERIAL_LOG" || true
  echo "Console log:"
  cat "$CONSOLE_LOG" || true
  echo "Bochs log:"
  cat "$BOCHS_LOG" || true
  exit 1
}

main "$@"
