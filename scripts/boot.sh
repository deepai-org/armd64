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
POLY_CPUID_HEADER="$ROOT_DIR/tools/polycpuid.h"
POLY_APP_SRC="$ROOT_DIR/tools/polyapp.c"
POLY_APP_BIN="$OUT_DIR/polyapp"
POLY_EXEC_SRC="$ROOT_DIR/tools/polyexec.c"
POLY_EXEC_BIN="$OUT_DIR/polyexec"
POLY_CALL_SRC="$ROOT_DIR/tools/polycall.c"
POLY_CALL_X86_HELPERS_SRC="$ROOT_DIR/tools/polycall_x86_helpers.c"
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
POLYCALL_STACK_PROTECTOR_REAL_SRC="$ROOT_DIR/tools/polycall_stack_protector_real.c"
POLYCALL_ERRNO_REAL_SRC="$ROOT_DIR/tools/polycall_errno_real.c"
POLYCALL_GETAUXVAL_REAL_SRC="$ROOT_DIR/tools/polycall_getauxval_real.c"
POLYCALL_GETPAGESIZE_REAL_SRC="$ROOT_DIR/tools/polycall_getpagesize_real.c"
POLYCALL_SYSCONF_REAL_SRC="$ROOT_DIR/tools/polycall_sysconf_real.c"
POLYCALL_ENV_REAL_SRC="$ROOT_DIR/tools/polycall_env_real.c"
POLYCALL_ALLOC_REAL_SRC="$ROOT_DIR/tools/polycall_alloc_real.c"
POLYCALL_STRDUP_REAL_SRC="$ROOT_DIR/tools/polycall_strdup_real.c"
POLYCALL_ALIGNED_ALLOC_REAL_SRC="$ROOT_DIR/tools/polycall_aligned_alloc_real.c"
POLYCALL_ATEXIT_REAL_SRC="$ROOT_DIR/tools/polycall_atexit_real.c"
POLYCALL_PROCESS_REAL_SRC="$ROOT_DIR/tools/polycall_process_real.c"
POLYCALL_NEEDED_LEAF_REAL_SRC="$ROOT_DIR/tools/polycall_needed_leaf_real.c"
POLYCALL_NEEDED_DEP_REAL_SRC="$ROOT_DIR/tools/polycall_needed_dep_real.c"
POLYCALL_NEEDED_OVERRIDE_REAL_SRC="$ROOT_DIR/tools/polycall_needed_override_real.c"
POLYCALL_NEEDED_EXTRA_REAL_SRC="$ROOT_DIR/tools/polycall_needed_extra_real.c"
POLYCALL_NEEDED_MAIN_REAL_SRC="$ROOT_DIR/tools/polycall_needed_main_real.c"
POLYCALL_FUNCPTR_REAL_SRC="$ROOT_DIR/tools/polycall_funcptr_real.c"
POLYCALL_PAIR_REAL_SRC="$ROOT_DIR/tools/polycall_pair_real.c"
POLYCALL_SRET_REAL_SRC="$ROOT_DIR/tools/polycall_sret_real.c"
POLYCALL_CTOR_REAL_SRC="$ROOT_DIR/tools/polycall_ctor_real.c"
POLYCALL_FINI_REAL_SRC="$ROOT_DIR/tools/polycall_fini_real.c"
POLYCALL_TLS_REAL_SRC="$ROOT_DIR/tools/polycall_tls_real.c"
POLYCALL_TLS_INITIAL_EXEC_REAL_SRC="$ROOT_DIR/tools/polycall_tls_initial_exec_real.c"
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
POLYCALL_INT128_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_int128_helpers_real.c"
POLYCALL_INT128_FP_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_int128_fp_helpers_real.c"
POLYCALL_INT128_FLOAT_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_int128_float_helpers_real.c"
POLYCALL_BIT_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_bit_helpers_real.c"
POLYCALL_LONGDOUBLE_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_longdouble_helpers_real.c"
POLYCALL_LONGDOUBLE_SIGNED_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_longdouble_signed_helpers_real.c"
POLYCALL_LONGDOUBLE_COMPARE_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_longdouble_compare_helpers_real.c"
POLYCALL_LONGDOUBLE_INT32_HELPERS_REAL_SRC="$ROOT_DIR/tools/polycall_longdouble_int32_helpers_real.c"
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
POLYCALL_FP64_STACK_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_stack_real.c"
POLYCALL_FPAIR_REAL_SRC="$ROOT_DIR/tools/polycall_fpair_real.c"
POLYCALL_FPAIR32_REAL_SRC="$ROOT_DIR/tools/polycall_fpair32_real.c"
POLYCALL_FPAIR_ARG_REAL_SRC="$ROOT_DIR/tools/polycall_fpair_arg_real.c"
POLYCALL_FPAIR32_ARG_REAL_SRC="$ROOT_DIR/tools/polycall_fpair32_arg_real.c"
POLYCALL_HETERO_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_real.c"
POLYCALL_HETERO_REV_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_rev_real.c"
POLYCALL_HETERO32_REAL_SRC="$ROOT_DIR/tools/polycall_hetero32_real.c"
POLYCALL_HETERO32_REV_REAL_SRC="$ROOT_DIR/tools/polycall_hetero32_rev_real.c"
POLYCALL_HETERO_U32_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_u32_real.c"
POLYCALL_HETERO_U32_REV_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_u32_rev_real.c"
POLYCALL_HETERO_U32_F32_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_u32_f32_real.c"
POLYCALL_HETERO_F32_U32_REAL_SRC="$ROOT_DIR/tools/polycall_hetero_f32_u32_real.c"
POLYCALL_MIXED_ARGS_REAL_SRC="$ROOT_DIR/tools/polycall_mixed_args_real.c"
POLYCALL_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_import_real.c"
POLYCALL_X86_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_fp64_import_real.c"
POLYCALL_X86_FP64_SUM8_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_fp64_sum8_import_real.c"
POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_mixed_u64_fp64_import_real.c"
POLYCALL_X86_FP32_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_fp32_import_real.c"
POLYCALL_X86_SUM8_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_sum8_import_real.c"
POLYCALL_X86_SUM8_POST_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_x86_sum8_post_import_real.c"
POLYCALL_FP32_IMPORT_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_import_real.c"
POLYCALL_FP64_CALLEE_REAL_SRC="$ROOT_DIR/tools/polycall_fp64_callee_real.c"
POLYCALL_FP32_CALLEE_REAL_SRC="$ROOT_DIR/tools/polycall_fp32_callee_real.c"
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
POLY_COMPAT_TRAPS="${POLY_COMPAT_TRAPS:-1}"
RUN_POLY_PROBE="${RUN_POLY_PROBE:-0}"
RUN_POLY_APPS="${RUN_POLY_APPS:-0}"
RUN_POLY_EXEC="${RUN_POLY_EXEC:-$RUN_POLY_APPS}"
RUN_POLY_ARCH_TRAP_EXEC="${RUN_POLY_ARCH_TRAP_EXEC:-0}"
RUN_POLY_CALL="${RUN_POLY_CALL:-$RUN_POLY_APPS}"
RUN_POLY_THREAD="${RUN_POLY_THREAD:-$RUN_POLY_CALL}"
RUN_POLY_SIGNAL="${RUN_POLY_SIGNAL:-$RUN_POLY_THREAD}"
RUN_POLY_BENCH="${RUN_POLY_BENCH:-0}"
RUN_POLY_BINFMT="${RUN_POLY_BINFMT:-0}"
RUN_NATIVE_CHECK="${RUN_NATIVE_CHECK:-0}"
EXPECT_POLY_CPUID="${EXPECT_POLY_CPUID:-0}"
EXPECT_POLY_COMPAT_TRAPS="${EXPECT_POLY_COMPAT_TRAPS:-$POLY_COMPAT_TRAPS}"
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

  if [[ -x "$bin" && "$bin" -nt "$src" && "$bin" -nt "$POLY_CPUID_HEADER" ]]; then
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
  if [[ -x "$POLY_CALL_BIN" && "$POLY_CALL_BIN" -nt "$POLY_CALL_SRC" &&
      "$POLY_CALL_BIN" -nt "$POLY_CALL_X86_HELPERS_SRC" ]]; then
    return
  fi

  local compiler=""
  for candidate in "${POLY_CALL_CC:-}" x86_64-linux-gnu-gcc gcc-x86-64-linux-gnu cc gcc; do
    if [[ -n "$candidate" ]] && command -v "$candidate" >/dev/null 2>&1; then
      compiler="$candidate"
      break
    fi
  done

  if [[ -z "$compiler" ]]; then
    if [[ -x "$POLY_CALL_BIN" ]]; then
      return
    fi
    echo "No compiler available for $POLY_CALL_SRC and no prebuilt $POLY_CALL_BIN found." >&2
    exit 1
  fi

  local -a compiler_args=(-O2 -static -s -fno-stack-protector)
  if [[ "$compiler" == x86_64-linux-gnu-gcc || "$compiler" == gcc-x86-64-linux-gnu ]]; then
    compiler_args+=(--sysroot=/usr/x86_64-linux-gnu)
  fi
  "$compiler" "${compiler_args[@]}" "$POLY_CALL_SRC" \
    "$POLY_CALL_X86_HELPERS_SRC" -o "$POLY_CALL_BIN"
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
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fstack-protector-all \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STACK_PROTECTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-stack-protector-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ERRNO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-errno-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GETAUXVAL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-getauxval-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GETPAGESIZE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-getpagesize-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYSCONF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sysconf-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ENV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-env-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ALLOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-alloc-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRDUP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-strdup-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ALIGNED_ALLOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-aligned-alloc-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATEXIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-atexit-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PROCESS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-process-real.so"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneeded-leaf-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneeded-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_DEP_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneeded-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneeded-override-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_OVERRIDE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-override-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneeded-extra-a-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_EXTRA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-extra-a-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneeded-extra-b-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_EXTRA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyneeded-extra-b-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyneeded-aarch64.so \
    -Wl,--no-as-needed -l:libpolyneeded-override-aarch64.so \
    -Wl,--no-as-needed -l:libpolyneeded-extra-a-aarch64.so \
    -Wl,--no-as-needed -l:libpolyneeded-extra-b-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-real.so"
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
    "$POLYCALL_FINI_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fini-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-tls-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_INITIAL_EXEC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-tls-ie-real.so"
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
    "$POLYCALL_INT128_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int128-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT128_FP_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int128-fp-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT128_FLOAT_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-int128-float-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BIT_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-bit-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-longdouble-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_SIGNED_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-longdouble-signed-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_COMPARE_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-longdouble-compare-helpers-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_INT32_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-longdouble-int32-helpers-real.so"
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
    "$POLYCALL_FP64_STACK_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-stack-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR32_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fpair32-arg-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MIXED_ARGS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-mixed-args-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-rev-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO32_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero32-rev-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-u32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-u32-rev-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_F32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-u32-f32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_F32_U32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hetero-f32-u32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_SUM8_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp64-sum8-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp32-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM8_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sum8-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM8_POST_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sum8-post-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp64-callee-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fp32-callee-real.so"
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
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$RISCV64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-real-rv64gc.so"
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
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-real-rv64gc.so"
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
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fstack-protector-all \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STACK_PROTECTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-stack-protector-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ERRNO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-errno-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GETAUXVAL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-getauxval-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GETPAGESIZE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-getpagesize-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYSCONF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sysconf-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ENV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-env-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ALLOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-alloc-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRDUP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-strdup-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ALIGNED_ALLOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-aligned-alloc-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATEXIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-atexit-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PROCESS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-process-real.so"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneeded-leaf-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneeded-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_DEP_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneeded-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneeded-override-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_OVERRIDE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-override-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneeded-extra-a-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_EXTRA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneeded-extra-a-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneeded-extra-b-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_EXTRA_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyneeded-extra-b-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyneeded-riscv.so \
    -Wl,--no-as-needed -l:libpolyneeded-override-riscv.so \
    -Wl,--no-as-needed -l:libpolyneeded-extra-a-riscv.so \
    -Wl,--no-as-needed -l:libpolyneeded-extra-b-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-real.so"
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
    "$POLYCALL_FINI_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fini-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-tls-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_INITIAL_EXEC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-tls-ie-real.so"
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
    "$POLYCALL_INT128_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int128-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT128_FP_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int128-fp-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INT128_FLOAT_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-int128-float-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BIT_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-bit-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-longdouble-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_SIGNED_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-longdouble-signed-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_COMPARE_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-longdouble-compare-helpers-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_LONGDOUBLE_INT32_HELPERS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-longdouble-int32-helpers-real.so"
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
    "$POLYCALL_FP64_STACK_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-stack-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair-arg-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FPAIR32_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fpair32-arg-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MIXED_ARGS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-mixed-args-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-rev-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO32_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero32-rev-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-u32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_REV_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-u32-rev-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_U32_F32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-u32-f32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HETERO_F32_U32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-hetero-f32-u32-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_SUM8_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp64-sum8-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp32-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM8_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sum8-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM8_POST_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sum8-post-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP64_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp64-callee-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d -ffp-contract=off \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FP32_CALLEE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fp32-callee-real.so"
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-rel.elf" --dyn-rel-relative64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relr.elf" --dyn-relr64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf" --dyn-relr-bitmap64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-irelative.elf" --dyn-irelative64 0x7b --export-at poly_entry 12 0xd0000000 0x91002000 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dynsym.elf" --dyn-symbol64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dyntab.elf" --dyn-symbol64 0x7b 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dyntab-entry.elf" --dyn-symbol64 0x7b --export-dyntab-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-jumprel.elf" --dyn-jump-slot64 0x7b 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-rel-jumprel.elf" --dyn-rel-jump-slot64 0x7b 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import.elf" --dyn-import64 poly_import_value 0xd0000000 0x91000000 0xf9400000 0xf9400000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-func.elf" --dyn-import-func64 poly_import_add 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-mul.elf" --dyn-import-func64 poly_import_mul 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-x86.elf" --dyn-import-func64 poly_import_x86_add 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-x86-mul.elf" --dyn-import-func64 poly_import_x86_mul 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-x86-sum6.elf" --dyn-import-func64 poly_import_x86_sum6 0xa9bf7bfd 0xd0000008 0x91000108 0xf9400108 0xd63f0100 0xa8c17bfd 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mem.elf" 0xd28009a0 0xf9000020 0xd2800000 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memwidth.elf" 0x928001a5 0x39000025 0x39400026 0x92800025 0xb9000425 0xb9400427 0xd2824685 0x79001025 0x79401028 0x8b0700c0 0x8b080000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-strlen.elf" 0xd4200020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memfill.elf" 0xd2800080 0xd2800822 0xd4200040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcmp.elf" 0xd2800083 0x91000422 0xd4200060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcpy.elf" 0xd2800080 0x91000422 0xd4200080
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-eventfd2.elf" 0xd2800060 0xd2800001 0xd2800268 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-init1.elf" 0xd2800000 0xd2800348 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-add-watch.elf" 0xd28001c0 0xd2802002 0xd2800368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-rm-watch.elf" 0xd28001c0 0xd28003e1 0xd2800388 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-dup3.elf" 0xd28000a0 0xd2800101 0xd2800002 0xd2800308 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setxattr.elf" 0xaa0103e0 0xaa0103e1 0xaa0103e2 0xd2800083 0xd2800004 0xd28000a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-lsetxattr.elf" 0xaa0103e0 0xaa0103e1 0xaa0103e2 0xd2800083 0xd2800004 0xd28000c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fsetxattr.elf" 0xd2800060 0xaa0103e1 0xaa0103e2 0xd2800083 0xd2800004 0xd28000e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getxattr.elf" 0xaa0103e0 0xaa0103e1 0xaa0103e2 0xd2800103 0xd2800108 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-lgetxattr.elf" 0xaa0103e0 0xaa0103e1 0xaa0103e2 0xd2800103 0xd2800128 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fgetxattr.elf" 0xd2800060 0xaa0103e1 0xaa0103e2 0xd2800103 0xd2800148 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-listxattr.elf" 0xaa0103e0 0xaa0103e1 0xd2800202 0xd2800168 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-llistxattr.elf" 0xaa0103e0 0xaa0103e1 0xd2800202 0xd2800188 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-flistxattr.elf" 0xd2800060 0xaa0103e1 0xd2800202 0xd28001a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-removexattr.elf" 0xaa0103e0 0xaa0103e1 0xd28001c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-lremovexattr.elf" 0xaa0103e0 0xaa0103e1 0xd28001e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fremovexattr.elf" 0xd2800060 0xaa0103e1 0xd2800208 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ioprio-set.elf" 0xd2800020 0xd2800001 0xd2800002 0xd28003c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ioprio-get.elf" 0xd2800020 0xd2800001 0xd28003e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-flock.elf" 0xd2800060 0xd2800041 0xd2800408 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mknodat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2800003 0xd2800428 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mkdirat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2800448 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-unlinkat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2800468 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-symlinkat.elf" 0xaa0103e0 0xd2800001 0xaa0103e2 0xd2800488 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-linkat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xaa0103e3 0xd2800004 0xd28004a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-renameat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xaa0103e3 0xd28004c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-umount2.elf" 0xaa0103e0 0xd2800001 0xd28004e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mount.elf" 0xaa0103e0 0xaa0103e1 0xaa0103e2 0xd2800003 0xd2800004 0xd2800508 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pivot-root.elf" 0xaa0103e0 0xaa0103e1 0xd2800528 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-chroot.elf" 0xaa0103e0 0xd2800668 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-renameat2.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xaa0103e3 0xd2800004 0xd2802288 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-open-tree.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2803588 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-move-mount.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xaa0103e3 0xd2800004 0xd28035a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fsopen.elf" 0xaa0103e0 0xd2800001 0xd28035c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fsconfig.elf" 0xd2800200 0xd2800001 0xaa0103e2 0xaa0103e3 0xd2800004 0xd28035e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fsmount.elf" 0xd2800200 0xd2800001 0xd2800002 0xd2803608 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fspick.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2803628 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mount-setattr.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xaa0103e3 0xd2800004 0xd2803748 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pipe2.elf" 0xaa0103e2 0xaa0103e0 0xd2800001 0xd2800768 0xd4000001 0xb9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fsync.elf" 0xd2800060 0xd2800a48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fdatasync.elf" 0xd2800060 0xd2800a68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sync-file-range.elf" 0xd2800060 0xd2800001 0xd2800002 0xd2800003 0xd2800a88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fadvise64.elf" 0xd2800060 0xd2800001 0xd2800002 0xd2800003 0xd2801be8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-statfs.elf" 0xaa0103e0 0xd2800568 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fstatfs.elf" 0xd2800060 0xd2800588 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-truncate.elf" 0xaa0103e0 0xd2800001 0xd28005a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ftruncate.elf" 0xd2800060 0xd2800001 0xd28005c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fallocate.elf" 0xd2800060 0xd2800001 0xd2800002 0xd2800003 0xd28005e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-chdir.elf" 0xaa0103e0 0xd2800628 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fchdir.elf" 0xd2800060 0xd2800648 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fchmod.elf" 0xd2800060 0xd2800001 0xd2800688 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fchmodat.elf" 0xd2800000 0xd2800002 0xd2800003 0xd28006a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fchownat.elf" 0xd2800000 0xd2807d02 0xd2807d03 0xd2800004 0xd28006c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fchown.elf" 0xd2800060 0xd2807d01 0xd2807d02 0xd28006e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-create.elf" 0xd2800020 0xd2800001 0xd2800aa8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-settime.elf" 0xaa0103e4 0xd28001a0 0xd2800001 0xaa0403e2 0xaa0403e3 0xd2800ac8 0xd4000001 0xf9400880
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-gettime.elf" 0xaa0103e2 0xd28001a0 0xaa0203e1 0xd2800ae8 0xd4000001 0xf9400840
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-create.elf" 0xaa0103e2 0xd2800000 0xd2800001 0xd2800d68 0xd4000001 0xb9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-gettime.elf" 0xd28002e0 0xd2800d88 0xd4000001 0xf9400820
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-getoverrun.elf" 0xd28002e0 0xd2800da8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-settime.elf" 0xaa0103e2 0xaa0103e3 0xd28002e0 0xd2800001 0xd2800dc8 0xd4000001 0xf9400860
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-delete.elf" 0xd28002e0 0xd2800de8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-read.elf" 0xd2800000 0x91000021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-readv.elf" 0xaa0103e3 0xf9000023 0xd2800084 0xf9000424 0xd2800000 0xd2800022 0xd2800828 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-write.elf" 0xd2800020 0x91000021 0xd28000a2 0xd2800808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-writev.elf" 0xaa0103e3 0xf9000023 0xd28000a4 0xf9000424 0xd2800020 0xd2800022 0xd2800848 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pread64.elf" 0xd2800060 0x91000021 0xd2800082 0xd28000e3 0xd2800868 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pwrite64.elf" 0xd2800020 0x91000021 0xd28000a2 0xd28000e3 0xd2800888 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-preadv.elf" 0xaa0103e3 0xf9000023 0xd2800084 0xf9000424 0xd2800060 0xd2800022 0xd28000e3 0xd2800004 0xd28008a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pwritev.elf" 0xaa0103e3 0xf9000023 0xd28000a4 0xf9000424 0xd2800020 0xd2800022 0xd28000e3 0xd2800004 0xd28008c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pselect6.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2800003 0xd2800004 0xd2800005 0xd2800908 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ppoll.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2800003 0xd2800928 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-create1.elf" 0xd2800000 0xd2800288 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-ctl.elf" 0xd2800080 0xd2800041 0xd2800062 0xd2800003 0xd28002a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-pwait.elf" 0xd2800080 0xd2800001 0xd2800002 0xd2800003 0xd2800004 0xd2800005 0xd28002c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-nanosleep.elf" 0xaa0103e0 0xd2800001 0xd2800ca8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getitimer.elf" 0xd2800000 0xd2800cc8 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setitimer.elf" 0xd2800000 0xd2800002 0xd2800ce8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-nanosleep.elf" 0xaa0103e2 0xd2800020 0xd2800001 0xd2800003 0xd2800e68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-setparam.elf" 0xd2800000 0xd2800ec8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-setscheduler.elf" 0xaa0103e2 0xd2800000 0xd2800001 0xd2800ee8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-getscheduler.elf" 0xd2800000 0xd2800f08 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-getparam.elf" 0xd2800000 0xd2800f28 0xd4000001 0xb9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-setaffinity.elf" 0xaa0103e2 0xd2800000 0xd2800101 0xd2800f48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-getaffinity.elf" 0xaa0103e2 0xd2800000 0xd2800101 0xd2800f68 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-yield.elf" 0xd2800f88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-get-priority-max.elf" 0xd2800000 0xd2800fa8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sched-get-priority-min.elf" 0xd2800000 0xd2800fc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-socket.elf" 0xd2800040 0xd2800021 0xd2800002 0xd28018c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-socketpair.elf" 0xaa0103e3 0xd2800040 0xd2800021 0xd2800002 0xd28018e8 0xd4000001 0xb9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-bind.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2801908 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-listen.elf" 0xd28000a0 0xd2800021 0xd2801928 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-accept.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2801948 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-connect.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2801968 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsockname.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2801988 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpeername.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd28019a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sendto.elf" 0xd28000a0 0x91000021 0xd28000a2 0xd2800003 0xd2800004 0xd2800005 0xd28019c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-recvfrom.elf" 0xd28000a0 0x91000021 0xd2800082 0xd2800003 0xd2800004 0xd2800005 0xd28019e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setsockopt.elf" 0xd28000a0 0xd2800021 0xd2800042 0xd2800003 0xd2800004 0xd2801a08 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsockopt.elf" 0xd28000a0 0xd2800021 0xd2800042 0xd2800003 0xd2800004 0xd2801a28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-shutdown.elf" 0xd28000a0 0xd2800041 0xd2801a48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-accept4.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2800003 0xd2801e48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fcntl.elf" 0xd2800060 0xd2800001 0xd2800002 0xd2800328 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ioctl.elf" 0xd2800020 0xd2800001 0xd2800002 0xd28003a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-faccessat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2800003 0xd2800608 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-readlinkat.elf" 0xd2800000 0xaa0103e2 0xd2800203 0xd28009c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-newfstatat.elf" 0xd2800000 0xaa0103e2 0xd2800003 0xd28009e8 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fstat.elf" 0xd2800060 0xd2800a08 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-statx.elf" 0xd2800000 0xaa0103e4 0xd2800002 0xd2800003 0xd2802468 0xd4000001 0xf9400080
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getdents64.elf" 0xd2800060 0xaa0103e1 0xd2800802 0xd28007a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-lseek.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xd28000e1 0xd2800002 0xd28007c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0x91002021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read-close.elf" 0x91000026 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xf90000c0 0x910020c1 0xd2800082 0xd28007e8 0xd4000001 0xf94000c0 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-gettime.elf" 0xd2800000 0x91000021 0xd2800e28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-getres.elf" 0xd2800000 0x91000021 0xd2800e48 0xd4000001 0xf9400420
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-times.elf" 0x91000020 0xd2801328 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpgid.elf" 0xd2800000 0xd2801368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsid.elf" 0xd2800000 0xd2801388 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrlimit.elf" 0xd2800060 0xd2801468 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setrlimit.elf" 0xd2800060 0xd2801488 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrusage.elf" 0xd2800000 0x91000021 0xd28014a8 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getcpu.elf" 0x91000020 0x91002021 0xd2801508 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-gettimeofday.elf" 0x91000020 0xd2800001 0xd2801528 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sysinfo.elf" 0x91000020 0xd2801668 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-capget.elf" 0xaa0103e2 0xaa0203e0 0x91002041 0xd2800b48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-capset.elf" 0xaa0103e2 0xaa0203e0 0x91002041 0xd2800b68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-personality.elf" 0x92800000 0xd2800b88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-waitid.elf" 0xd2800be8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-wait4.elf" 0xd2802088 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setpriority.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2801188 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpriority.elf" 0xd2800000 0xd2800001 0xd28011a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setpgid.elf" 0xd2800000 0xd2800001 0xd2801348 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setsid.elf" 0xd28013a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-umask.elf" 0xd2800240 0xd28014c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-prctl-set-name.elf" 0xd28001e0 0xd28014e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setregid.elf" 0xd2807d00 0xd2807d01 0xd28011e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setgid.elf" 0xd2807d00 0xd2801208 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setreuid.elf" 0xd2807d00 0xd2807d01 0xd2801228 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setuid.elf" 0xd2807d00 0xd2801248 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setresuid.elf" 0xd2807d00 0xd2807d01 0xd2807d02 0xd2801268 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getresuid.elf" 0xaa0103e3 0xaa0303e0 0x91001061 0x91002062 0xd2801288 0xd4000001 0xb9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setresgid.elf" 0xd2807d00 0xd2807d01 0xd2807d02 0xd28012a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getresgid.elf" 0xaa0103e3 0xaa0303e0 0x91001061 0x91002062 0xd28012c8 0xd4000001 0xb9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setfsuid.elf" 0xd2807d00 0xd28012e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setfsgid.elf" 0xd2807d00 0xd2801308 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getgroups.elf" 0xaa0103e2 0xd2800020 0xaa0203e1 0xd28013c8 0xd4000001 0xb9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setgroups.elf" 0xd2800000 0xd2800001 0xd28013e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-kill.elf" 0xd2800000 0xd2800001 0xd2801028 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-tkill.elf" 0xd2800000 0xd2800001 0xd2801048 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-tgkill.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2801068 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sigaltstack.elf" 0xd2800000 0xd2801088 0xd4000001 0xb9400820
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rt-sigaction.elf" 0xd2800040 0xd2800001 0xd2800002 0xd2801003 0xd28010c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rt-sigprocmask.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2801003 0xd28010e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-futex.elf" 0xaa0103e0 0xd2800001 0xd2800002 0xd2800003 0xd2800c48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-madvise.elf" 0xaa0103e0 0xd2820001 0xd2800002 0xd2801d28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mremap.elf" 0xaa0103e9 0xaa0903e0 0xd2800801 0xd2801002 0xd2800003 0xd2800004 0xd2801b08 0xd4000001 0xcb090000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clone.elf" 0xd2801b88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-execve.elf" 0xd2801ba8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-membarrier-query.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2802368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-membarrier-cmd.elf" 0xd2800020 0xd2800001 0xd2800002 0xd2802368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rseq.elf" 0xaa0103e0 0xd2800401 0xd2800002 0xd2800003 0xd28024a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlock.elf" 0xd2801c88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-munlock.elf" 0xd2801ca8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlockall.elf" 0xd2801cc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-munlockall.elf" 0xd2801ce8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-get-mempolicy.elf" 0xd2801d88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-mempolicy.elf" 0xd2801da8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-migrate-pages.elf" 0xd2801dc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-move-pages.elf" 0xd2801de8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-seccomp.elf" 0xd28022a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-bpf.elf" 0xd2802308 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-userfaultfd.elf" 0xd2802348 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlock2.elf" 0xd2802388 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pkey-mprotect.elf" 0xd2802408 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pkey-alloc.elf" 0xd2802428 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pkey-free.elf" 0xd2802448 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pidfd-send-signal.elf" 0xd2803508 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-io-uring-setup.elf" 0xd2803528 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-io-uring-enter.elf" 0xd2803548 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-io-uring-register.elf" 0xd2803568 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pidfd-open.elf" 0xd2803648 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clone3.elf" 0xd2803668 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-close-range.elf" 0xd2803688 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat2.elf" 0xd28036a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pidfd-getfd.elf" 0xd28036c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-madvise.elf" 0xd2803708 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-landlock-create-ruleset.elf" 0xd2803788 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-landlock-add-rule.elf" 0xd28037a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-landlock-restrict-self.elf" 0xd28037c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-mrelease.elf" 0xd2803808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-futex-waitv.elf" 0xd2803828 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-mempolicy-home-node.elf" 0xd2803848 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-tid-address.elf" 0xaa0103e0 0xd2800c08 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-robust-list.elf" 0xaa0103e0 0xd2800301 0xd2800c68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-get-robust-list.elf" 0xd2800000 0x91002022 0xd2800c88 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-prlimit64.elf" 0xaa0103e3 0xd2800000 0xd2800061 0xd2800002 0xd28020a8 0xd4000001 0xf9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrandom.elf" 0xaa0103e0 0xd2800081 0xd2800002 0xd28022c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap.elf" 0xd2800000 0xd2801bc8 0xd4000001 0xcb020000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap6.elf" 0xd2800000 0xd2800201 0xd2800062 0xd2800443 0xd28000a4 0xd28000e5 0xd2801bc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap-store.elf" 0xd2800000 0xd2801bc8 0xd4000001 0xd28009a1 0xf9000001 0xf9400000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap-real-store.elf" 0xd2800000 0xd2820001 0xd2800062 0xd2800443 0x92800004 0xd2800005 0xd2801bc8 0xd4000001 0xd28009a1 0xf9000001 0xf9400000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sys-brk.elf" 0xd2800000 0xd2801ac8 0xd4000001 0xcb010000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-munmap.elf" 0xaa0103e0 0xd2820001 0xd2801ae8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mprotect.elf" 0xaa0103e0 0xd2820001 0xd2800022 0xd2801c48 0xd4000001
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-exit-group.elf" 0xd28000e0 0xd2800bc8 0xd4000001 0xd2800c80
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-svc.elf" 0xd40000e1
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-brk.elf" 0xd42000a0
  local -a aarch64_long=(0xd2800000)
  for _ in $(seq 1 80); do
    aarch64_long+=(0x91000400)
  done
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-long.elf" "${aarch64_long[@]}"
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-add.elf" 0x01f00513 0xffc50513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed.elf" 0x05194555 0x00018082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-half.elf" h:0x4555 h:0x0519 h:0x8082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-jalr.elf" h:0x456d 0x00008067
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-rel.elf" --dyn-rel-relative64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relr.elf" --dyn-relr64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relr-bitmap.elf" --dyn-relr-bitmap64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-irelative.elf" --dyn-irelative64 0x7b --export-at poly_entry 12 0x00002517 0x00850513 0x00008067 0x00002517 0xff453503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dynsym.elf" --dyn-symbol64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dyntab.elf" --dyn-symbol64 0x7b 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dyntab-entry.elf" --dyn-symbol64 0x7b --export-dyntab-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-jumprel.elf" --dyn-jump-slot64 0x7b 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-rel-jumprel.elf" --dyn-rel-jump-slot64 0x7b 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import.elf" --dyn-import64 poly_import_value 0x00002517 0x00053503 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-func.elf" --dyn-import-func64 poly_import_add 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-cjalr.elf" --dyn-import-func64 poly_import_add 0xff010113 0x00113423 0x00002297 0xff82b283 0x00019282 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-cjr.elf" --dyn-import-func64 poly_import_add 0x00002297 0x0002b283 0x00018282
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-mul.elf" --dyn-import-func64 poly_import_mul 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-x86.elf" --dyn-import-func64 poly_import_x86_add 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-x86-mul.elf" --dyn-import-func64 poly_import_x86_mul 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-import-x86-sum6.elf" --dyn-import-func64 poly_import_x86_sum6 0xff010113 0x00113423 0x00002297 0xff82b283 0x000280e7 0x00813083 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mem.elf" 0x04d00513 0x00a5b023 0x00000513 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memwidth.elf" 0xff200293 0x00558023 0x0005c303 0x00058383 0x00730333 0xffe00293 0x0055a223 0x0045ee03 0x0045ae83 0x01de0e33 0x01c30533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-strlen.elf" 0x00100893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memfill.elf" 0x00400513 0x05200613 0x00200893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcmp.elf" 0x00400693 0x00158613 0x00300893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcpy.elf" 0x00400513 0x00158613 0x00400893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-eventfd2.elf" 0x00300513 0x00000593 0x01300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-init1.elf" 0x00000513 0x01a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-add-watch.elf" 0x00e00513 0x10000613 0x01b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-rm-watch.elf" 0x00e00513 0x01f00593 0x01c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-dup3.elf" 0x00500513 0x00800593 0x00000613 0x01800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setxattr.elf" 0x00058513 0x00058613 0x00400693 0x00000713 0x00500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-lsetxattr.elf" 0x00058513 0x00058613 0x00400693 0x00000713 0x00600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fsetxattr.elf" 0x00300513 0x00058613 0x00400693 0x00000713 0x00700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getxattr.elf" 0x00058513 0x00058613 0x00800693 0x00800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-lgetxattr.elf" 0x00058513 0x00058613 0x00800693 0x00900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fgetxattr.elf" 0x00300513 0x00058613 0x00800693 0x00a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-listxattr.elf" 0x00058513 0x01000613 0x00b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-llistxattr.elf" 0x00058513 0x01000613 0x00c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-flistxattr.elf" 0x00300513 0x01000613 0x00d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-removexattr.elf" 0x00058513 0x00e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-lremovexattr.elf" 0x00058513 0x00f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fremovexattr.elf" 0x00300513 0x01000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ioprio-set.elf" 0x00100513 0x00000593 0x00000613 0x01e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ioprio-get.elf" 0x00100513 0x00000593 0x01f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-flock.elf" 0x00300513 0x00200593 0x02000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mknodat.elf" 0x00000513 0x00058613 0x00000693 0x02100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mkdirat.elf" 0x00000513 0x00058613 0x02200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-unlinkat.elf" 0x00000513 0x00058613 0x02300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-symlinkat.elf" 0x00058513 0x00000593 0x00058613 0x02400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-linkat.elf" 0x00000513 0x00058613 0x00000613 0x00058693 0x00000713 0x02500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-renameat.elf" 0x00000513 0x00058613 0x00000613 0x00058693 0x02600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-umount2.elf" 0x00058513 0x00000593 0x02700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mount.elf" 0x00058513 0x00058593 0x00058613 0x00000693 0x00000713 0x02800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pivot-root.elf" 0x00058513 0x00058593 0x02900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-chroot.elf" 0x00058513 0x03300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-renameat2.elf" 0x00000513 0x00058613 0x00000613 0x00058693 0x00000713 0x11400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-open-tree.elf" 0x00000513 0x00058613 0x00000613 0x1ac00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-move-mount.elf" 0x00000513 0x00058613 0x00000613 0x00058693 0x00000713 0x1ad00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fsopen.elf" 0x00058513 0x00000593 0x1ae00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fsconfig.elf" 0x01000513 0x00000593 0x00058613 0x00058693 0x00000713 0x1af00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fsmount.elf" 0x01000513 0x00000593 0x00000613 0x1b000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fspick.elf" 0x00000513 0x00058613 0x00000613 0x1b100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mount-setattr.elf" 0x00000513 0x00058613 0x00000613 0x00058693 0x00000713 0x1ba00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pipe2.elf" 0x00058613 0x00058513 0x00000593 0x03b00893 0x00000073 0x00062503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fsync.elf" 0x00300513 0x05200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fdatasync.elf" 0x00300513 0x05300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sync-file-range.elf" 0x00300513 0x00000593 0x00000613 0x00000693 0x05400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fadvise64.elf" 0x00300513 0x00000593 0x00000613 0x00000693 0x0df00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-statfs.elf" 0x00058513 0x02b00893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fstatfs.elf" 0x00300513 0x02c00893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-truncate.elf" 0x00058513 0x00000593 0x02d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ftruncate.elf" 0x00300513 0x00000593 0x02e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fallocate.elf" 0x00300513 0x00000593 0x00000613 0x00000693 0x02f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-chdir.elf" 0x00058513 0x03100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fchdir.elf" 0x00300513 0x03200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fchmod.elf" 0x00300513 0x00000593 0x03400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fchmodat.elf" 0x00000513 0x00000613 0x00000693 0x03500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fchownat.elf" 0x00000513 0x3e800613 0x3e800693 0x00000713 0x03600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fchown.elf" 0x00300513 0x3e800593 0x3e800613 0x03700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-create.elf" 0x00100513 0x00000593 0x05500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-settime.elf" 0x00058293 0x00d00513 0x00000593 0x00028613 0x00028693 0x05600893 0x00000073 0x0102b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-gettime.elf" 0x00058293 0x00d00513 0x00028593 0x05700893 0x00000073 0x0102b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-create.elf" 0x00058613 0x00000513 0x00000593 0x06b00893 0x00000073 0x00062503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-gettime.elf" 0x01700513 0x06c00893 0x00000073 0x0105b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-getoverrun.elf" 0x01700513 0x06d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-settime.elf" 0x00058613 0x00058693 0x01700513 0x00000593 0x06e00893 0x00000073 0x0106b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-delete.elf" 0x01700513 0x06f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-read.elf" 0x00000513 0x00058593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-readv.elf" 0x00058293 0x0055b023 0x00400313 0x0065b423 0x00000513 0x00100613 0x04100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-write.elf" 0x00100513 0x00058593 0x00500613 0x04000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-writev.elf" 0x00058293 0x0055b023 0x00500313 0x0065b423 0x00100513 0x00100613 0x04200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pread64.elf" 0x00300513 0x00058593 0x00400613 0x00700693 0x04300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pwrite64.elf" 0x00100513 0x00058593 0x00500613 0x00700693 0x04400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-preadv.elf" 0x00058293 0x0055b023 0x00400313 0x0065b423 0x00300513 0x00100613 0x00700693 0x00000713 0x04500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pwritev.elf" 0x00058293 0x0055b023 0x00500313 0x0065b423 0x00100513 0x00100613 0x00700693 0x00000713 0x04600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pselect6.elf" 0x00000513 0x00000593 0x00000613 0x00000693 0x00000713 0x00000793 0x04800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ppoll.elf" 0x00000513 0x00000593 0x00000613 0x00000693 0x04900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-create1.elf" 0x00000513 0x01400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-ctl.elf" 0x00400513 0x00200593 0x00300613 0x00000693 0x01500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-pwait.elf" 0x00400513 0x00000593 0x00000613 0x00000693 0x00000713 0x00000793 0x01600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-nanosleep.elf" 0x00058513 0x00000593 0x06500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getitimer.elf" 0x00000513 0x06600893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setitimer.elf" 0x00000513 0x00000613 0x06700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-nanosleep.elf" 0x00058613 0x00100513 0x00000593 0x00000693 0x07300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-setparam.elf" 0x00000513 0x07600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-setscheduler.elf" 0x00058613 0x00000513 0x00000593 0x07700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-getscheduler.elf" 0x00000513 0x07800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-getparam.elf" 0x00000513 0x07900893 0x00000073 0x0005a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-setaffinity.elf" 0x00058613 0x00000513 0x00800593 0x07a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-getaffinity.elf" 0x00058293 0x00000513 0x00800593 0x00028613 0x07b00893 0x00000073 0x0002b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-yield.elf" 0x07c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-get-priority-max.elf" 0x00000513 0x07d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sched-get-priority-min.elf" 0x00000513 0x07e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-socket.elf" 0x00200513 0x00100593 0x00000613 0x0c600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-socketpair.elf" 0x00058693 0x00200513 0x00100593 0x00000613 0x0c700893 0x00000073 0x0006a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-bind.elf" 0x00500513 0x00000593 0x00000613 0x0c800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-listen.elf" 0x00500513 0x00100593 0x0c900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-accept.elf" 0x00500513 0x00000593 0x00000613 0x0ca00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-connect.elf" 0x00500513 0x00000593 0x00000613 0x0cb00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsockname.elf" 0x00500513 0x00000593 0x00000613 0x0cc00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpeername.elf" 0x00500513 0x00000593 0x00000613 0x0cd00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sendto.elf" 0x00500513 0x00058593 0x00500613 0x00000693 0x00000713 0x00000793 0x0ce00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-recvfrom.elf" 0x00500513 0x00058593 0x00400613 0x00000693 0x00000713 0x00000793 0x0cf00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setsockopt.elf" 0x00500513 0x00100593 0x00200613 0x00000693 0x00000713 0x0d000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsockopt.elf" 0x00500513 0x00100593 0x00200613 0x00000693 0x00000713 0x0d100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-shutdown.elf" 0x00500513 0x00200593 0x0d200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-accept4.elf" 0x00500513 0x00000593 0x00000613 0x00000693 0x0f200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fcntl.elf" 0x00300513 0x00000593 0x00000613 0x01900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ioctl.elf" 0x00100513 0x00000593 0x00000613 0x01d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-faccessat.elf" 0x00000513 0x00058613 0x00000693 0x03000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-readlinkat.elf" 0x00000513 0x00058613 0x01000693 0x04e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-newfstatat.elf" 0x00000513 0x00058613 0x00000693 0x04f00893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fstat.elf" 0x00300513 0x05000893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-statx.elf" 0x00000513 0x00058713 0x00000613 0x00000693 0x12300893 0x00000073 0x00073503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getdents64.elf" 0x00300513 0x00058593 0x04000613 0x03d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-lseek.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00700593 0x00000613 0x03e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00858593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read-close.elf" 0x00058813 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00a83023 0x00880593 0x00400613 0x03f00893 0x00000073 0x00083503 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-gettime.elf" 0x00000513 0x00058593 0x07100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-getres.elf" 0x00000513 0x00058593 0x07200893 0x00000073 0x0085b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-times.elf" 0x00058513 0x09900893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpgid.elf" 0x00000513 0x09b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsid.elf" 0x00000513 0x09c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrlimit.elf" 0x00300513 0x0a300893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setrlimit.elf" 0x00300513 0x0a400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrusage.elf" 0x00000513 0x00058593 0x0a500893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getcpu.elf" 0x00058513 0x00858593 0x0a800893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-gettimeofday.elf" 0x00058513 0x00000593 0x0a900893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sysinfo.elf" 0x00058513 0x0b300893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-capget.elf" 0x00058293 0x00028513 0x00828593 0x05a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-capset.elf" 0x00058293 0x00028513 0x00828593 0x05b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-personality.elf" 0xfff00513 0x05c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-waitid.elf" 0x05f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-wait4.elf" 0x10400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setpriority.elf" 0x00000513 0x00000593 0x00000613 0x08c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpriority.elf" 0x00000513 0x00000593 0x08d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setpgid.elf" 0x00000513 0x00000593 0x09a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setsid.elf" 0x09d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-umask.elf" 0x01200513 0x0a600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-prctl-set-name.elf" 0x00f00513 0x0a700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setregid.elf" 0x3e800513 0x3e800593 0x08f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setgid.elf" 0x3e800513 0x09000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setreuid.elf" 0x3e800513 0x3e800593 0x09100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setuid.elf" 0x3e800513 0x09200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setresuid.elf" 0x3e800513 0x3e800593 0x3e800613 0x09300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getresuid.elf" 0x00058293 0x00028513 0x00428593 0x00828613 0x09400893 0x00000073 0x0002a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setresgid.elf" 0x3e800513 0x3e800593 0x3e800613 0x09500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getresgid.elf" 0x00058293 0x00028513 0x00428593 0x00828613 0x09600893 0x00000073 0x0002a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setfsuid.elf" 0x3e800513 0x09700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setfsgid.elf" 0x3e800513 0x09800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getgroups.elf" 0x00058293 0x00100513 0x00028593 0x09e00893 0x00000073 0x0002a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setgroups.elf" 0x00000513 0x00000593 0x09f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-kill.elf" 0x00000513 0x00000593 0x08100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-tkill.elf" 0x00000513 0x00000593 0x08200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-tgkill.elf" 0x00000513 0x00000593 0x00000613 0x08300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sigaltstack.elf" 0x00000513 0x08400893 0x00000073 0x0085a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rt-sigaction.elf" 0x00200513 0x00000593 0x00000613 0x00800693 0x08600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rt-sigprocmask.elf" 0x00000513 0x00000593 0x00000613 0x00800693 0x08700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-futex.elf" 0x00058513 0x00000593 0x00000613 0x00000693 0x06200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-madvise.elf" 0x00058513 0x000015b7 0x00000613 0x0e900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mremap.elf" 0x00058293 0x00028513 0x04000593 0x08000613 0x00000693 0x00000713 0x0d800893 0x00000073 0x40550533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clone.elf" 0x0dc00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-execve.elf" 0x0dd00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-membarrier-query.elf" 0x00000513 0x00000593 0x00000613 0x11b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-membarrier-cmd.elf" 0x00100513 0x00000593 0x00000613 0x11b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rseq.elf" 0x00058513 0x02000593 0x00000613 0x00000693 0x12500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlock.elf" 0x0e400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-munlock.elf" 0x0e500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlockall.elf" 0x0e600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-munlockall.elf" 0x0e700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-get-mempolicy.elf" 0x0ec00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-mempolicy.elf" 0x0ed00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-migrate-pages.elf" 0x0ee00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-move-pages.elf" 0x0ef00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-seccomp.elf" 0x11500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-bpf.elf" 0x11800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-userfaultfd.elf" 0x11a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlock2.elf" 0x11c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pkey-mprotect.elf" 0x12000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pkey-alloc.elf" 0x12100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pkey-free.elf" 0x12200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pidfd-send-signal.elf" 0x1a800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-io-uring-setup.elf" 0x1a900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-io-uring-enter.elf" 0x1aa00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-io-uring-register.elf" 0x1ab00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pidfd-open.elf" 0x1b200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clone3.elf" 0x1b300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-close-range.elf" 0x1b400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat2.elf" 0x1b500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pidfd-getfd.elf" 0x1b600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-madvise.elf" 0x1b800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-landlock-create-ruleset.elf" 0x1bc00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-landlock-add-rule.elf" 0x1bd00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-landlock-restrict-self.elf" 0x1be00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-mrelease.elf" 0x1c000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-futex-waitv.elf" 0x1c100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-mempolicy-home-node.elf" 0x1c200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-tid-address.elf" 0x00058513 0x06000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-robust-list.elf" 0x00058513 0x01800593 0x06300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-get-robust-list.elf" 0x00000513 0x00858613 0x06400893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-prlimit64.elf" 0x00058693 0x00000513 0x00300593 0x00000613 0x10500893 0x00000073 0x0006b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrandom.elf" 0x00058513 0x00400593 0x00000613 0x11600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap.elf" 0x00000513 0x0de00893 0x00000073 0x40c50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap6.elf" 0x00000513 0x01000593 0x00300613 0x02200693 0x00500713 0x00700793 0x0de00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap-store.elf" 0x00000513 0x0de00893 0x00000073 0x04d00593 0x00b53023 0x00053503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap-real-store.elf" 0x00000513 0x000015b7 0x00300613 0x02200693 0xfff00713 0x00000793 0x0de00893 0x00000073 0x04d00593 0x00b53023 0x00053503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sys-brk.elf" 0x00000513 0x0d600893 0x00000073 0x40b50533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-munmap.elf" 0x00058513 0x000015b7 0x0d700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mprotect.elf" 0x00058513 0x000015b7 0x00100613 0x0e200893 0x00000073
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-exit-group.elf" 0x00700513 0x05e00893 0x00000073 0x06400513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ecall.elf" 0x3ff00893 0x00000073
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
EXPECT_POLY_COMPAT_TRAPS="$EXPECT_POLY_COMPAT_TRAPS"

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
  EXPECT_POLY_CPUID="$EXPECT_POLY_CPUID" EXPECT_POLY_COMPAT_TRAPS="$EXPECT_POLY_COMPAT_TRAPS" /usr/bin/nativecheck.elf >/dev/ttyS0 2>&1
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
    /usr/lib/polyapps/aarch64-eventfd2.elf=7 \
    /usr/lib/polyapps/aarch64-inotify-init1.elf=14 \
    /usr/lib/polyapps/aarch64-inotify-add-watch.elf=31 \
    /usr/lib/polyapps/aarch64-inotify-rm-watch.elf=0 \
    /usr/lib/polyapps/aarch64-dup3.elf=8 \
    /usr/lib/polyapps/aarch64-setxattr.elf=0 \
    /usr/lib/polyapps/aarch64-lsetxattr.elf=0 \
    /usr/lib/polyapps/aarch64-fsetxattr.elf=0 \
    /usr/lib/polyapps/aarch64-getxattr.elf=4 \
    /usr/lib/polyapps/aarch64-lgetxattr.elf=4 \
    /usr/lib/polyapps/aarch64-fgetxattr.elf=4 \
    /usr/lib/polyapps/aarch64-listxattr.elf=10 \
    /usr/lib/polyapps/aarch64-llistxattr.elf=10 \
    /usr/lib/polyapps/aarch64-flistxattr.elf=10 \
    /usr/lib/polyapps/aarch64-removexattr.elf=0 \
    /usr/lib/polyapps/aarch64-lremovexattr.elf=0 \
    /usr/lib/polyapps/aarch64-fremovexattr.elf=0 \
    /usr/lib/polyapps/aarch64-ioprio-set.elf=0 \
    /usr/lib/polyapps/aarch64-ioprio-get.elf=0 \
    /usr/lib/polyapps/aarch64-flock.elf=0 \
    /usr/lib/polyapps/aarch64-mknodat.elf=0 \
    /usr/lib/polyapps/aarch64-mkdirat.elf=0 \
    /usr/lib/polyapps/aarch64-unlinkat.elf=0 \
    /usr/lib/polyapps/aarch64-symlinkat.elf=0 \
    /usr/lib/polyapps/aarch64-linkat.elf=0 \
    /usr/lib/polyapps/aarch64-renameat.elf=0 \
    /usr/lib/polyapps/aarch64-umount2.elf=0 \
    /usr/lib/polyapps/aarch64-mount.elf=0 \
    /usr/lib/polyapps/aarch64-pivot-root.elf=0 \
    /usr/lib/polyapps/aarch64-chroot.elf=0 \
    /usr/lib/polyapps/aarch64-renameat2.elf=0 \
    /usr/lib/polyapps/aarch64-open-tree.elf=15 \
    /usr/lib/polyapps/aarch64-move-mount.elf=0 \
    /usr/lib/polyapps/aarch64-fsopen.elf=16 \
    /usr/lib/polyapps/aarch64-fsconfig.elf=0 \
    /usr/lib/polyapps/aarch64-fsmount.elf=17 \
    /usr/lib/polyapps/aarch64-fspick.elf=18 \
    /usr/lib/polyapps/aarch64-mount-setattr.elf=0 \
    /usr/lib/polyapps/aarch64-pipe2.elf=9 \
    /usr/lib/polyapps/aarch64-fsync.elf=0 \
    /usr/lib/polyapps/aarch64-fdatasync.elf=0 \
    /usr/lib/polyapps/aarch64-sync-file-range.elf=0 \
    /usr/lib/polyapps/aarch64-fadvise64.elf=0 \
    /usr/lib/polyapps/aarch64-statfs.elf=0x21215441545350 \
    /usr/lib/polyapps/aarch64-fstatfs.elf=0x21215441545350 \
    /usr/lib/polyapps/aarch64-truncate.elf=0 \
    /usr/lib/polyapps/aarch64-ftruncate.elf=0 \
    /usr/lib/polyapps/aarch64-fallocate.elf=0 \
    /usr/lib/polyapps/aarch64-chdir.elf=0 \
    /usr/lib/polyapps/aarch64-fchdir.elf=0 \
    /usr/lib/polyapps/aarch64-fchmod.elf=0 \
    /usr/lib/polyapps/aarch64-fchmodat.elf=0 \
    /usr/lib/polyapps/aarch64-fchownat.elf=0 \
    /usr/lib/polyapps/aarch64-fchown.elf=0 \
    /usr/lib/polyapps/aarch64-timerfd-create.elf=13 \
    /usr/lib/polyapps/aarch64-timerfd-settime.elf=21 \
    /usr/lib/polyapps/aarch64-timerfd-gettime.elf=34 \
    /usr/lib/polyapps/aarch64-timer-create.elf=23 \
    /usr/lib/polyapps/aarch64-timer-gettime.elf=44 \
    /usr/lib/polyapps/aarch64-timer-getoverrun.elf=0 \
    /usr/lib/polyapps/aarch64-timer-settime.elf=55 \
    /usr/lib/polyapps/aarch64-timer-delete.elf=0 \
    /usr/lib/polyapps/aarch64-read.elf=4 \
    /usr/lib/polyapps/aarch64-readv.elf=4 \
    /usr/lib/polyapps/aarch64-write.elf=5 \
    /usr/lib/polyapps/aarch64-writev.elf=5 \
    /usr/lib/polyapps/aarch64-pread64.elf=4 \
    /usr/lib/polyapps/aarch64-pwrite64.elf=5 \
    /usr/lib/polyapps/aarch64-preadv.elf=4 \
    /usr/lib/polyapps/aarch64-pwritev.elf=5 \
    /usr/lib/polyapps/aarch64-pselect6.elf=0 \
    /usr/lib/polyapps/aarch64-ppoll.elf=0 \
    /usr/lib/polyapps/aarch64-epoll-create1.elf=4 \
    /usr/lib/polyapps/aarch64-epoll-ctl.elf=0 \
    /usr/lib/polyapps/aarch64-epoll-pwait.elf=0 \
    /usr/lib/polyapps/aarch64-nanosleep.elf=0 \
    /usr/lib/polyapps/aarch64-getitimer.elf=0 \
    /usr/lib/polyapps/aarch64-setitimer.elf=0 \
    /usr/lib/polyapps/aarch64-clock-nanosleep.elf=0 \
    /usr/lib/polyapps/aarch64-sched-setparam.elf=0 \
    /usr/lib/polyapps/aarch64-sched-setscheduler.elf=0 \
    /usr/lib/polyapps/aarch64-sched-getscheduler.elf=0 \
    /usr/lib/polyapps/aarch64-sched-getparam.elf=0 \
    /usr/lib/polyapps/aarch64-sched-setaffinity.elf=0 \
    /usr/lib/polyapps/aarch64-sched-getaffinity.elf=1 \
    /usr/lib/polyapps/aarch64-sched-yield.elf=0 \
    /usr/lib/polyapps/aarch64-sched-get-priority-max.elf=0 \
    /usr/lib/polyapps/aarch64-sched-get-priority-min.elf=0 \
    /usr/lib/polyapps/aarch64-socket.elf=5 \
    /usr/lib/polyapps/aarch64-socketpair.elf=11 \
    /usr/lib/polyapps/aarch64-bind.elf=0 \
    /usr/lib/polyapps/aarch64-listen.elf=0 \
    /usr/lib/polyapps/aarch64-accept.elf=6 \
    /usr/lib/polyapps/aarch64-connect.elf=0 \
    /usr/lib/polyapps/aarch64-getsockname.elf=0 \
    /usr/lib/polyapps/aarch64-getpeername.elf=0 \
    /usr/lib/polyapps/aarch64-sendto.elf=5 \
    /usr/lib/polyapps/aarch64-recvfrom.elf=4 \
    /usr/lib/polyapps/aarch64-setsockopt.elf=0 \
    /usr/lib/polyapps/aarch64-getsockopt.elf=0 \
    /usr/lib/polyapps/aarch64-shutdown.elf=0 \
    /usr/lib/polyapps/aarch64-accept4.elf=6 \
    /usr/lib/polyapps/aarch64-fcntl.elf=0 \
    /usr/lib/polyapps/aarch64-ioctl.elf=0 \
    /usr/lib/polyapps/aarch64-openat.elf=3 \
    /usr/lib/polyapps/aarch64-faccessat.elf=0 \
    /usr/lib/polyapps/aarch64-readlinkat.elf=5 \
    /usr/lib/polyapps/aarch64-newfstatat.elf=0x21215441545350 \
    /usr/lib/polyapps/aarch64-fstat.elf=0x21215441545350 \
    /usr/lib/polyapps/aarch64-statx.elf=0x21215441545350 \
    /usr/lib/polyapps/aarch64-getdents64.elf=24 \
    /usr/lib/polyapps/aarch64-openat-lseek.elf=7 \
    /usr/lib/polyapps/aarch64-openat-read.elf=4 \
    /usr/lib/polyapps/aarch64-openat-read-close.elf=0 \
    /usr/lib/polyapps/aarch64-clock-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-clock-getres.elf=1 \
    /usr/lib/polyapps/aarch64-times.elf=11 \
    /usr/lib/polyapps/aarch64-getpgid.elf=4242 \
    /usr/lib/polyapps/aarch64-getsid.elf=4242 \
    /usr/lib/polyapps/aarch64-getrlimit.elf=8388608 \
    /usr/lib/polyapps/aarch64-setrlimit.elf=0 \
    /usr/lib/polyapps/aarch64-getrusage.elf=321 \
    /usr/lib/polyapps/aarch64-getcpu.elf=12 \
    /usr/lib/polyapps/aarch64-gettimeofday.elf=246 \
    /usr/lib/polyapps/aarch64-sysinfo.elf=98765 \
    /usr/lib/polyapps/aarch64-capget.elf=0 \
    /usr/lib/polyapps/aarch64-capset.elf=0 \
    /usr/lib/polyapps/aarch64-personality.elf=0 \
    /usr/lib/polyapps/aarch64-waitid.elf=0xfffffffffffffff6 \
    /usr/lib/polyapps/aarch64-wait4.elf=0xfffffffffffffff6 \
    /usr/lib/polyapps/aarch64-setpriority.elf=0 \
    /usr/lib/polyapps/aarch64-getpriority.elf=20 \
    /usr/lib/polyapps/aarch64-setpgid.elf=0 \
    /usr/lib/polyapps/aarch64-setsid.elf=4242 \
    /usr/lib/polyapps/aarch64-umask.elf=18 \
    /usr/lib/polyapps/aarch64-prctl-set-name.elf=0 \
    /usr/lib/polyapps/aarch64-setregid.elf=0 \
    /usr/lib/polyapps/aarch64-setgid.elf=0 \
    /usr/lib/polyapps/aarch64-setreuid.elf=0 \
    /usr/lib/polyapps/aarch64-setuid.elf=0 \
    /usr/lib/polyapps/aarch64-setresuid.elf=0 \
    /usr/lib/polyapps/aarch64-getresuid.elf=1000 \
    /usr/lib/polyapps/aarch64-setresgid.elf=0 \
    /usr/lib/polyapps/aarch64-getresgid.elf=1000 \
    /usr/lib/polyapps/aarch64-setfsuid.elf=1000 \
    /usr/lib/polyapps/aarch64-setfsgid.elf=1000 \
    /usr/lib/polyapps/aarch64-getgroups.elf=1000 \
    /usr/lib/polyapps/aarch64-setgroups.elf=0 \
    /usr/lib/polyapps/aarch64-kill.elf=0 \
    /usr/lib/polyapps/aarch64-tkill.elf=0 \
    /usr/lib/polyapps/aarch64-tgkill.elf=0 \
    /usr/lib/polyapps/aarch64-sigaltstack.elf=2 \
    /usr/lib/polyapps/aarch64-rt-sigaction.elf=0 \
    /usr/lib/polyapps/aarch64-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/aarch64-futex.elf=0 \
    /usr/lib/polyapps/aarch64-madvise.elf=0 \
    /usr/lib/polyapps/aarch64-mremap.elf=0 \
    /usr/lib/polyapps/aarch64-clone.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-execve.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-membarrier-query.elf=1 \
    /usr/lib/polyapps/aarch64-membarrier-cmd.elf=0 \
    /usr/lib/polyapps/aarch64-rseq.elf=0 \
    /usr/lib/polyapps/aarch64-mlock.elf=0 \
    /usr/lib/polyapps/aarch64-munlock.elf=0 \
    /usr/lib/polyapps/aarch64-mlockall.elf=0 \
    /usr/lib/polyapps/aarch64-munlockall.elf=0 \
    /usr/lib/polyapps/aarch64-get-mempolicy.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-set-mempolicy.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-migrate-pages.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-move-pages.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-seccomp.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-bpf.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-userfaultfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-mlock2.elf=0 \
    /usr/lib/polyapps/aarch64-pkey-mprotect.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pkey-alloc.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pkey-free.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pidfd-send-signal.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-io-uring-setup.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-io-uring-enter.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-io-uring-register.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pidfd-open.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-clone3.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-close-range.elf=0 \
    /usr/lib/polyapps/aarch64-openat2.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pidfd-getfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-process-madvise.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-landlock-create-ruleset.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-landlock-add-rule.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-landlock-restrict-self.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-process-mrelease.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-futex-waitv.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-set-mempolicy-home-node.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-set-tid-address.elf=4243 \
    /usr/lib/polyapps/aarch64-set-robust-list.elf=0 \
    /usr/lib/polyapps/aarch64-get-robust-list.elf=24 \
    /usr/lib/polyapps/aarch64-prlimit64.elf=8388608 \
    /usr/lib/polyapps/aarch64-getrandom.elf=4 \
    /usr/lib/polyapps/aarch64-mmap.elf=0 \
    /usr/lib/polyapps/aarch64-mmap6.elf=65 \
    /usr/lib/polyapps/aarch64-mmap-store.elf=77 \
    /usr/lib/polyapps/aarch64-sys-brk.elf=0 \
    /usr/lib/polyapps/aarch64-munmap.elf=0 \
    /usr/lib/polyapps/aarch64-mprotect.elf=0 \
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
    /usr/lib/polyapps/aarch64-exit-group.elf=7 \
    /usr/lib/polyapps/aarch64-brk.elf=0x4c000305 \
    /usr/lib/polyapps/aarch64-svc.elf=0x53000703 \
    /usr/lib/polyapps/aarch64-long.elf=80 \
    /usr/lib/polyapps/riscv-add.elf=27 \
    /usr/lib/polyapps/riscv-compressed.elf=27 \
    /usr/lib/polyapps/riscv-compressed-half.elf=27 \
    /usr/lib/polyapps/riscv-compressed-jalr.elf=27 \
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
    /usr/lib/polyapps/riscv-eventfd2.elf=7 \
    /usr/lib/polyapps/riscv-inotify-init1.elf=14 \
    /usr/lib/polyapps/riscv-inotify-add-watch.elf=31 \
    /usr/lib/polyapps/riscv-inotify-rm-watch.elf=0 \
    /usr/lib/polyapps/riscv-dup3.elf=8 \
    /usr/lib/polyapps/riscv-setxattr.elf=0 \
    /usr/lib/polyapps/riscv-lsetxattr.elf=0 \
    /usr/lib/polyapps/riscv-fsetxattr.elf=0 \
    /usr/lib/polyapps/riscv-getxattr.elf=4 \
    /usr/lib/polyapps/riscv-lgetxattr.elf=4 \
    /usr/lib/polyapps/riscv-fgetxattr.elf=4 \
    /usr/lib/polyapps/riscv-listxattr.elf=10 \
    /usr/lib/polyapps/riscv-llistxattr.elf=10 \
    /usr/lib/polyapps/riscv-flistxattr.elf=10 \
    /usr/lib/polyapps/riscv-removexattr.elf=0 \
    /usr/lib/polyapps/riscv-lremovexattr.elf=0 \
    /usr/lib/polyapps/riscv-fremovexattr.elf=0 \
    /usr/lib/polyapps/riscv-ioprio-set.elf=0 \
    /usr/lib/polyapps/riscv-ioprio-get.elf=0 \
    /usr/lib/polyapps/riscv-flock.elf=0 \
    /usr/lib/polyapps/riscv-mknodat.elf=0 \
    /usr/lib/polyapps/riscv-mkdirat.elf=0 \
    /usr/lib/polyapps/riscv-unlinkat.elf=0 \
    /usr/lib/polyapps/riscv-symlinkat.elf=0 \
    /usr/lib/polyapps/riscv-linkat.elf=0 \
    /usr/lib/polyapps/riscv-renameat.elf=0 \
    /usr/lib/polyapps/riscv-umount2.elf=0 \
    /usr/lib/polyapps/riscv-mount.elf=0 \
    /usr/lib/polyapps/riscv-pivot-root.elf=0 \
    /usr/lib/polyapps/riscv-chroot.elf=0 \
    /usr/lib/polyapps/riscv-renameat2.elf=0 \
    /usr/lib/polyapps/riscv-open-tree.elf=15 \
    /usr/lib/polyapps/riscv-move-mount.elf=0 \
    /usr/lib/polyapps/riscv-fsopen.elf=16 \
    /usr/lib/polyapps/riscv-fsconfig.elf=0 \
    /usr/lib/polyapps/riscv-fsmount.elf=17 \
    /usr/lib/polyapps/riscv-fspick.elf=18 \
    /usr/lib/polyapps/riscv-mount-setattr.elf=0 \
    /usr/lib/polyapps/riscv-pipe2.elf=9 \
    /usr/lib/polyapps/riscv-fsync.elf=0 \
    /usr/lib/polyapps/riscv-fdatasync.elf=0 \
    /usr/lib/polyapps/riscv-sync-file-range.elf=0 \
    /usr/lib/polyapps/riscv-fadvise64.elf=0 \
    /usr/lib/polyapps/riscv-statfs.elf=0x21215441545350 \
    /usr/lib/polyapps/riscv-fstatfs.elf=0x21215441545350 \
    /usr/lib/polyapps/riscv-truncate.elf=0 \
    /usr/lib/polyapps/riscv-ftruncate.elf=0 \
    /usr/lib/polyapps/riscv-fallocate.elf=0 \
    /usr/lib/polyapps/riscv-chdir.elf=0 \
    /usr/lib/polyapps/riscv-fchdir.elf=0 \
    /usr/lib/polyapps/riscv-fchmod.elf=0 \
    /usr/lib/polyapps/riscv-fchmodat.elf=0 \
    /usr/lib/polyapps/riscv-fchownat.elf=0 \
    /usr/lib/polyapps/riscv-fchown.elf=0 \
    /usr/lib/polyapps/riscv-timerfd-create.elf=13 \
    /usr/lib/polyapps/riscv-timerfd-settime.elf=21 \
    /usr/lib/polyapps/riscv-timerfd-gettime.elf=34 \
    /usr/lib/polyapps/riscv-timer-create.elf=23 \
    /usr/lib/polyapps/riscv-timer-gettime.elf=44 \
    /usr/lib/polyapps/riscv-timer-getoverrun.elf=0 \
    /usr/lib/polyapps/riscv-timer-settime.elf=55 \
    /usr/lib/polyapps/riscv-timer-delete.elf=0 \
    /usr/lib/polyapps/riscv-read.elf=4 \
    /usr/lib/polyapps/riscv-readv.elf=4 \
    /usr/lib/polyapps/riscv-write.elf=5 \
    /usr/lib/polyapps/riscv-writev.elf=5 \
    /usr/lib/polyapps/riscv-pread64.elf=4 \
    /usr/lib/polyapps/riscv-pwrite64.elf=5 \
    /usr/lib/polyapps/riscv-preadv.elf=4 \
    /usr/lib/polyapps/riscv-pwritev.elf=5 \
    /usr/lib/polyapps/riscv-pselect6.elf=0 \
    /usr/lib/polyapps/riscv-ppoll.elf=0 \
    /usr/lib/polyapps/riscv-epoll-create1.elf=4 \
    /usr/lib/polyapps/riscv-epoll-ctl.elf=0 \
    /usr/lib/polyapps/riscv-epoll-pwait.elf=0 \
    /usr/lib/polyapps/riscv-nanosleep.elf=0 \
    /usr/lib/polyapps/riscv-getitimer.elf=0 \
    /usr/lib/polyapps/riscv-setitimer.elf=0 \
    /usr/lib/polyapps/riscv-clock-nanosleep.elf=0 \
    /usr/lib/polyapps/riscv-sched-setparam.elf=0 \
    /usr/lib/polyapps/riscv-sched-setscheduler.elf=0 \
    /usr/lib/polyapps/riscv-sched-getscheduler.elf=0 \
    /usr/lib/polyapps/riscv-sched-getparam.elf=0 \
    /usr/lib/polyapps/riscv-sched-setaffinity.elf=0 \
    /usr/lib/polyapps/riscv-sched-getaffinity.elf=1 \
    /usr/lib/polyapps/riscv-sched-yield.elf=0 \
    /usr/lib/polyapps/riscv-sched-get-priority-max.elf=0 \
    /usr/lib/polyapps/riscv-sched-get-priority-min.elf=0 \
    /usr/lib/polyapps/riscv-socket.elf=5 \
    /usr/lib/polyapps/riscv-socketpair.elf=11 \
    /usr/lib/polyapps/riscv-bind.elf=0 \
    /usr/lib/polyapps/riscv-listen.elf=0 \
    /usr/lib/polyapps/riscv-accept.elf=6 \
    /usr/lib/polyapps/riscv-connect.elf=0 \
    /usr/lib/polyapps/riscv-getsockname.elf=0 \
    /usr/lib/polyapps/riscv-getpeername.elf=0 \
    /usr/lib/polyapps/riscv-sendto.elf=5 \
    /usr/lib/polyapps/riscv-recvfrom.elf=4 \
    /usr/lib/polyapps/riscv-setsockopt.elf=0 \
    /usr/lib/polyapps/riscv-getsockopt.elf=0 \
    /usr/lib/polyapps/riscv-shutdown.elf=0 \
    /usr/lib/polyapps/riscv-accept4.elf=6 \
    /usr/lib/polyapps/riscv-fcntl.elf=0 \
    /usr/lib/polyapps/riscv-ioctl.elf=0 \
    /usr/lib/polyapps/riscv-openat.elf=3 \
    /usr/lib/polyapps/riscv-faccessat.elf=0 \
    /usr/lib/polyapps/riscv-readlinkat.elf=5 \
    /usr/lib/polyapps/riscv-newfstatat.elf=0x21215441545350 \
    /usr/lib/polyapps/riscv-fstat.elf=0x21215441545350 \
    /usr/lib/polyapps/riscv-statx.elf=0x21215441545350 \
    /usr/lib/polyapps/riscv-getdents64.elf=24 \
    /usr/lib/polyapps/riscv-openat-lseek.elf=7 \
    /usr/lib/polyapps/riscv-openat-read.elf=4 \
    /usr/lib/polyapps/riscv-openat-read-close.elf=0 \
    /usr/lib/polyapps/riscv-clock-gettime.elf=0 \
    /usr/lib/polyapps/riscv-clock-getres.elf=1 \
    /usr/lib/polyapps/riscv-times.elf=11 \
    /usr/lib/polyapps/riscv-getpgid.elf=4242 \
    /usr/lib/polyapps/riscv-getsid.elf=4242 \
    /usr/lib/polyapps/riscv-getrlimit.elf=8388608 \
    /usr/lib/polyapps/riscv-setrlimit.elf=0 \
    /usr/lib/polyapps/riscv-getrusage.elf=321 \
    /usr/lib/polyapps/riscv-getcpu.elf=12 \
    /usr/lib/polyapps/riscv-gettimeofday.elf=246 \
    /usr/lib/polyapps/riscv-sysinfo.elf=98765 \
    /usr/lib/polyapps/riscv-capget.elf=0 \
    /usr/lib/polyapps/riscv-capset.elf=0 \
    /usr/lib/polyapps/riscv-personality.elf=0 \
    /usr/lib/polyapps/riscv-waitid.elf=0xfffffffffffffff6 \
    /usr/lib/polyapps/riscv-wait4.elf=0xfffffffffffffff6 \
    /usr/lib/polyapps/riscv-setpriority.elf=0 \
    /usr/lib/polyapps/riscv-getpriority.elf=20 \
    /usr/lib/polyapps/riscv-setpgid.elf=0 \
    /usr/lib/polyapps/riscv-setsid.elf=4242 \
    /usr/lib/polyapps/riscv-umask.elf=18 \
    /usr/lib/polyapps/riscv-prctl-set-name.elf=0 \
    /usr/lib/polyapps/riscv-setregid.elf=0 \
    /usr/lib/polyapps/riscv-setgid.elf=0 \
    /usr/lib/polyapps/riscv-setreuid.elf=0 \
    /usr/lib/polyapps/riscv-setuid.elf=0 \
    /usr/lib/polyapps/riscv-setresuid.elf=0 \
    /usr/lib/polyapps/riscv-getresuid.elf=1000 \
    /usr/lib/polyapps/riscv-setresgid.elf=0 \
    /usr/lib/polyapps/riscv-getresgid.elf=1000 \
    /usr/lib/polyapps/riscv-setfsuid.elf=1000 \
    /usr/lib/polyapps/riscv-setfsgid.elf=1000 \
    /usr/lib/polyapps/riscv-getgroups.elf=1000 \
    /usr/lib/polyapps/riscv-setgroups.elf=0 \
    /usr/lib/polyapps/riscv-kill.elf=0 \
    /usr/lib/polyapps/riscv-tkill.elf=0 \
    /usr/lib/polyapps/riscv-tgkill.elf=0 \
    /usr/lib/polyapps/riscv-sigaltstack.elf=2 \
    /usr/lib/polyapps/riscv-rt-sigaction.elf=0 \
    /usr/lib/polyapps/riscv-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/riscv-futex.elf=0 \
    /usr/lib/polyapps/riscv-madvise.elf=0 \
    /usr/lib/polyapps/riscv-mremap.elf=0 \
    /usr/lib/polyapps/riscv-clone.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-execve.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-membarrier-query.elf=1 \
    /usr/lib/polyapps/riscv-membarrier-cmd.elf=0 \
    /usr/lib/polyapps/riscv-rseq.elf=0 \
    /usr/lib/polyapps/riscv-mlock.elf=0 \
    /usr/lib/polyapps/riscv-munlock.elf=0 \
    /usr/lib/polyapps/riscv-mlockall.elf=0 \
    /usr/lib/polyapps/riscv-munlockall.elf=0 \
    /usr/lib/polyapps/riscv-get-mempolicy.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-set-mempolicy.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-migrate-pages.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-move-pages.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-seccomp.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-bpf.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-userfaultfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-mlock2.elf=0 \
    /usr/lib/polyapps/riscv-pkey-mprotect.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pkey-alloc.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pkey-free.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pidfd-send-signal.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-io-uring-setup.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-io-uring-enter.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-io-uring-register.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pidfd-open.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-clone3.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-close-range.elf=0 \
    /usr/lib/polyapps/riscv-openat2.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pidfd-getfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-process-madvise.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-landlock-create-ruleset.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-landlock-add-rule.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-landlock-restrict-self.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-process-mrelease.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-futex-waitv.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-set-mempolicy-home-node.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-set-tid-address.elf=4243 \
    /usr/lib/polyapps/riscv-set-robust-list.elf=0 \
    /usr/lib/polyapps/riscv-get-robust-list.elf=24 \
    /usr/lib/polyapps/riscv-prlimit64.elf=8388608 \
    /usr/lib/polyapps/riscv-getrandom.elf=4 \
    /usr/lib/polyapps/riscv-mmap.elf=0 \
    /usr/lib/polyapps/riscv-mmap6.elf=65 \
    /usr/lib/polyapps/riscv-mmap-store.elf=77 \
    /usr/lib/polyapps/riscv-sys-brk.elf=0 \
    /usr/lib/polyapps/riscv-munmap.elf=0 \
    /usr/lib/polyapps/riscv-mprotect.elf=0 \
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
    /usr/lib/polyapps/riscv-exit-group.elf=7 \
    /usr/lib/polyapps/riscv-ebreak.elf=0x4c000405 \
    /usr/lib/polyapps/riscv-ecall.elf=0x5303ff04 \
    /usr/lib/polyapps/riscv-long.elf=80 >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_ARCH_TRAP_EXEC" = "1" ]; then
    /usr/bin/polyexec \
    /usr/lib/polyapps/aarch64-getpid.elf=pid \
    /usr/lib/polyapps/riscv-getpid.elf=pid \
    /usr/lib/polyapps/aarch64-getppid.elf=ppid \
    /usr/lib/polyapps/riscv-getppid.elf=ppid \
    /usr/lib/polyapps/aarch64-getuid.elf=uid \
    /usr/lib/polyapps/riscv-getuid.elf=uid \
    /usr/lib/polyapps/aarch64-geteuid.elf=euid \
    /usr/lib/polyapps/riscv-geteuid.elf=euid \
    /usr/lib/polyapps/aarch64-getgid.elf=gid \
    /usr/lib/polyapps/riscv-getgid.elf=gid \
    /usr/lib/polyapps/aarch64-getegid.elf=egid \
    /usr/lib/polyapps/riscv-getegid.elf=egid \
    /usr/lib/polyapps/aarch64-gettid.elf=tid \
    /usr/lib/polyapps/riscv-gettid.elf=tid \
    /usr/lib/polyapps/aarch64-getcwd.elf=cwd \
    /usr/lib/polyapps/riscv-getcwd.elf=cwd \
    /usr/lib/polyapps/aarch64-uname.elf=0 \
    /usr/lib/polyapps/riscv-uname.elf=0 \
    /usr/lib/polyapps/aarch64-clock-gettime.elf=0 \
    /usr/lib/polyapps/riscv-clock-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-mmap-real-store.elf=77 \
    /usr/lib/polyapps/riscv-mmap-real-store.elf=77 \
    /usr/lib/polyapps/aarch64-strlen.elf=5 \
    /usr/lib/polyapps/riscv-strlen.elf=5 \
    /usr/lib/polyapps/aarch64-memfill.elf=4 \
    /usr/lib/polyapps/riscv-memfill.elf=4 \
    /usr/lib/polyapps/aarch64-memcmp.elf=1 \
    /usr/lib/polyapps/riscv-memcmp.elf=1 \
    /usr/lib/polyapps/aarch64-memcpy.elf=4 \
    /usr/lib/polyapps/riscv-memcpy.elf=4 >/dev/ttyS0 2>&1
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
    /usr/lib/polyapps/aarch64-pcall-libc-import-real.so#poly_entry=171221 \
    /usr/lib/polyapps/aarch64-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/aarch64-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/aarch64-pcall-stack-protector-real.so#poly_entry=49 \
    /usr/lib/polyapps/aarch64-pcall-errno-real.so#poly_entry=29 \
    /usr/lib/polyapps/aarch64-pcall-getauxval-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-getpagesize-real.so#poly_entry=4141 \
    /usr/lib/polyapps/aarch64-pcall-sysconf-real.so#poly_entry=4141 \
    /usr/lib/polyapps/aarch64-pcall-env-real.so#poly_entry=53 \
    /usr/lib/polyapps/aarch64-pcall-alloc-real.so#poly_entry=90 \
    /usr/lib/polyapps/aarch64-pcall-strdup-real.so#poly_entry=911 \
    /usr/lib/polyapps/aarch64-pcall-aligned-alloc-real.so#poly_entry=177 \
    /usr/lib/polyapps/aarch64-pcall-atexit-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-process-real.so#poly_entry=16771 \
    /usr/lib/polyapps/aarch64-pcall-needed-real.so#poly_entry=397 \
    depfini:/usr/lib/polyapps/aarch64-pcall-needed-real.so#poly_entry=103 \
    /usr/lib/polyapps/aarch64-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/aarch64-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/aarch64-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/aarch64-pcall-ctor-real.so#poly_entry=245 \
    fini:/usr/lib/polyapps/aarch64-pcall-fini-real.so#poly_entry=1145 \
    /usr/lib/polyapps/aarch64-pcall-tls-real.so#poly_entry=55 \
    /usr/lib/polyapps/aarch64-pcall-tls-ie-real.so#poly_entry=55 \
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
    /usr/lib/polyapps/aarch64-pcall-int128-helpers-real.so#poly_entry=6148914613049023859 \
    /usr/lib/polyapps/aarch64-pcall-int128-fp-helpers-real.so#poly_entry=123358025 \
    /usr/lib/polyapps/aarch64-pcall-int128-float-helpers-real.so#poly_entry=11359 \
    /usr/lib/polyapps/aarch64-pcall-bit-helpers-real.so#poly_entry=43 \
    /usr/lib/polyapps/aarch64-pcall-longdouble-helpers-real.so#poly_entry=37 \
    /usr/lib/polyapps/aarch64-pcall-longdouble-signed-helpers-real.so#poly_entry=37 \
    /usr/lib/polyapps/aarch64-pcall-longdouble-compare-helpers-real.so#poly_entry=112 \
    /usr/lib/polyapps/aarch64-pcall-longdouble-int32-helpers-real.so#poly_entry=49390 \
    /usr/lib/polyapps/aarch64-pcall-int-carry-real.so#poly_entry=0x8000000000000021 \
    /usr/lib/polyapps/aarch64-pcall-int-varshift-real.so#poly_entry=0xe5d48b633e422ba5 \
    /usr/lib/polyapps/aarch64-pcall-int-logic-real.so#poly_entry=0x21d9737d81792d5e \
    /usr/lib/polyapps/aarch64-pcall-int-bitops-real.so#poly_entry=0xe5caa38822572301 \
    /usr/lib/polyapps/aarch64-pcall-int-rotate-real.so#poly_entry=0xbc1e4a9e37a5682e \
    /usr/lib/polyapps/aarch64-pcall-int-ccmp-real.so#poly_entry=5 \
    /usr/lib/polyapps/aarch64-pcall-postindex-mem.so#poly_entry=68 \
    /usr/lib/polyapps/aarch64-pcall-atomic.so#poly_entry=8590005819 \
    /usr/lib/polyapps/aarch64-pcall-atomic-outline.so#poly_entry=8590005819 \
    /usr/lib/polyapps/aarch64-pcall-atomic-lse.so#poly_entry=8590005819 \
    /usr/lib/polyapps/aarch64-pcall-unscaled-mem-real.so#poly_entry=0xffffffffffffffc1 \
    /usr/lib/polyapps/aarch64-pcall-indexed-mem-real.so#poly_entry=41 \
    /usr/lib/polyapps/aarch64-pcall-callee-real.so#poly_entry=420 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
    fpair:/usr/lib/polyapps/aarch64-pcall-fpair-real.so#poly_entry=0x40268000400b0000 \
    fpair32:/usr/lib/polyapps/aarch64-pcall-fpair32-real.so#poly_entry=0x40d8000040700000 \
    fpairarg:/usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so#poly_entry=0x4026800000000000 \
    fpair32arg:/usr/lib/polyapps/aarch64-pcall-fpair32-arg-real.so#poly_entry=0x41340000 \
    mixedargs:/usr/lib/polyapps/aarch64-pcall-mixed-args-real.so#poly_entry=0x40a9320000000000 \
    hetero:/usr/lib/polyapps/aarch64-pcall-hetero-real.so#poly_entry=0x0008401d00000000 \
    heterorev:/usr/lib/polyapps/aarch64-pcall-hetero-rev-real.so#poly_entry=0x0008401d00000000 \
    heterof32:/usr/lib/polyapps/aarch64-pcall-hetero32-real.so#poly_entry=0x0000000840e80000 \
    heterof32rev:/usr/lib/polyapps/aarch64-pcall-hetero32-rev-real.so#poly_entry=0x0000000840e80000 \
    heterou32:/usr/lib/polyapps/aarch64-pcall-hetero-u32-real.so#poly_entry=0x0008401d00000000 \
    heterou32rev:/usr/lib/polyapps/aarch64-pcall-hetero-u32-rev-real.so#poly_entry=0x0008401d00000000 \
    heterou32f32:/usr/lib/polyapps/aarch64-pcall-hetero-u32-f32-real.so#poly_entry=0x40e8000000000008 \
    heterof32u32:/usr/lib/polyapps/aarch64-pcall-hetero-f32-u32-real.so#poly_entry=0x0000000840e80000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-import-real.so#poly_entry=0x402b800000000000 \
    fp64stack:/usr/lib/polyapps/aarch64-pcall-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fp64-import-real.so#poly_entry=0x4069e80000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fp64-sum8-import-real.so#poly_entry=0x406da80000000000 \
    mixedargs:/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-import-real.so#poly_entry=0x406aa80000000000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-x86-fp32-import-real.so#poly_entry=0x434f4000 \
    /usr/lib/polyapps/aarch64-pcall-x86-sum8-import-real.so#poly_entry=236 \
    /usr/lib/polyapps/aarch64-pcall-x86-sum8-post-import-real.so#poly_entry=245 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-import-real.so#poly_entry=0x415c0000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-fp64-callee-real.so#poly_entry=0x4040400000000000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-fp32-callee-real.so#poly_entry=0x42020000 \
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
    /usr/lib/polyapps/riscv-pcall-real-rv64gc.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-state.so#poly_entry=83 \
    /usr/lib/polyapps/riscv-pcall-import-real.so#poly_entry=145 \
    /usr/lib/polyapps/riscv-pcall-import-real-rv64gc.so#poly_entry=145 \
    /usr/lib/polyapps/riscv-pcall-libc-import-real.so#poly_entry=171221 \
    /usr/lib/polyapps/riscv-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/riscv-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/riscv-pcall-stack-protector-real.so#poly_entry=49 \
    /usr/lib/polyapps/riscv-pcall-errno-real.so#poly_entry=29 \
    /usr/lib/polyapps/riscv-pcall-getauxval-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-getpagesize-real.so#poly_entry=4141 \
    /usr/lib/polyapps/riscv-pcall-sysconf-real.so#poly_entry=4141 \
    /usr/lib/polyapps/riscv-pcall-env-real.so#poly_entry=53 \
    /usr/lib/polyapps/riscv-pcall-alloc-real.so#poly_entry=90 \
    /usr/lib/polyapps/riscv-pcall-strdup-real.so#poly_entry=911 \
    /usr/lib/polyapps/riscv-pcall-aligned-alloc-real.so#poly_entry=177 \
    /usr/lib/polyapps/riscv-pcall-atexit-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-process-real.so#poly_entry=16771 \
    /usr/lib/polyapps/riscv-pcall-needed-real.so#poly_entry=397 \
    depfini:/usr/lib/polyapps/riscv-pcall-needed-real.so#poly_entry=103 \
    /usr/lib/polyapps/riscv-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/riscv-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/riscv-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/riscv-pcall-ctor-real.so#poly_entry=245 \
    fini:/usr/lib/polyapps/riscv-pcall-fini-real.so#poly_entry=1145 \
    /usr/lib/polyapps/riscv-pcall-tls-real.so#poly_entry=55 \
    /usr/lib/polyapps/riscv-pcall-tls-ie-real.so#poly_entry=55 \
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
    /usr/lib/polyapps/riscv-pcall-int128-helpers-real.so#poly_entry=6148914613049023859 \
    /usr/lib/polyapps/riscv-pcall-int128-fp-helpers-real.so#poly_entry=123358025 \
    /usr/lib/polyapps/riscv-pcall-int128-float-helpers-real.so#poly_entry=11359 \
    /usr/lib/polyapps/riscv-pcall-bit-helpers-real.so#poly_entry=43 \
    /usr/lib/polyapps/riscv-pcall-longdouble-helpers-real.so#poly_entry=37 \
    /usr/lib/polyapps/riscv-pcall-longdouble-signed-helpers-real.so#poly_entry=37 \
    /usr/lib/polyapps/riscv-pcall-longdouble-compare-helpers-real.so#poly_entry=112 \
    /usr/lib/polyapps/riscv-pcall-longdouble-int32-helpers-real.so#poly_entry=49390 \
    /usr/lib/polyapps/riscv-pcall-int-carry-real.so#poly_entry=0x8000000000000021 \
    /usr/lib/polyapps/riscv-pcall-int-varshift-real.so#poly_entry=0xe5d48b633e422ba5 \
    /usr/lib/polyapps/riscv-pcall-int-logic-real.so#poly_entry=0x21d9737d81792d5e \
    /usr/lib/polyapps/riscv-pcall-int-bitops-real.so#poly_entry=0xe5caa38822572301 \
    /usr/lib/polyapps/riscv-pcall-int-rotate-real.so#poly_entry=0xbc1e4a9e37a5682e \
    /usr/lib/polyapps/riscv-pcall-int-ccmp-real.so#poly_entry=5 \
    /usr/lib/polyapps/riscv-pcall-atomic.so#poly_entry=8590005819 \
    /usr/lib/polyapps/riscv-pcall-unscaled-mem-real.so#poly_entry=0xffffffffffffffc1 \
    /usr/lib/polyapps/riscv-pcall-indexed-mem-real.so#poly_entry=41 \
    /usr/lib/polyapps/riscv-pcall-callee-real.so#poly_entry=420 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
    fpair:/usr/lib/polyapps/riscv-pcall-fpair-real.so#poly_entry=0x40268000400b0000 \
    fpair32:/usr/lib/polyapps/riscv-pcall-fpair32-real.so#poly_entry=0x40d8000040700000 \
    fpairarg:/usr/lib/polyapps/riscv-pcall-fpair-arg-real.so#poly_entry=0x4026800000000000 \
    fpair32arg:/usr/lib/polyapps/riscv-pcall-fpair32-arg-real.so#poly_entry=0x41340000 \
    mixedargs:/usr/lib/polyapps/riscv-pcall-mixed-args-real.so#poly_entry=0x40a9320000000000 \
    hetero:/usr/lib/polyapps/riscv-pcall-hetero-real.so#poly_entry=0x0008401d00000000 \
    heterorev:/usr/lib/polyapps/riscv-pcall-hetero-rev-real.so#poly_entry=0x0008401d00000000 \
    heterof32:/usr/lib/polyapps/riscv-pcall-hetero32-real.so#poly_entry=0x0000000840e80000 \
    heterof32rev:/usr/lib/polyapps/riscv-pcall-hetero32-rev-real.so#poly_entry=0x0000000840e80000 \
    heterou32:/usr/lib/polyapps/riscv-pcall-hetero-u32-real.so#poly_entry=0x0008401d00000000 \
    heterou32rev:/usr/lib/polyapps/riscv-pcall-hetero-u32-rev-real.so#poly_entry=0x0008401d00000000 \
    heterou32f32:/usr/lib/polyapps/riscv-pcall-hetero-u32-f32-real.so#poly_entry=0x40e8000000000008 \
    heterof32u32:/usr/lib/polyapps/riscv-pcall-hetero-f32-u32-real.so#poly_entry=0x0000000840e80000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-import-real.so#poly_entry=0x402b800000000000 \
    fp64stack:/usr/lib/polyapps/riscv-pcall-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fp64-import-real.so#poly_entry=0x4069e80000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fp64-sum8-import-real.so#poly_entry=0x406da80000000000 \
    mixedargs:/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-import-real.so#poly_entry=0x406aa80000000000 \
    fp32:/usr/lib/polyapps/riscv-pcall-x86-fp32-import-real.so#poly_entry=0x434f4000 \
    /usr/lib/polyapps/riscv-pcall-x86-sum8-import-real.so#poly_entry=236 \
    /usr/lib/polyapps/riscv-pcall-x86-sum8-post-import-real.so#poly_entry=245 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-import-real.so#poly_entry=0x415c0000 \
    fp64:/usr/lib/polyapps/riscv-pcall-fp64-callee-real.so#poly_entry=0x4040400000000000 \
    fp32:/usr/lib/polyapps/riscv-pcall-fp32-callee-real.so#poly_entry=0x42020000 \
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
    /usr/lib/polyapps/aarch64-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dyntab.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-dyntab-entry.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-import.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-import-func.elf=103 \
    /usr/lib/polyapps/aarch64-pcall-import-mul.elf=102 \
    /usr/lib/polyapps/aarch64-pcall-import-x86.elf=203 \
    /usr/lib/polyapps/aarch64-pcall-import-x86-mul.elf=202 \
    /usr/lib/polyapps/aarch64-pcall-import-x86-sum6.elf=221 \
    /usr/lib/polyapps/riscv-pcall-frame.elf=45 \
    /usr/lib/polyapps/riscv-pcall-split-load.elf=123 \
    /usr/lib/polyapps/riscv-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dyntab.elf=123 \
    /usr/lib/polyapps/riscv-pcall-dyntab-entry.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-pcall-import.elf=123 \
    /usr/lib/polyapps/riscv-pcall-import-func.elf=103 \
    /usr/lib/polyapps/riscv-pcall-import-cjalr.elf=103 \
    /usr/lib/polyapps/riscv-pcall-import-cjr.elf=103 \
    /usr/lib/polyapps/riscv-pcall-import-mul.elf=102 \
    /usr/lib/polyapps/riscv-pcall-import-x86.elf=203 \
    /usr/lib/polyapps/riscv-pcall-import-x86-mul.elf=202 \
    /usr/lib/polyapps/riscv-pcall-import-x86-sum6.elf=221 >/dev/ttyS0 2>&1
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
    /usr/lib/polyapps/aarch64-eventfd2.elf \
    /usr/lib/polyapps/aarch64-inotify-init1.elf \
    /usr/lib/polyapps/aarch64-inotify-add-watch.elf \
    /usr/lib/polyapps/aarch64-inotify-rm-watch.elf \
    /usr/lib/polyapps/aarch64-dup3.elf \
    /usr/lib/polyapps/aarch64-setxattr.elf \
    /usr/lib/polyapps/aarch64-lsetxattr.elf \
    /usr/lib/polyapps/aarch64-fsetxattr.elf \
    /usr/lib/polyapps/aarch64-getxattr.elf \
    /usr/lib/polyapps/aarch64-lgetxattr.elf \
    /usr/lib/polyapps/aarch64-fgetxattr.elf \
    /usr/lib/polyapps/aarch64-listxattr.elf \
    /usr/lib/polyapps/aarch64-llistxattr.elf \
    /usr/lib/polyapps/aarch64-flistxattr.elf \
    /usr/lib/polyapps/aarch64-removexattr.elf \
    /usr/lib/polyapps/aarch64-lremovexattr.elf \
    /usr/lib/polyapps/aarch64-fremovexattr.elf \
    /usr/lib/polyapps/aarch64-ioprio-set.elf \
    /usr/lib/polyapps/aarch64-ioprio-get.elf \
    /usr/lib/polyapps/aarch64-flock.elf \
    /usr/lib/polyapps/aarch64-mknodat.elf \
    /usr/lib/polyapps/aarch64-mkdirat.elf \
    /usr/lib/polyapps/aarch64-unlinkat.elf \
    /usr/lib/polyapps/aarch64-symlinkat.elf \
    /usr/lib/polyapps/aarch64-linkat.elf \
    /usr/lib/polyapps/aarch64-renameat.elf \
    /usr/lib/polyapps/aarch64-umount2.elf \
    /usr/lib/polyapps/aarch64-mount.elf \
    /usr/lib/polyapps/aarch64-pivot-root.elf \
    /usr/lib/polyapps/aarch64-chroot.elf \
    /usr/lib/polyapps/aarch64-renameat2.elf \
    /usr/lib/polyapps/aarch64-open-tree.elf \
    /usr/lib/polyapps/aarch64-move-mount.elf \
    /usr/lib/polyapps/aarch64-fsopen.elf \
    /usr/lib/polyapps/aarch64-fsconfig.elf \
    /usr/lib/polyapps/aarch64-fsmount.elf \
    /usr/lib/polyapps/aarch64-fspick.elf \
    /usr/lib/polyapps/aarch64-mount-setattr.elf \
    /usr/lib/polyapps/aarch64-pipe2.elf \
    /usr/lib/polyapps/aarch64-fsync.elf \
    /usr/lib/polyapps/aarch64-fdatasync.elf \
    /usr/lib/polyapps/aarch64-sync-file-range.elf \
    /usr/lib/polyapps/aarch64-fadvise64.elf \
    /usr/lib/polyapps/aarch64-statfs.elf \
    /usr/lib/polyapps/aarch64-fstatfs.elf \
    /usr/lib/polyapps/aarch64-truncate.elf \
    /usr/lib/polyapps/aarch64-ftruncate.elf \
    /usr/lib/polyapps/aarch64-fallocate.elf \
    /usr/lib/polyapps/aarch64-chdir.elf \
    /usr/lib/polyapps/aarch64-fchdir.elf \
    /usr/lib/polyapps/aarch64-fchmod.elf \
    /usr/lib/polyapps/aarch64-fchmodat.elf \
    /usr/lib/polyapps/aarch64-fchownat.elf \
    /usr/lib/polyapps/aarch64-fchown.elf \
    /usr/lib/polyapps/aarch64-timerfd-create.elf \
    /usr/lib/polyapps/aarch64-timerfd-settime.elf \
    /usr/lib/polyapps/aarch64-timerfd-gettime.elf \
    /usr/lib/polyapps/aarch64-timer-create.elf \
    /usr/lib/polyapps/aarch64-timer-gettime.elf \
    /usr/lib/polyapps/aarch64-timer-getoverrun.elf \
    /usr/lib/polyapps/aarch64-timer-settime.elf \
    /usr/lib/polyapps/aarch64-timer-delete.elf \
    /usr/lib/polyapps/aarch64-read.elf \
    /usr/lib/polyapps/aarch64-readv.elf \
    /usr/lib/polyapps/aarch64-write.elf \
    /usr/lib/polyapps/aarch64-writev.elf \
    /usr/lib/polyapps/aarch64-pread64.elf \
    /usr/lib/polyapps/aarch64-pwrite64.elf \
    /usr/lib/polyapps/aarch64-preadv.elf \
    /usr/lib/polyapps/aarch64-pwritev.elf \
    /usr/lib/polyapps/aarch64-pselect6.elf \
    /usr/lib/polyapps/aarch64-ppoll.elf \
    /usr/lib/polyapps/aarch64-epoll-create1.elf \
    /usr/lib/polyapps/aarch64-epoll-ctl.elf \
    /usr/lib/polyapps/aarch64-epoll-pwait.elf \
    /usr/lib/polyapps/aarch64-nanosleep.elf \
    /usr/lib/polyapps/aarch64-getitimer.elf \
    /usr/lib/polyapps/aarch64-setitimer.elf \
    /usr/lib/polyapps/aarch64-clock-nanosleep.elf \
    /usr/lib/polyapps/aarch64-sched-setparam.elf \
    /usr/lib/polyapps/aarch64-sched-setscheduler.elf \
    /usr/lib/polyapps/aarch64-sched-getscheduler.elf \
    /usr/lib/polyapps/aarch64-sched-getparam.elf \
    /usr/lib/polyapps/aarch64-sched-setaffinity.elf \
    /usr/lib/polyapps/aarch64-sched-getaffinity.elf \
    /usr/lib/polyapps/aarch64-sched-yield.elf \
    /usr/lib/polyapps/aarch64-sched-get-priority-max.elf \
    /usr/lib/polyapps/aarch64-sched-get-priority-min.elf \
    /usr/lib/polyapps/aarch64-socket.elf \
    /usr/lib/polyapps/aarch64-socketpair.elf \
    /usr/lib/polyapps/aarch64-bind.elf \
    /usr/lib/polyapps/aarch64-listen.elf \
    /usr/lib/polyapps/aarch64-accept.elf \
    /usr/lib/polyapps/aarch64-connect.elf \
    /usr/lib/polyapps/aarch64-getsockname.elf \
    /usr/lib/polyapps/aarch64-getpeername.elf \
    /usr/lib/polyapps/aarch64-sendto.elf \
    /usr/lib/polyapps/aarch64-recvfrom.elf \
    /usr/lib/polyapps/aarch64-setsockopt.elf \
    /usr/lib/polyapps/aarch64-getsockopt.elf \
    /usr/lib/polyapps/aarch64-shutdown.elf \
    /usr/lib/polyapps/aarch64-accept4.elf \
    /usr/lib/polyapps/aarch64-fcntl.elf \
    /usr/lib/polyapps/aarch64-ioctl.elf \
    /usr/lib/polyapps/aarch64-openat.elf \
    /usr/lib/polyapps/aarch64-faccessat.elf \
    /usr/lib/polyapps/aarch64-readlinkat.elf \
    /usr/lib/polyapps/aarch64-newfstatat.elf \
    /usr/lib/polyapps/aarch64-fstat.elf \
    /usr/lib/polyapps/aarch64-statx.elf \
    /usr/lib/polyapps/aarch64-getdents64.elf \
    /usr/lib/polyapps/aarch64-openat-lseek.elf \
    /usr/lib/polyapps/aarch64-openat-read.elf \
    /usr/lib/polyapps/aarch64-openat-read-close.elf \
    /usr/lib/polyapps/aarch64-clock-gettime.elf \
    /usr/lib/polyapps/aarch64-clock-getres.elf \
    /usr/lib/polyapps/aarch64-times.elf \
    /usr/lib/polyapps/aarch64-getpgid.elf \
    /usr/lib/polyapps/aarch64-getsid.elf \
    /usr/lib/polyapps/aarch64-getrlimit.elf \
    /usr/lib/polyapps/aarch64-setrlimit.elf \
    /usr/lib/polyapps/aarch64-getrusage.elf \
    /usr/lib/polyapps/aarch64-getcpu.elf \
    /usr/lib/polyapps/aarch64-gettimeofday.elf \
    /usr/lib/polyapps/aarch64-sysinfo.elf \
    /usr/lib/polyapps/aarch64-capget.elf \
    /usr/lib/polyapps/aarch64-capset.elf \
    /usr/lib/polyapps/aarch64-personality.elf \
    /usr/lib/polyapps/aarch64-waitid.elf \
    /usr/lib/polyapps/aarch64-wait4.elf \
    /usr/lib/polyapps/aarch64-setpriority.elf \
    /usr/lib/polyapps/aarch64-getpriority.elf \
    /usr/lib/polyapps/aarch64-setpgid.elf \
    /usr/lib/polyapps/aarch64-setsid.elf \
    /usr/lib/polyapps/aarch64-umask.elf \
    /usr/lib/polyapps/aarch64-prctl-set-name.elf \
    /usr/lib/polyapps/aarch64-setregid.elf \
    /usr/lib/polyapps/aarch64-setgid.elf \
    /usr/lib/polyapps/aarch64-setreuid.elf \
    /usr/lib/polyapps/aarch64-setuid.elf \
    /usr/lib/polyapps/aarch64-setresuid.elf \
    /usr/lib/polyapps/aarch64-getresuid.elf \
    /usr/lib/polyapps/aarch64-setresgid.elf \
    /usr/lib/polyapps/aarch64-getresgid.elf \
    /usr/lib/polyapps/aarch64-setfsuid.elf \
    /usr/lib/polyapps/aarch64-setfsgid.elf \
    /usr/lib/polyapps/aarch64-getgroups.elf \
    /usr/lib/polyapps/aarch64-setgroups.elf \
    /usr/lib/polyapps/aarch64-kill.elf \
    /usr/lib/polyapps/aarch64-tkill.elf \
    /usr/lib/polyapps/aarch64-tgkill.elf \
    /usr/lib/polyapps/aarch64-sigaltstack.elf \
    /usr/lib/polyapps/aarch64-rt-sigaction.elf \
    /usr/lib/polyapps/aarch64-rt-sigprocmask.elf \
    /usr/lib/polyapps/aarch64-futex.elf \
    /usr/lib/polyapps/aarch64-madvise.elf \
    /usr/lib/polyapps/aarch64-mremap.elf \
    /usr/lib/polyapps/aarch64-clone.elf \
    /usr/lib/polyapps/aarch64-execve.elf \
    /usr/lib/polyapps/aarch64-membarrier-query.elf \
    /usr/lib/polyapps/aarch64-membarrier-cmd.elf \
    /usr/lib/polyapps/aarch64-rseq.elf \
    /usr/lib/polyapps/aarch64-mlock.elf \
    /usr/lib/polyapps/aarch64-munlock.elf \
    /usr/lib/polyapps/aarch64-mlockall.elf \
    /usr/lib/polyapps/aarch64-munlockall.elf \
    /usr/lib/polyapps/aarch64-get-mempolicy.elf \
    /usr/lib/polyapps/aarch64-set-mempolicy.elf \
    /usr/lib/polyapps/aarch64-migrate-pages.elf \
    /usr/lib/polyapps/aarch64-move-pages.elf \
    /usr/lib/polyapps/aarch64-seccomp.elf \
    /usr/lib/polyapps/aarch64-bpf.elf \
    /usr/lib/polyapps/aarch64-userfaultfd.elf \
    /usr/lib/polyapps/aarch64-mlock2.elf \
    /usr/lib/polyapps/aarch64-pkey-mprotect.elf \
    /usr/lib/polyapps/aarch64-pkey-alloc.elf \
    /usr/lib/polyapps/aarch64-pkey-free.elf \
    /usr/lib/polyapps/aarch64-pidfd-send-signal.elf \
    /usr/lib/polyapps/aarch64-io-uring-setup.elf \
    /usr/lib/polyapps/aarch64-io-uring-enter.elf \
    /usr/lib/polyapps/aarch64-io-uring-register.elf \
    /usr/lib/polyapps/aarch64-pidfd-open.elf \
    /usr/lib/polyapps/aarch64-clone3.elf \
    /usr/lib/polyapps/aarch64-close-range.elf \
    /usr/lib/polyapps/aarch64-openat2.elf \
    /usr/lib/polyapps/aarch64-pidfd-getfd.elf \
    /usr/lib/polyapps/aarch64-process-madvise.elf \
    /usr/lib/polyapps/aarch64-landlock-create-ruleset.elf \
    /usr/lib/polyapps/aarch64-landlock-add-rule.elf \
    /usr/lib/polyapps/aarch64-landlock-restrict-self.elf \
    /usr/lib/polyapps/aarch64-process-mrelease.elf \
    /usr/lib/polyapps/aarch64-futex-waitv.elf \
    /usr/lib/polyapps/aarch64-set-mempolicy-home-node.elf \
    /usr/lib/polyapps/aarch64-set-tid-address.elf \
    /usr/lib/polyapps/aarch64-set-robust-list.elf \
    /usr/lib/polyapps/aarch64-get-robust-list.elf \
    /usr/lib/polyapps/aarch64-prlimit64.elf \
    /usr/lib/polyapps/aarch64-getrandom.elf \
    /usr/lib/polyapps/aarch64-mmap.elf \
    /usr/lib/polyapps/aarch64-mmap6.elf \
    /usr/lib/polyapps/aarch64-mmap-store.elf \
    /usr/lib/polyapps/aarch64-sys-brk.elf \
    /usr/lib/polyapps/aarch64-munmap.elf \
    /usr/lib/polyapps/aarch64-mprotect.elf \
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
    /usr/lib/polyapps/aarch64-exit-group.elf \
    /usr/lib/polyapps/aarch64-brk.elf \
    /usr/lib/polyapps/aarch64-svc.elf \
    /usr/lib/polyapps/aarch64-long.elf \
    /usr/lib/polyapps/riscv-add.elf \
    /usr/lib/polyapps/riscv-compressed.elf \
    /usr/lib/polyapps/riscv-compressed-half.elf \
    /usr/lib/polyapps/riscv-compressed-jalr.elf \
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
    /usr/lib/polyapps/riscv-eventfd2.elf \
    /usr/lib/polyapps/riscv-inotify-init1.elf \
    /usr/lib/polyapps/riscv-inotify-add-watch.elf \
    /usr/lib/polyapps/riscv-inotify-rm-watch.elf \
    /usr/lib/polyapps/riscv-dup3.elf \
    /usr/lib/polyapps/riscv-setxattr.elf \
    /usr/lib/polyapps/riscv-lsetxattr.elf \
    /usr/lib/polyapps/riscv-fsetxattr.elf \
    /usr/lib/polyapps/riscv-getxattr.elf \
    /usr/lib/polyapps/riscv-lgetxattr.elf \
    /usr/lib/polyapps/riscv-fgetxattr.elf \
    /usr/lib/polyapps/riscv-listxattr.elf \
    /usr/lib/polyapps/riscv-llistxattr.elf \
    /usr/lib/polyapps/riscv-flistxattr.elf \
    /usr/lib/polyapps/riscv-removexattr.elf \
    /usr/lib/polyapps/riscv-lremovexattr.elf \
    /usr/lib/polyapps/riscv-fremovexattr.elf \
    /usr/lib/polyapps/riscv-ioprio-set.elf \
    /usr/lib/polyapps/riscv-ioprio-get.elf \
    /usr/lib/polyapps/riscv-flock.elf \
    /usr/lib/polyapps/riscv-mknodat.elf \
    /usr/lib/polyapps/riscv-mkdirat.elf \
    /usr/lib/polyapps/riscv-unlinkat.elf \
    /usr/lib/polyapps/riscv-symlinkat.elf \
    /usr/lib/polyapps/riscv-linkat.elf \
    /usr/lib/polyapps/riscv-renameat.elf \
    /usr/lib/polyapps/riscv-umount2.elf \
    /usr/lib/polyapps/riscv-mount.elf \
    /usr/lib/polyapps/riscv-pivot-root.elf \
    /usr/lib/polyapps/riscv-chroot.elf \
    /usr/lib/polyapps/riscv-renameat2.elf \
    /usr/lib/polyapps/riscv-open-tree.elf \
    /usr/lib/polyapps/riscv-move-mount.elf \
    /usr/lib/polyapps/riscv-fsopen.elf \
    /usr/lib/polyapps/riscv-fsconfig.elf \
    /usr/lib/polyapps/riscv-fsmount.elf \
    /usr/lib/polyapps/riscv-fspick.elf \
    /usr/lib/polyapps/riscv-mount-setattr.elf \
    /usr/lib/polyapps/riscv-pipe2.elf \
    /usr/lib/polyapps/riscv-fsync.elf \
    /usr/lib/polyapps/riscv-fdatasync.elf \
    /usr/lib/polyapps/riscv-sync-file-range.elf \
    /usr/lib/polyapps/riscv-fadvise64.elf \
    /usr/lib/polyapps/riscv-statfs.elf \
    /usr/lib/polyapps/riscv-fstatfs.elf \
    /usr/lib/polyapps/riscv-truncate.elf \
    /usr/lib/polyapps/riscv-ftruncate.elf \
    /usr/lib/polyapps/riscv-fallocate.elf \
    /usr/lib/polyapps/riscv-chdir.elf \
    /usr/lib/polyapps/riscv-fchdir.elf \
    /usr/lib/polyapps/riscv-fchmod.elf \
    /usr/lib/polyapps/riscv-fchmodat.elf \
    /usr/lib/polyapps/riscv-fchownat.elf \
    /usr/lib/polyapps/riscv-fchown.elf \
    /usr/lib/polyapps/riscv-timerfd-create.elf \
    /usr/lib/polyapps/riscv-timerfd-settime.elf \
    /usr/lib/polyapps/riscv-timerfd-gettime.elf \
    /usr/lib/polyapps/riscv-timer-create.elf \
    /usr/lib/polyapps/riscv-timer-gettime.elf \
    /usr/lib/polyapps/riscv-timer-getoverrun.elf \
    /usr/lib/polyapps/riscv-timer-settime.elf \
    /usr/lib/polyapps/riscv-timer-delete.elf \
    /usr/lib/polyapps/riscv-read.elf \
    /usr/lib/polyapps/riscv-readv.elf \
    /usr/lib/polyapps/riscv-write.elf \
    /usr/lib/polyapps/riscv-writev.elf \
    /usr/lib/polyapps/riscv-pread64.elf \
    /usr/lib/polyapps/riscv-pwrite64.elf \
    /usr/lib/polyapps/riscv-preadv.elf \
    /usr/lib/polyapps/riscv-pwritev.elf \
    /usr/lib/polyapps/riscv-pselect6.elf \
    /usr/lib/polyapps/riscv-ppoll.elf \
    /usr/lib/polyapps/riscv-epoll-create1.elf \
    /usr/lib/polyapps/riscv-epoll-ctl.elf \
    /usr/lib/polyapps/riscv-epoll-pwait.elf \
    /usr/lib/polyapps/riscv-nanosleep.elf \
    /usr/lib/polyapps/riscv-getitimer.elf \
    /usr/lib/polyapps/riscv-setitimer.elf \
    /usr/lib/polyapps/riscv-clock-nanosleep.elf \
    /usr/lib/polyapps/riscv-sched-setparam.elf \
    /usr/lib/polyapps/riscv-sched-setscheduler.elf \
    /usr/lib/polyapps/riscv-sched-getscheduler.elf \
    /usr/lib/polyapps/riscv-sched-getparam.elf \
    /usr/lib/polyapps/riscv-sched-setaffinity.elf \
    /usr/lib/polyapps/riscv-sched-getaffinity.elf \
    /usr/lib/polyapps/riscv-sched-yield.elf \
    /usr/lib/polyapps/riscv-sched-get-priority-max.elf \
    /usr/lib/polyapps/riscv-sched-get-priority-min.elf \
    /usr/lib/polyapps/riscv-socket.elf \
    /usr/lib/polyapps/riscv-socketpair.elf \
    /usr/lib/polyapps/riscv-bind.elf \
    /usr/lib/polyapps/riscv-listen.elf \
    /usr/lib/polyapps/riscv-accept.elf \
    /usr/lib/polyapps/riscv-connect.elf \
    /usr/lib/polyapps/riscv-getsockname.elf \
    /usr/lib/polyapps/riscv-getpeername.elf \
    /usr/lib/polyapps/riscv-sendto.elf \
    /usr/lib/polyapps/riscv-recvfrom.elf \
    /usr/lib/polyapps/riscv-setsockopt.elf \
    /usr/lib/polyapps/riscv-getsockopt.elf \
    /usr/lib/polyapps/riscv-shutdown.elf \
    /usr/lib/polyapps/riscv-accept4.elf \
    /usr/lib/polyapps/riscv-fcntl.elf \
    /usr/lib/polyapps/riscv-ioctl.elf \
    /usr/lib/polyapps/riscv-openat.elf \
    /usr/lib/polyapps/riscv-faccessat.elf \
    /usr/lib/polyapps/riscv-readlinkat.elf \
    /usr/lib/polyapps/riscv-newfstatat.elf \
    /usr/lib/polyapps/riscv-fstat.elf \
    /usr/lib/polyapps/riscv-statx.elf \
    /usr/lib/polyapps/riscv-getdents64.elf \
    /usr/lib/polyapps/riscv-openat-lseek.elf \
    /usr/lib/polyapps/riscv-openat-read.elf \
    /usr/lib/polyapps/riscv-openat-read-close.elf \
    /usr/lib/polyapps/riscv-clock-gettime.elf \
    /usr/lib/polyapps/riscv-clock-getres.elf \
    /usr/lib/polyapps/riscv-times.elf \
    /usr/lib/polyapps/riscv-getpgid.elf \
    /usr/lib/polyapps/riscv-getsid.elf \
    /usr/lib/polyapps/riscv-getrlimit.elf \
    /usr/lib/polyapps/riscv-setrlimit.elf \
    /usr/lib/polyapps/riscv-getrusage.elf \
    /usr/lib/polyapps/riscv-getcpu.elf \
    /usr/lib/polyapps/riscv-gettimeofday.elf \
    /usr/lib/polyapps/riscv-sysinfo.elf \
    /usr/lib/polyapps/riscv-capget.elf \
    /usr/lib/polyapps/riscv-capset.elf \
    /usr/lib/polyapps/riscv-personality.elf \
    /usr/lib/polyapps/riscv-waitid.elf \
    /usr/lib/polyapps/riscv-wait4.elf \
    /usr/lib/polyapps/riscv-setpriority.elf \
    /usr/lib/polyapps/riscv-getpriority.elf \
    /usr/lib/polyapps/riscv-setpgid.elf \
    /usr/lib/polyapps/riscv-setsid.elf \
    /usr/lib/polyapps/riscv-umask.elf \
    /usr/lib/polyapps/riscv-prctl-set-name.elf \
    /usr/lib/polyapps/riscv-setregid.elf \
    /usr/lib/polyapps/riscv-setgid.elf \
    /usr/lib/polyapps/riscv-setreuid.elf \
    /usr/lib/polyapps/riscv-setuid.elf \
    /usr/lib/polyapps/riscv-setresuid.elf \
    /usr/lib/polyapps/riscv-getresuid.elf \
    /usr/lib/polyapps/riscv-setresgid.elf \
    /usr/lib/polyapps/riscv-getresgid.elf \
    /usr/lib/polyapps/riscv-setfsuid.elf \
    /usr/lib/polyapps/riscv-setfsgid.elf \
    /usr/lib/polyapps/riscv-getgroups.elf \
    /usr/lib/polyapps/riscv-setgroups.elf \
    /usr/lib/polyapps/riscv-kill.elf \
    /usr/lib/polyapps/riscv-tkill.elf \
    /usr/lib/polyapps/riscv-tgkill.elf \
    /usr/lib/polyapps/riscv-sigaltstack.elf \
    /usr/lib/polyapps/riscv-rt-sigaction.elf \
    /usr/lib/polyapps/riscv-rt-sigprocmask.elf \
    /usr/lib/polyapps/riscv-futex.elf \
    /usr/lib/polyapps/riscv-madvise.elf \
    /usr/lib/polyapps/riscv-mremap.elf \
    /usr/lib/polyapps/riscv-clone.elf \
    /usr/lib/polyapps/riscv-execve.elf \
    /usr/lib/polyapps/riscv-membarrier-query.elf \
    /usr/lib/polyapps/riscv-membarrier-cmd.elf \
    /usr/lib/polyapps/riscv-rseq.elf \
    /usr/lib/polyapps/riscv-mlock.elf \
    /usr/lib/polyapps/riscv-munlock.elf \
    /usr/lib/polyapps/riscv-mlockall.elf \
    /usr/lib/polyapps/riscv-munlockall.elf \
    /usr/lib/polyapps/riscv-get-mempolicy.elf \
    /usr/lib/polyapps/riscv-set-mempolicy.elf \
    /usr/lib/polyapps/riscv-migrate-pages.elf \
    /usr/lib/polyapps/riscv-move-pages.elf \
    /usr/lib/polyapps/riscv-seccomp.elf \
    /usr/lib/polyapps/riscv-bpf.elf \
    /usr/lib/polyapps/riscv-userfaultfd.elf \
    /usr/lib/polyapps/riscv-mlock2.elf \
    /usr/lib/polyapps/riscv-pkey-mprotect.elf \
    /usr/lib/polyapps/riscv-pkey-alloc.elf \
    /usr/lib/polyapps/riscv-pkey-free.elf \
    /usr/lib/polyapps/riscv-pidfd-send-signal.elf \
    /usr/lib/polyapps/riscv-io-uring-setup.elf \
    /usr/lib/polyapps/riscv-io-uring-enter.elf \
    /usr/lib/polyapps/riscv-io-uring-register.elf \
    /usr/lib/polyapps/riscv-pidfd-open.elf \
    /usr/lib/polyapps/riscv-clone3.elf \
    /usr/lib/polyapps/riscv-close-range.elf \
    /usr/lib/polyapps/riscv-openat2.elf \
    /usr/lib/polyapps/riscv-pidfd-getfd.elf \
    /usr/lib/polyapps/riscv-process-madvise.elf \
    /usr/lib/polyapps/riscv-landlock-create-ruleset.elf \
    /usr/lib/polyapps/riscv-landlock-add-rule.elf \
    /usr/lib/polyapps/riscv-landlock-restrict-self.elf \
    /usr/lib/polyapps/riscv-process-mrelease.elf \
    /usr/lib/polyapps/riscv-futex-waitv.elf \
    /usr/lib/polyapps/riscv-set-mempolicy-home-node.elf \
    /usr/lib/polyapps/riscv-set-tid-address.elf \
    /usr/lib/polyapps/riscv-set-robust-list.elf \
    /usr/lib/polyapps/riscv-get-robust-list.elf \
    /usr/lib/polyapps/riscv-prlimit64.elf \
    /usr/lib/polyapps/riscv-getrandom.elf \
    /usr/lib/polyapps/riscv-mmap.elf \
    /usr/lib/polyapps/riscv-mmap6.elf \
    /usr/lib/polyapps/riscv-mmap-store.elf \
    /usr/lib/polyapps/riscv-sys-brk.elf \
    /usr/lib/polyapps/riscv-munmap.elf \
    /usr/lib/polyapps/riscv-mprotect.elf \
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
    /usr/lib/polyapps/riscv-exit-group.elf \
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
cpu: poly_enabled=$POLY_ENABLED, poly_compat_traps=$POLY_COMPAT_TRAPS
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
      if [[ "$RUN_POLY_ARCH_TRAP_EXEC" == "1" ]]; then
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
