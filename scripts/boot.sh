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

APKINDEX_ARCHIVE="$CACHE_DIR/APKINDEX-x86_64.tar.gz"
KERNEL_IMAGE="$CACHE_DIR/vmlinuz-virt"
INITRAMFS_IMAGE="$OUT_DIR/initramfs.cpio.gz"
ISO_ROOT="$TMP_DIR/iso-root"
ISO_IMAGE="$OUT_DIR/bochs-boot.iso"
SERIAL_LOG="$OUT_DIR/serial.log"
BOCHS_LOG="$OUT_DIR/bochs.log"
BOCHSRC="$TMP_DIR/bochsrc.txt"
BOCHS_RC="$TMP_DIR/bochs.rc"
CONSOLE_LOG="$OUT_DIR/bochs-console.log"
POLY_XCR0_MODULE="$OUT_DIR/poly_xcr0.ko"
POLY_PROBE_SRC="$ROOT_DIR/tools/programs/polyprobe.c"
POLY_PROBE_BIN="$OUT_DIR/polyprobe"
POLY_CPUID_HEADER="$ROOT_DIR/tools/include/polycpuid.h"
POLY_IMPORT_ID_CHECK="$ROOT_DIR/scripts/checks/check_poly_import_ids.sh"
POLY_ARCH_CONTRACT_CHECK="$ROOT_DIR/scripts/checks/check_poly_arch_contract.sh"
POLY_CPUID_CONTRACT_CHECK="$ROOT_DIR/scripts/checks/check_poly_cpuid_contract.sh"
POLY_APP_SRC="$ROOT_DIR/tools/programs/polyapp.c"
POLY_APP_BIN="$OUT_DIR/polyapp"
POLY_EXEC_SRC="$ROOT_DIR/tools/runtime/polyexec.c"
POLY_EXEC_BIN="$OUT_DIR/polyexec"
POLY_CALL_SRC="$ROOT_DIR/tools/runtime/polycall.c"
POLY_CALL_X86_HELPERS_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_helpers.c"
POLY_CALL_BIN="$OUT_DIR/polycall"
POLY_THREAD_SRC="$ROOT_DIR/tools/programs/polythread.c"
POLY_THREAD_BIN="$OUT_DIR/polythread"
POLY_SIGNAL_SRC="$ROOT_DIR/tools/programs/polysignal.c"
POLY_SIGNAL_BIN="$OUT_DIR/polysignal"
POLY_BENCH_SRC="$ROOT_DIR/tools/programs/polybench.c"
POLY_BENCH_BIN="$OUT_DIR/polybench"
POLY_BINFMT_SRC="$ROOT_DIR/tools/runtime/polybinfmt.sh"
NATIVE_CHECK_SRC="$ROOT_DIR/tools/programs/nativecheck.c"
NATIVE_CHECK_BIN="$OUT_DIR/nativecheck"
AARCH64_POLYCALL_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/aarch64_polycall_real.c"
RISCV64_POLYCALL_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/riscv64_polycall_real.c"
POLYEXEC_GNU_HASH_REAL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_gnu_hash_real.c"
POLYEXEC_PROCESS_START_REAL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_start_real.c"
POLYEXEC_PROCESS_SYSCALL_REAL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_syscall_real.c"
POLYEXEC_PROCESS_RELOC_REAL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_reloc_real.c"
POLYEXEC_PROCESS_NEEDED_REAL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_needed_real.c"
POLYEXEC_PROCESS_VERSIONED_DEP_REAL_MAP="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_versioned_dep_real.map"
POLYCALL_STATE_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_state.c"
POLYCALL_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_import_real.c"
POLYCALL_LIBC_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_libc_import_real.c"
POLYCALL_IMPORT_VALUE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_import_value_real.c"
POLYCALL_WEAK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_weak_import_real.c"
POLYCALL_GNU_UNIQUE_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_gnu_unique_dep_real.S"
POLYCALL_GNU_UNIQUE_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_gnu_unique_main_real.c"
POLYCALL_IFUNC_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_ifunc_real.c"
POLYCALL_STACK_PROTECTOR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_stack_protector_real.c"
POLYCALL_ERRNO_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_errno_real.c"
POLYCALL_GETAUXVAL_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_getauxval_real.c"
POLYCALL_GETPAGESIZE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_getpagesize_real.c"
POLYCALL_SYSCONF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_sysconf_real.c"
POLYCALL_ENV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_env_real.c"
POLYCALL_PUTS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_puts_real.c"
POLYCALL_SNPRINTF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_snprintf_real.c"
POLYCALL_INTEGER_PARSE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_integer_parse_real.c"
POLYCALL_CTYPE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_ctype_real.c"
POLYCALL_ABS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_abs_real.c"
POLYCALL_ATOL_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_atol_real.c"
POLYCALL_FFS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_ffs_real.c"
POLYCALL_STRTOD_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_strtod_real.c"
POLYCALL_STRTOF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_strtof_real.c"
POLYCALL_FABSF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fabsf_real.c"
POLYCALL_FABS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fabs_real.c"
POLYCALL_SQRTF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_sqrtf_real.c"
POLYCALL_SQRT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_sqrt_real.c"
POLYCALL_ROUNDING_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_rounding_real.c"
POLYCALL_STRING_SEARCH_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_string_search_real.c"
POLYCALL_ALLOC_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_alloc_real.c"
POLYCALL_STRDUP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_strdup_real.c"
POLYCALL_ALIGNED_ALLOC_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_aligned_alloc_real.c"
POLYCALL_QSORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_qsort_real.c"
POLYCALL_BSEARCH_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_bsearch_real.c"
POLYCALL_QSORT_R_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_qsort_r_real.c"
POLYCALL_PTHREAD_ONCE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_once_real.c"
POLYCALL_PTHREAD_KEY_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_key_real.c"
POLYCALL_PTHREAD_MUTEX_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_mutex_real.c"
POLYCALL_PTHREAD_SELF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_self_real.c"
POLYCALL_PTHREAD_RWLOCK_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_rwlock_real.c"
POLYCALL_PTHREAD_MUTEXATTR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_mutexattr_real.c"
POLYCALL_PTHREAD_SPIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_spin_real.c"
POLYCALL_PTHREAD_COND_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pthread_cond_real.c"
POLYCALL_TIME_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_time_real.c"
POLYCALL_ATEXIT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_atexit_real.c"
POLYCALL_CXA_GUARD_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxa_guard_real.c"
POLYCALL_CXX_STATIC_GUARD_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_static_guard_real.cc"
POLYCALL_CXX_GUARD_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_guard_dep_real.cc"
POLYCALL_CXX_GUARD_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_guard_main_real.cc"
POLYCALL_CXX_VIRTUAL_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_virtual_dep_real.cc"
POLYCALL_CXX_VIRTUAL_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_virtual_main_real.cc"
POLYCALL_CXX_GLOBAL_DTOR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_global_dtor_real.cc"
POLYCALL_CXX_DEP_DTOR_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_dep_dtor_dep_real.cc"
POLYCALL_CXX_DEP_DTOR_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_dep_dtor_main_real.cc"
POLYCALL_CXX_FINALIZE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cxx_finalize_real.cc"
POLYCALL_PROCESS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_process_real.c"
POLYCALL_NEEDED_LEAF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_leaf_real.c"
POLYCALL_NEEDED_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_dep_real.c"
POLYCALL_NEEDED_OVERRIDE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_override_real.c"
POLYCALL_NEEDED_EXTRA_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_extra_real.c"
POLYCALL_NEEDED_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_main_real.c"
POLYCALL_CROSS_NEEDED_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_needed_dep_real.c"
POLYCALL_CROSS_NEEDED_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_needed_main_real.c"
POLYCALL_CROSS_NEEDED_LEAF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_needed_leaf_real.c"
POLYCALL_CROSS_NEEDED_MID_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_needed_mid_real.c"
POLYCALL_CROSS_NEEDED_TRANSITIVE_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_needed_transitive_main_real.c"
POLYCALL_CROSS_COMPACT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_compact_dep_real.c"
POLYCALL_CROSS_COMPACT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_compact_main_real.c"
POLYCALL_CROSS_IFUNC_COMPACT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_compact_dep_real.c"
POLYCALL_CROSS_IFUNC_COMPACT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_compact_main_real.c"
POLYCALL_CROSS_IFUNC_COMPACT_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_compact_root_dep_real.c"
POLYCALL_CROSS_IFUNC_COMPACT_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_compact_root_main_real.c"
POLYCALL_CROSS_IFUNC_FP64_STACK_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_fp64_stack_dep_real.c"
POLYCALL_CROSS_IFUNC_FP64_STACK_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_fp64_stack_main_real.c"
POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_fp64_stack_root_dep_real.c"
POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_fp64_stack_root_main_real.c"
POLYCALL_CROSS_IFUNC_VEC128_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_vec128_dep_real.c"
POLYCALL_CROSS_IFUNC_VEC128_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_vec128_main_real.c"
POLYCALL_CROSS_IFUNC_VEC128_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_vec128_root_dep_real.c"
POLYCALL_CROSS_IFUNC_VEC128_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_ifunc_vec128_root_main_real.c"
POLYCALL_CROSS_COMPACT_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_compact_root_dep_real.c"
POLYCALL_CROSS_COMPACT_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_compact_root_main_real.c"
POLYCALL_CROSS_FP64_STACK_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_fp64_stack_dep_real.c"
POLYCALL_CROSS_FP64_STACK_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_fp64_stack_main_real.c"
POLYCALL_CROSS_FP64_STACK_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_fp64_stack_root_dep_real.c"
POLYCALL_CROSS_FP64_STACK_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_fp64_stack_root_main_real.c"
POLYCALL_CROSS_VEC128_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_vec128_dep_real.c"
POLYCALL_CROSS_VEC128_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_vec128_main_real.c"
POLYCALL_CROSS_VEC128_ROOT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_vec128_root_dep_real.c"
POLYCALL_CROSS_VEC128_ROOT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cross_vec128_root_main_real.c"
POLYCALL_SYMBOLIC_OVERRIDE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_symbolic_override_real.c"
POLYCALL_SYMBOLIC_TARGET_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_symbolic_target_real.c"
POLYCALL_PROTECTED_TARGET_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_protected_target_real.c"
POLYCALL_SYMBOLIC_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_symbolic_main_real.c"
POLYCALL_ABS_NEEDED_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_abs_needed_dep_real.c"
POLYCALL_ABS_NEEDED_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_abs_needed_main_real.c"
POLYCALL_PRELOAD_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_dep_real.c"
POLYCALL_PRELOAD_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_main_real.c"
POLYCALL_PRELOAD_OVERRIDE_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_override_dep_real.c"
POLYCALL_PRELOAD_SECOND_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_second_dep_real.c"
POLYCALL_PRELOAD_CHAIN_LEAF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_chain_leaf_real.c"
POLYCALL_PRELOAD_CHAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preload_chain_real.c"
POLYCALL_RUNPATH_PREFER_BAD_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_runpath_prefer_bad_dep_real.c"
POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_rpath_inherit_leaf_real.c"
POLYCALL_RPATH_INHERIT_MID_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_rpath_inherit_mid_real.c"
POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_rpath_inherit_main_real.c"
POLYCALL_SONAME_ONCE_LEAF_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_soname_once_leaf_real.c"
POLYCALL_SONAME_ONCE_DEP_A_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_soname_once_dep_a_real.c"
POLYCALL_SONAME_ONCE_DEP_B_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_soname_once_dep_b_real.c"
POLYCALL_SONAME_ONCE_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_soname_once_main_real.c"
POLYCALL_MANY_NEEDED_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_many_needed_dep_real.c"
POLYCALL_MANY_NEEDED_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_many_needed_main_real.c"
POLYCALL_ROOT_EXPORT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_export_dep_real.c"
POLYCALL_ROOT_EXPORT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_export_main_real.c"
POLYCALL_ROOT_TLS_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_tls_dep_real.c"
POLYCALL_ROOT_TLS_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_tls_main_real.c"
POLYCALL_ROOT_IFUNC_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_ifunc_dep_real.c"
POLYCALL_ROOT_IFUNC_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_ifunc_main_real.c"
POLYCALL_ROOT_WEAK_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_weak_dep_real.c"
POLYCALL_ROOT_WEAK_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_root_weak_main_real.c"
POLYCALL_NEEDED_TLS_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_tls_dep_real.c"
POLYCALL_NEEDED_TLS_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_tls_main_real.c"
POLYCALL_NEEDED_TLS_EXTERNAL_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_tls_external_main_real.c"
POLYCALL_VERSIONED_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_versioned_dep_real.c"
POLYCALL_VERSIONED_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_versioned_main_real.c"
POLYCALL_VERSIONED_DEP_REAL_MAP="$ROOT_DIR/tools/fixtures/polycall/polycall_versioned_dep_real.map"
POLYCALL_NEEDED_IFUNC_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_ifunc_dep_real.c"
POLYCALL_NEEDED_IFUNC_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_ifunc_main_real.c"
POLYCALL_NEEDED_DT_INIT_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_dt_init_dep_real.c"
POLYCALL_NEEDED_DT_INIT_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_dt_init_main_real.c"
POLYCALL_NEEDED_RELRO_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_relro_dep_real.c"
POLYCALL_NEEDED_RELRO_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_needed_relro_main_real.c"
POLYCALL_COPY_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_copy_dep_real.c"
POLYCALL_COPY_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_copy_main_real.c"
POLYCALL_FUNCPTR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_funcptr_real.c"
POLYCALL_PAIR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_pair_real.c"
POLYCALL_SRET_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_sret_real.c"
POLYCALL_CTOR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_ctor_real.c"
POLYCALL_FINI_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fini_real.c"
POLYCALL_DT_INIT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_dt_init_real.c"
POLYCALL_PREINIT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_preinit_real.c"
POLYCALL_INIT_ORDER_DEP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_init_order_dep_real.c"
POLYCALL_INIT_ORDER_MAIN_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_init_order_main_real.c"
POLYCALL_RELRO_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_relro_real.c"
POLYCALL_TLS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_tls_real.c"
POLYCALL_TLS_INITIAL_EXEC_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_tls_initial_exec_real.c"
POLYCALL_COND_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cond_real.c"
POLYCALL_SELECT_VARIANTS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_select_variants_real.c"
POLYCALL_CBZ_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_cbz_real.c"
POLYCALL_BITBRANCH_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_bitbranch_real.c"
POLYCALL_UBFM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_ubfm_real.c"
POLYCALL_SBFM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_sbfm_real.c"
POLYCALL_SIGNED_EXT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_signed_ext_real.c"
POLYCALL_SIGNED_LOAD_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_signed_load_real.c"
POLYCALL_INT_DIV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_div_real.c"
POLYCALL_INT_MADD_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_madd_real.c"
POLYCALL_INT_HIGHMUL_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_highmul_real.c"
POLYCALL_INT128_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int128_helpers_real.c"
POLYCALL_INT128_FP_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int128_fp_helpers_real.c"
POLYCALL_INT128_FLOAT_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int128_float_helpers_real.c"
POLYCALL_BIT_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_bit_helpers_real.c"
POLYCALL_LONGDOUBLE_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_longdouble_helpers_real.c"
POLYCALL_LONGDOUBLE_SIGNED_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_longdouble_signed_helpers_real.c"
POLYCALL_LONGDOUBLE_COMPARE_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_longdouble_compare_helpers_real.c"
POLYCALL_LONGDOUBLE_INT32_HELPERS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_longdouble_int32_helpers_real.c"
POLYCALL_INT_CARRY_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_carry_real.c"
POLYCALL_INT_VARSHIFT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_varshift_real.c"
POLYCALL_INT_LOGIC_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_logic_real.c"
POLYCALL_INT_BITOPS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_bitops_real.c"
POLYCALL_INT_ROTATE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_rotate_real.c"
POLYCALL_INT_CCMP_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_ccmp_real.c"
POLYCALL_POSTINDEX_MEM_AARCH64_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_postindex_mem_aarch64.c"
POLYCALL_ATOMIC_AARCH64_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_atomic_aarch64.c"
POLYCALL_ATOMIC_RISCV_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_atomic_riscv.c"
POLYCALL_UNSCALED_MEM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_unscaled_mem_real.c"
POLYCALL_INDEXED_MEM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_indexed_mem_real.c"
POLYCALL_CALLEE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_callee_real.c"
POLYCALL_FP64_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_real.c"
POLYCALL_FP64_STACK_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_stack_real.c"
POLYCALL_FPAIR_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fpair_real.c"
POLYCALL_FPAIR32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fpair32_real.c"
POLYCALL_FPAIR_ARG_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fpair_arg_real.c"
POLYCALL_FPAIR32_ARG_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fpair32_arg_real.c"
POLYCALL_HFA3_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hfa3_real.c"
POLYCALL_HFA4_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hfa4_real.c"
POLYCALL_HFA3_F32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hfa3_f32_real.c"
POLYCALL_HFA4_F32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hfa4_f32_real.c"
POLYCALL_HFA_ARG_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hfa_arg_real.c"
POLYCALL_VEC128_AARCH64_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_vec128_aarch64_real.c"
POLYCALL_HETERO_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_real.c"
POLYCALL_HETERO_REV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_rev_real.c"
POLYCALL_HETERO32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero32_real.c"
POLYCALL_HETERO32_REV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero32_rev_real.c"
POLYCALL_HETERO_U32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_u32_real.c"
POLYCALL_HETERO_U32_REV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_u32_rev_real.c"
POLYCALL_HETERO_U32_F32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_u32_f32_real.c"
POLYCALL_HETERO_F32_U32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_hetero_f32_u32_real.c"
POLYCALL_MIXED_ARGS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_mixed_args_real.c"
POLYCALL_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_import_real.c"
POLYCALL_X86_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp64_import_real.c"
POLYCALL_X86_FP64_SUM8_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp64_sum8_import_real.c"
POLYCALL_X86_FP64_SUM10_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp64_sum10_import_real.c"
POLYCALL_X86_FP64_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp64_callee_import_real.c"
POLYCALL_X86_FP64_CALLEE_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp64_callee_stack_import_real.c"
POLYCALL_X86_FPAIR64_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fpair64_import_real.c"
POLYCALL_X86_FPAIR64_FP64_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fpair64_fp64_callee_import_real.c"
POLYCALL_X86_FPAIR32_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fpair32_import_real.c"
POLYCALL_X86_FPAIR32_FP32_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fpair32_fp32_callee_import_real.c"
POLYCALL_X86_VEC128_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_vec128_import_real.c"
POLYCALL_X86_VEC128_FP64_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_vec128_fp64_callee_import_real.c"
POLYCALL_X86_SRET_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sret_import_real.c"
POLYCALL_X86_SRET_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sret_stack_import_real.c"
POLYCALL_X86_SRET_STACK10_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sret_stack10_import_real.c"
POLYCALL_X86_SRET_CALLEE_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sret_callee_stack_import_real.c"
POLYCALL_X86_SRET_FP64_CALLEE_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sret_fp64_callee_stack_import_real.c"
POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_mixed_u64_fp64_import_real.c"
POLYCALL_X86_MIXED_U64_FP64_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_mixed_u64_fp64_callee_import_real.c"
POLYCALL_X86_MIXED_U64_FP64_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_mixed_u64_fp64_stack_import_real.c"
POLYCALL_X86_FP32_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_fp32_import_real.c"
POLYCALL_X86_SUM8_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sum8_import_real.c"
POLYCALL_X86_SUM10_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sum10_import_real.c"
POLYCALL_X86_SUM14_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sum14_import_real.c"
POLYCALL_X86_ALIGN14_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_align14_import_real.c"
POLYCALL_X86_I128_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_i128_import_real.c"
POLYCALL_X86_I128_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_i128_callee_import_real.c"
POLYCALL_X86_CALLEE_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_callee_import_real.c"
POLYCALL_X86_CALLEE_STACK_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_callee_stack_import_real.c"
POLYCALL_X86_SUM8_POST_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_x86_sum8_post_import_real.c"
POLYCALL_FP32_IMPORT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_import_real.c"
POLYCALL_FP64_CALLEE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_callee_real.c"
POLYCALL_FP32_CALLEE_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_callee_real.c"
POLYCALL_FP64_COND_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_cond_real.c"
POLYCALL_FP64_DIV_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_div_real.c"
POLYCALL_FP64_UNARY_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_unary_real.c"
POLYCALL_FP64_ABS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_abs_real.c"
POLYCALL_FP64_SQRT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_sqrt_real.c"
POLYCALL_FP64_FMA_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_fma_real.c"
POLYCALL_FP64_FMA_VARIANTS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_fma_variants_real.c"
POLYCALL_FP64_MINMAX_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_minmax_real.c"
POLYCALL_FP64_SELECT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_select_real.c"
POLYCALL_FP64_INDEXED_MEM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_indexed_mem_real.c"
POLYCALL_FP64_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_convert_real.c"
POLYCALL_FP64_SIGNED_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_signed_convert_real.c"
POLYCALL_FP64_I32_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_i32_convert_real.c"
POLYCALL_FP64_U32_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp64_u32_convert_real.c"
POLYCALL_FP_MIXED_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp_mixed_convert_real.c"
POLYCALL_INT_FP_CONVERT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_int_fp_convert_real.c"
POLYCALL_FP32_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_real.c"
POLYCALL_FP32_ABS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_abs_real.c"
POLYCALL_FP32_SQRT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_sqrt_real.c"
POLYCALL_FP32_FMA_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_fma_real.c"
POLYCALL_FP32_FMA_VARIANTS_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_fma_variants_real.c"
POLYCALL_FP32_MINMAX_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_minmax_real.c"
POLYCALL_FP32_SELECT_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_select_real.c"
POLYCALL_FP32_MEM_REAL_SRC="$ROOT_DIR/tools/fixtures/polycall/polycall_fp32_mem_real.c"
POLY_APP_PAYLOAD_DIR="$ROOT_DIR/tools/fixtures/polyapps"
POLY_ELF_GEN_SRC="$ROOT_DIR/tools/build/mkpolyelf.c"
POLY_ELF_GEN_BIN="$OUT_DIR/mkpolyelf"
POLY_ENABLED="${POLY_ENABLED:-0}"
RUN_POLY_PROBE="${RUN_POLY_PROBE:-0}"
RUN_POLY_APPS="${RUN_POLY_APPS:-0}"
RUN_POLY_NEUTRAL="${RUN_POLY_NEUTRAL:-0}"
RUN_POLY_EXEC="${RUN_POLY_EXEC:-$RUN_POLY_APPS}"
RUN_POLY_ARCH_TRAP_EXEC="${RUN_POLY_ARCH_TRAP_EXEC:-0}"
RUN_POLY_CALL="${RUN_POLY_CALL:-$RUN_POLY_APPS}"
RUN_POLY_THREAD="${RUN_POLY_THREAD:-$RUN_POLY_CALL}"
RUN_POLY_SIGNAL="${RUN_POLY_SIGNAL:-$RUN_POLY_THREAD}"
RUN_POLY_BENCH="${RUN_POLY_BENCH:-0}"
RUN_POLY_BINFMT="${RUN_POLY_BINFMT:-0}"
RUN_POLY_BINFMT_ARCH_TRAPS="${RUN_POLY_BINFMT_ARCH_TRAPS:-0}"
RUN_NATIVE_CHECK="${RUN_NATIVE_CHECK:-0}"
EXPECT_POLY_CPUID="${EXPECT_POLY_CPUID:-0}"
REQUIRE_POLY_REAL_XSAVE="${REQUIRE_POLY_REAL_XSAVE:-0}"
RUN_CONTRACT_CHECKS="${RUN_CONTRACT_CHECKS:-0}"
BOCHS_BIOS_DIR=""
if [[ -d "$ROOT_DIR/bochs-src/bochs/bios" ]]; then
  BOCHS_BIOS_DIR="$ROOT_DIR/bochs-src/bochs/bios"
elif [[ -d "$ROOT_DIR/bochs-prepoly-src/bochs/bios" ]]; then
  BOCHS_BIOS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs/bios"
fi

if [[ "$RUN_CONTRACT_CHECKS" == "1" ]]; then
  "$POLY_IMPORT_ID_CHECK"
  "$POLY_ARCH_CONTRACT_CHECK"
  "$POLY_CPUID_CONTRACT_CHECK"
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
  local link_mode="${4:-static}"

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

  if [[ -x "$bin" && "$bin" -nt "$src" && "$bin" -nt "$POLY_CPUID_HEADER" ]]; then
    if [[ "$link_mode" != "static-pie" ]] ||
        readelf -h "$bin" 2>/dev/null | grep -q 'Type:[[:space:]]*DYN'; then
      return
    fi
  fi

  local -a compiler_args=(-O2 -s -fno-stack-protector)
  if [[ "$link_mode" == "static-pie" ]]; then
    compiler_args+=(-static-pie -fPIE)
  else
    compiler_args+=(-static)
  fi
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
  compile_poly_tool "$POLY_EXEC_SRC" "$POLY_EXEC_BIN" "${POLY_EXEC_CC:-}" static-pie
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
  if [[ -x "$POLY_THREAD_BIN" && "$POLY_THREAD_BIN" -nt "$POLY_THREAD_SRC" &&
      "$POLY_THREAD_BIN" -nt "$POLY_CPUID_HEADER" ]]; then
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
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/aarch64"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/riscv"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv"
  mkdir -p "$TMP_DIR/poly-link/aarch64"
  mkdir -p "$TMP_DIR/poly-link/riscv"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$AARCH64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$AARCH64_POLYCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-gnu-hash-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$POLYEXEC_GNU_HASH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-polyexec-gnu-hash-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_START_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-argv-envp-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_SYSCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-syscall-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_RELOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-reloc-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_PRELOAD_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreload-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesspreload-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_PRELOAD_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreload-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/libpolyprocesspreload-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_PRELOAD_SECOND_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreloadsecond-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesspreloadsecond-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessenv-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/aarch64/libpolyprocessenv-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocesscrossneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesscrossneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocesscrossneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolyprocesscrossneeded-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_IFUNC_DEP \
    -Wl,-soname,libpolyprocessifuncneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessifuncneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_IFUNC_DEP \
    -Wl,-soname,libpolyprocessifuncneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolyprocessifuncneeded-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps/libpolyprocessrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessrpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps/libpolyprocessrpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_PRELOAD_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-preload-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_PRELOAD_SECOND_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-preload-second-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/aarch64" \
    -Wl,--no-as-needed -l:libpolyprocessenv-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-envpath-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolyprocesscrossneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-cross-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_IFUNC_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessifuncneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-ifunc-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_IFUNC_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolyprocessifuncneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-cross-needed-ifunc-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/processdeps' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps" \
    -Wl,--no-as-needed -l:libpolyprocessrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-runpath-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/processdeps' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps" \
    -Wl,--no-as-needed -l:libpolyprocessrpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-rpath-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_LEAF \
    -Wl,-soname,libpolyprocessneededleaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneededleaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_MID \
    -Wl,-soname,libpolyprocessneededmid-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededleaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneededmid-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_TRANSITIVE_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededmid-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-transitive-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_NEEDED_INDIRECT_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededmid-aarch64.so \
    -Wl,--unresolved-symbols=ignore-all \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-indirect-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_ROOT_EXPORT_DEP \
    -Wl,-soname,libpolyprocessrootexport-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--unresolved-symbols=ignore-all \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessrootexport-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_ROOT_EXPORT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessrootexport-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-root-export-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_ROOT_IFUNC_DEP \
    -Wl,-soname,libpolyprocessrootifunc-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--unresolved-symbols=ignore-all \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessrootifunc-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_ROOT_IFUNC_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessrootifunc-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-needed-root-ifunc-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_WEAK_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-weak-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_WEAK_DEP \
    -Wl,-soname,libpolyprocessweak-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessweak-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_WEAK_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessweak-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-weak-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_INIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-init-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize \
    -nostdlib -nodefaultlibs -no-pie -DPOLY_PROCESS_PREINIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-preinit-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_INIT_DEP \
    -Wl,-soname,libpolyprocessinit-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessinit-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_INIT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessinit-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-init-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_INIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,-init,poly_process_dt_init_root \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-dt-init-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_FINI_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-fini-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_FINI_EXIT_GROUP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-fini-exit-group-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_FINI_ORDER_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-fini-order-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_FINI_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,-fini,poly_process_dt_fini_root \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-dt-fini-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_FINI_DEP \
    -Wl,-soname,libpolyprocessfini-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessfini-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_FINI_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessfini-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-fini-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_FINI_DEP \
    -Wl,-soname,libpolyprocessdtfini-aarch64.so \
    -Wl,-fini,poly_process_dt_fini_dep_dtor \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessdtfini-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_FINI_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessdtfini-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-dt-fini-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_INIT_DEP \
    -Wl,-soname,libpolyprocessdtinit-aarch64.so \
    -Wl,-init,poly_process_dt_init_dep_ctor \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessdtinit-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_DT_INIT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessdtinit-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-dt-init-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_VERSIONED_DEP \
    -Wl,-soname,libpolyprocessversioned-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--version-script="$POLYEXEC_PROCESS_VERSIONED_DEP_REAL_MAP" \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessversioned-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_VERSIONED_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessversioned-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-versioned-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_DEP \
    -Wl,-soname,libpolyprocesstls-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesstls-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesstls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_DEFAULT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-default-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_DEFAULT_DEP \
    -Wl,-soname,libpolyprocesstlsdefault-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesstlsdefault-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_TLS_DEFAULT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesstlsdefault-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-default-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -mtls-dialect=trad \
    -DPOLY_PROCESS_TLS_TRAD_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-trad-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -mtls-dialect=trad \
    -DPOLY_PROCESS_TLS_TRAD_DEP \
    -Wl,-soname,libpolyprocesstlstrad-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesstlstrad-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -mtls-dialect=trad \
    -DPOLY_PROCESS_TLS_TRAD_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesstlstrad-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-tls-trad-needed-real.elf"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_COPY_DEP \
    -Wl,-soname,libpolyprocesscopy-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesscopy-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fno-pic \
    -no-pie -nostdlib -nodefaultlibs -DPOLY_PROCESS_COPY_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesscopy-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-process-copy-reloc-real.elf"
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
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_QSORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-qsort-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BSEARCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-bsearch-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_QSORT_R_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-qsort-r-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_ONCE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-once-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_KEY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-key-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_MUTEX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-mutex-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_SELF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-self-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_RWLOCK_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-rwlock-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_MUTEXATTR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-mutexattr-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_SPIN_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-spin-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-pthread-cond-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TIME_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-time-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IMPORT_VALUE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-import-value-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_WEAK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-weak-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyunique-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_GNU_UNIQUE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyunique-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GNU_UNIQUE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyunique-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-gnu-unique-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IFUNC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ifunc-real.so"
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
    "$POLYCALL_PUTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-puts-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SNPRINTF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-snprintf-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INTEGER_PARSE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-integer-parse-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CTYPE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ctype-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-abs-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-atol-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FFS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ffs-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRTOD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-strtod-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRTOF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-strtof-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FABSF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fabsf-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-fabs-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SQRTF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sqrtf-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-sqrt-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROUNDING_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-rounding-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRING_SEARCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-string-search-real.so"
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
    "$POLYCALL_CXA_GUARD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxa-guard-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_STATIC_GUARD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-static-guard-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fno-devirtualize -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycxxvirtual-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_VIRTUAL_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxvirtual-aarch64.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fno-devirtualize -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_VIRTUAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxvirtual-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-virtual-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_GLOBAL_DTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-global-dtor-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_FINALIZE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-finalize-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycxxdepdtor-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_DEP_DTOR_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxdepdtor-aarch64.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_DEP_DTOR_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxdepdtor-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-dep-dtor-real.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycxxguard-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_GUARD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxguard-aarch64.so"
  aarch64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_GUARD_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxguard-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cxx-guard-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PROCESS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-process-real.so"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps"
  mkdir -p "$TMP_DIR/poly-link/aarch64"
  mkdir -p "$TMP_DIR/poly-link/riscv"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/aarch64"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/riscv"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/envorigin"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathinherit"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/mid"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/leaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/mid"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/leaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/mid"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/rootleaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/runleaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/mid"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/rootleaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/runleaf"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/a"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/b"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafa"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafb"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/a"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/b"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafa"
  mkdir -p "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafb"
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
    -Wl,-soname,libpolycrossneeded-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossvec-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrosscompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrosscompact-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifunccompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifunccompact-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifuncfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifuncfp64stack-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifuncvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifuncvec-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossfp64stack-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootvec-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootcompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootcompact-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifunccompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifunccompact-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifuncfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifuncfp64stack-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifuncvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifuncvec-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootfp64stack-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossleaf-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossleaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossleaf-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossleaf-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossmid-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_MID_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossleaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossmid-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_TRANSITIVE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycrossmid-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-needed-transitive-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossneeded-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossneeded-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossvec-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrosscompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrosscompact-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifunccompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossifunccompact-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifuncfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossifuncfp64stack-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossifuncvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossifuncvec-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossfp64stack-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootvec-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootcompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootcompact-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifunccompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootifunccompact-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifuncfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootifuncfp64stack-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootifuncvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootifuncvec-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycrossrootfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolycrossrootfp64stack-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossvec-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-vec128-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrosscompact-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-compact-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossifunccompact-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-ifunc-compact-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossifuncfp64stack-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-ifunc-fp64-stack-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossifuncvec-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-ifunc-vec128-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossfp64stack-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-fp64-stack-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootvec-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-vec128-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootcompact-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-compact-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootifunccompact-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-compact-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootifuncfp64stack-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-fp64-stack-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootifuncvec-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-vec128-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolycrossrootfp64stack-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-fp64-stack-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysymbolic-override-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_OVERRIDE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-override-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysymbolic-target-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysymbolic-target-bsymbolic-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none -Wl,-Bsymbolic \
    "$POLYCALL_SYMBOLIC_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-bsymbolic-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysymbolic-target-protected-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PROTECTED_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-protected-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-aarch64.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-symbolic-preempt-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-aarch64.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-bsymbolic-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-symbolic-bind-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-aarch64.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-protected-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-symbolic-protected-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,/usr/lib/polyapps/libpolyabsneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyabsneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyabsneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-abs-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreload-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreload-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadoverride-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_OVERRIDE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadoverride-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadoverride-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_OVERRIDE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/libpolypreloadoverride-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadsecond-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_SECOND_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadsecond-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadchainleaf-aarch64.so \
    -Wl,-init,poly_preload_chain_leaf_init \
    -Wl,-fini,poly_preload_chain_leaf_fini \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_CHAIN_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadchainleaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolypreloadchain-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_CHAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolypreloadchainleaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadchain-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_MAIN_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-preload-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolypreloadneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-preload-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,'$ORIGIN/polydeps/libpolyoriginneeded-aarch64.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyoriginneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyoriginneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-origin-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,'$ORIGIN/${PLATFORM}/polydeps/libpolyplatformneeded-aarch64.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps/libpolyplatformneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps" \
    -Wl,--no-as-needed -l:libpolyplatformneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-platform-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,'$ORIGIN/${LIB}/polydeps/libpolylibneeded-aarch64.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolylibneeded-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolylibneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-lib-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyabsrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyabsrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,/usr/lib/polyapps/polydeps \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyabsrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-abs-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-rpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathinherit-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathinherit/libpolyrpathinherit-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathinherit-mid-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathinherit" \
    -Wl,--no-as-needed -l:libpolyrpathinherit-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrpathinherit-mid-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/rpathinherit' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrpathinherit-mid-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-rpath-inherit-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathorigin-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/leaf/libpolyrpathorigin-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathorigin-mid-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/leaf" \
    -Wl,--no-as-needed -l:libpolyrpathorigin-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/mid/libpolyrpathorigin-mid-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/mid:$ORIGIN/leaf' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/mid" \
    -Wl,--no-as-needed -l:libpolyrpathorigin-mid-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/aarch64/aarch64-pcall-rpath-origin-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathrunpath-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/rootleaf/libpolyrpathrunpath-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -DPOLY_RPATH_INHERIT_VALUE=1700 \
    -Wl,-soname,libpolyrpathrunpath-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/runleaf/libpolyrpathrunpath-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrpathrunpath-mid-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../runleaf' \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/runleaf" \
    -Wl,--no-as-needed -l:libpolyrpathrunpath-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/mid/libpolyrpathrunpath-mid-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/mid:$ORIGIN/rootleaf' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/mid" \
    -Wl,--no-as-needed -l:libpolyrpathrunpath-mid-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/aarch64/aarch64-pcall-rpath-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysonameonce-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SONAME_ONCE_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafa/libpolysonameonce-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -DPOLY_SONAME_ONCE_VALUE=200 \
    -Wl,-soname,libpolysonameonce-leaf-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SONAME_ONCE_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafb/libpolysonameonce-leaf-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysonameonce-a-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../leafa' \
    "$POLYCALL_SONAME_ONCE_DEP_A_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafa" \
    -Wl,--no-as-needed -l:libpolysonameonce-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/a/libpolysonameonce-a-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolysonameonce-b-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../leafb' \
    "$POLYCALL_SONAME_ONCE_DEP_B_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/leafb" \
    -Wl,--no-as-needed -l:libpolysonameonce-leaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/b/libpolysonameonce-b-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/a:$ORIGIN/b' \
    "$POLYCALL_SONAME_ONCE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/a" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/b" \
    -Wl,--no-as-needed -l:libpolysonameonce-a-aarch64.so \
    -Wl,--no-as-needed -l:libpolysonameonce-b-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/aarch64-pcall-soname-once-real.so"
  aarch64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -DPOLY_PROCESS_SONAME_ONCE_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/a:$ORIGIN/b' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/a" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/b" \
    -Wl,--no-as-needed -l:libpolysonameonce-a-aarch64.so \
    -Wl,--no-as-needed -l:libpolysonameonce-b-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/aarch64/aarch64-process-soname-once-real.elf"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycolonrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolycolonrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/missing:$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolycolonrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-colon-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolybracedorigin-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolybracedorigin-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'${ORIGIN}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedorigin-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-braced-origin-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolylibrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolylibrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/$LIB/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolylibrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-lib-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolybracedlibrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolybracedlibrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/${LIB}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedlibrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-braced-lib-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyplatformrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps/libpolyplatformrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/$PLATFORM/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps" \
    -Wl,--no-as-needed -l:libpolyplatformrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-platform-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolybracedplatformrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps/libpolybracedplatformrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/${PLATFORM}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedplatformrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-braced-platform-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyenvpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/libpolyenvpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps" \
    -Wl,--no-as-needed -l:libpolyenvpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ld-library-path-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyenvplatform-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/aarch64/libpolyenvplatform-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/aarch64" \
    -Wl,--no-as-needed -l:libpolyenvplatform-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ld-platform-path-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyenvorigin-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envorigin/libpolyenvorigin-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envorigin" \
    -Wl,--no-as-needed -l:libpolyenvorigin-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ld-origin-path-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyenvprefer-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/libpolyenvprefer-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyenvprefer-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RUNPATH_PREFER_BAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyenvprefer-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps" \
    -Wl,--no-as-needed -l:libpolyenvprefer-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-ld-prefer-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrelrunpath-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrelrunpath-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,usr/lib/polyapps/polydeps \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrelrunpath-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relative-runpath-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrunpathprefer-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RUNPATH_PREFER_BAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrunpathprefer-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrunpathprefer-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrunpathprefer-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrunpathprefer-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-runpath-prefer-real.so"
  for idx in 1 2 3 4 5 6 7 8 9; do
    aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
      -DPOLY_MANY_NEEDED_INDEX="$idx" \
      -Wl,-soname,libpolymanyneeded"$idx"-aarch64.so \
      -Wl,--hash-style=sysv -Wl,--build-id=none \
      "$POLYCALL_MANY_NEEDED_DEP_REAL_SRC" \
      -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolymanyneeded$idx-aarch64.so"
  done
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MANY_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolymanyneeded1-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded2-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded3-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded4-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded5-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded6-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded7-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded8-aarch64.so \
    -Wl,--no-as-needed -l:libpolymanyneeded9-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-many-needed-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrootdep-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_EXPORT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootdep-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_EXPORT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootdep-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-root-export-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyroottls-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyroottls-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyroottls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-root-tls-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyroottls-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolyroottls-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolyroottls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-tls-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrootifunc-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootifunc-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootifunc-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-root-ifunc-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrootifunc-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolyrootifunc-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolyrootifunc-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyrootweak-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_WEAK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootweak-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_WEAK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootweak-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-root-weak-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneededtls-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededtls-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededtls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-tls-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_EXTERNAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededtls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-tls-external-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneededtls-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/aarch64/libpolyneededtls-riscv.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_EXTERNAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/aarch64" \
    -Wl,--no-as-needed -l:libpolyneededtls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-cross-needed-tls-external-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyversioned-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none -Wl,--version-script="$POLYCALL_VERSIONED_DEP_REAL_MAP" \
    "$POLYCALL_VERSIONED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyversioned-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_VERSIONED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyversioned-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-versioned-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneededifunc-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededifunc-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededifunc-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-ifunc-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneededdtinit-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none -Wl,-init,poly_needed_dt_init \
    -Wl,-fini,poly_needed_dt_fini \
    "$POLYCALL_NEEDED_DT_INIT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededdtinit-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_DT_INIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededdtinit-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-dt-init-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolyneededrelro-aarch64.so \
    -Wl,-z,relro -Wl,-z,now \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_RELRO_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededrelro-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_RELRO_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededrelro-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-needed-relro-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-soname,libpolycopy-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_COPY_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycopy-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fno-pie -no-pie -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_COPY_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycopy-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-copy-reloc.elf"
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
    -Wl,-e,poly_entry -Wl,-init,poly_dt_init -Wl,-fini,poly_dt_fini \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_DT_INIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-dt-init-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -pie -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PREINIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-preinit-real.elf"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,-z,relro -Wl,-z,now \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RELRO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-relro-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -DPOLY_INIT_ORDER_DIGIT=1 \
    -DPOLY_INIT_ORDER_FUNC=poly_init_order_normal_touch \
    -Wl,-soname,libpolyinitnormal-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyinitnormal-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -DPOLY_INIT_ORDER_DIGIT=2 \
    -DPOLY_INIT_ORDER_FUNC=poly_init_order_first_touch \
    -Wl,-z,initfirst -Wl,-soname,libpolyinitfirst-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyinitfirst-aarch64.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyinitnormal-aarch64.so \
    -Wl,--no-as-needed -l:libpolyinitfirst-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-initfirst-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-tls-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -mtls-dialect=trad \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TLS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-tls-trad-real.so"
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
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HFA3_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hfa3-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HFA4_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hfa4-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HFA3_F32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hfa3-f32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HFA4_F32_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hfa4-f32-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_hfa3_f64_arg -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_HFA_ARG_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so"
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
    "$POLYCALL_VEC128_AARCH64_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-vec128-real.so"
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
    "$POLYCALL_X86_FP64_SUM10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp64-sum10-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-stack-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fpair64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR64_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fpair64-fp64-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fpair32-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR32_FP32_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-fpair32-fp32-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_VEC128_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-vec128-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_VEC128_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-vec128-fp64-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sret-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sret-stack-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_STACK10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sret-stack10-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sret-callee-stack-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_FP64_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sret-fp64-callee-stack-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-stack-import-real.so"
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
    "$POLYCALL_X86_SUM10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sum10-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM14_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-sum14-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_ALIGN14_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-align14-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_I128_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-i128-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_I128_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-i128-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-callee-import-real.so"
  aarch64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-x86-callee-stack-import-real.so"
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
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$POLYEXEC_GNU_HASH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-polyexec-gnu-hash-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64gc -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=gnu -Wl,--build-id=none \
    "$POLYEXEC_GNU_HASH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-polyexec-gnu-hash-rv64gc.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_START_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-argv-envp-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_SYSCALL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-syscall-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_RELOC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-reloc-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_PRELOAD_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreload-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesspreload-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_PRELOAD_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreload-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/libpolyprocesspreload-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_PRELOAD_SECOND_OVERRIDE_DEP \
    -Wl,-soname,libpolyprocesspreloadsecond-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesspreloadsecond-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessenv-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/riscv/libpolyprocessenv-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocesscrossneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesscrossneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocesscrossneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolyprocesscrossneeded-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_IFUNC_DEP \
    -Wl,-soname,libpolyprocessifuncneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessifuncneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_IFUNC_DEP \
    -Wl,-soname,libpolyprocessifuncneeded-aarch64.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolyprocessifuncneeded-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps/libpolyprocessrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_DEP \
    -Wl,-soname,libpolyprocessrpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps/libpolyprocessrpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_PRELOAD_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-preload-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_PRELOAD_SECOND_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-preload-second-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processenvdeps/riscv" \
    -Wl,--no-as-needed -l:libpolyprocessenv-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-envpath-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolyprocesscrossneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-cross-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_IFUNC_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessifuncneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-ifunc-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_IFUNC_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolyprocessifuncneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-cross-needed-ifunc-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/processdeps' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps" \
    -Wl,--no-as-needed -l:libpolyprocessrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-runpath-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/processdeps' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/processdeps" \
    -Wl,--no-as-needed -l:libpolyprocessrpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-rpath-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_LEAF \
    -Wl,-soname,libpolyprocessneededleaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneededleaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_MID \
    -Wl,-soname,libpolyprocessneededmid-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededleaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessneededmid-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_TRANSITIVE_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededmid-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-transitive-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_NEEDED_INDIRECT_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneededmid-riscv.so \
    -Wl,--unresolved-symbols=ignore-all \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-indirect-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_ROOT_EXPORT_DEP \
    -Wl,-soname,libpolyprocessrootexport-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--unresolved-symbols=ignore-all \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessrootexport-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_ROOT_EXPORT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessrootexport-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-root-export-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_ROOT_IFUNC_DEP \
    -Wl,-soname,libpolyprocessrootifunc-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--unresolved-symbols=ignore-all \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessrootifunc-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_ROOT_IFUNC_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessrootifunc-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-needed-root-ifunc-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_WEAK_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-weak-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_WEAK_DEP \
    -Wl,-soname,libpolyprocessweak-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessweak-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_WEAK_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessweak-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-weak-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_INIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-init-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d -no-pie \
    -DPOLY_PROCESS_PREINIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-preinit-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_INIT_DEP \
    -Wl,-soname,libpolyprocessinit-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessinit-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_INIT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessinit-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-init-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_INIT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,-init,poly_process_dt_init_root \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-dt-init-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_FINI_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-fini-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_FINI_EXIT_GROUP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-fini-exit-group-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_FINI_ORDER_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-fini-order-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_FINI_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,-fini,poly_process_dt_fini_root \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-dt-fini-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_FINI_DEP \
    -Wl,-soname,libpolyprocessfini-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessfini-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_FINI_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessfini-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-fini-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_FINI_DEP \
    -Wl,-soname,libpolyprocessdtfini-riscv.so \
    -Wl,-fini,poly_process_dt_fini_dep_dtor \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessdtfini-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_FINI_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessdtfini-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-dt-fini-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_INIT_DEP \
    -Wl,-soname,libpolyprocessdtinit-riscv.so \
    -Wl,-init,poly_process_dt_init_dep_ctor \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessdtinit-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_DT_INIT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessdtinit-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-dt-init-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_VERSIONED_DEP \
    -Wl,-soname,libpolyprocessversioned-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--version-script="$POLYEXEC_PROCESS_VERSIONED_DEP_REAL_MAP" \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocessversioned-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_VERSIONED_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocessversioned-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-versioned-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-tls-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_DEP \
    -Wl,-soname,libpolyprocesstls-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesstls-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesstls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-tls-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_DEFAULT_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-tls-default-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_DEFAULT_DEP \
    -Wl,-soname,libpolyprocesstlsdefault-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesstlsdefault-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_TLS_DEFAULT_DEP_MAIN \
    -Wl,-e,_start -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesstlsdefault-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-tls-default-needed-real.elf"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_COPY_DEP \
    -Wl,-soname,libpolyprocesscopy-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyprocesscopy-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fno-pic \
    -no-pie -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_COPY_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyprocesscopy-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-process-copy-reloc-real.elf"
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
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_QSORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-qsort-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_BSEARCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-bsearch-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_QSORT_R_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-qsort-r-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_ONCE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-once-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_KEY_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-key-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_MUTEX_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-mutex-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_SELF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-self-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_RWLOCK_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-rwlock-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_MUTEXATTR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-mutexattr-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_SPIN_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-spin-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PTHREAD_COND_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-pthread-cond-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_TIME_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-time-real.so"
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
    -Wl,-soname,libpolyunique-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_GNU_UNIQUE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyunique-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_GNU_UNIQUE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyunique-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-gnu-unique-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_IFUNC_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ifunc-real.so"
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
    "$POLYCALL_PUTS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-puts-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SNPRINTF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-snprintf-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INTEGER_PARSE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-integer-parse-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CTYPE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ctype-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-abs-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ATOL_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-atol-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FFS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ffs-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRTOD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-strtod-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRTOF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-strtof-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FABSF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fabsf-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_FABS_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-fabs-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SQRTF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sqrtf-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SQRT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-sqrt-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROUNDING_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-rounding-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_STRING_SEARCH_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-string-search-real.so"
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
    "$POLYCALL_CXA_GUARD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxa-guard-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_STATIC_GUARD_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-static-guard-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fno-devirtualize -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycxxvirtual-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_VIRTUAL_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxvirtual-riscv.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fno-devirtualize -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_VIRTUAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxvirtual-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-virtual-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_GLOBAL_DTOR_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-global-dtor-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_FINALIZE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-finalize-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycxxdepdtor-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_DEP_DTOR_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxdepdtor-riscv.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fno-rtti -fno-exceptions \
    -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_DEP_DTOR_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxdepdtor-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-dep-dtor-real.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycxxguard-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CXX_GUARD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycxxguard-riscv.so"
  riscv64-linux-gnu-g++ -O2 -fno-builtin -fPIC -shared \
    -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CXX_GUARD_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycxxguard-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cxx-guard-needed-real.so"
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
    -Wl,-soname,libpolycrossneeded-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossvec-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrosscompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrosscompact-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifunccompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifunccompact-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifuncfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifuncfp64stack-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifuncvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossifuncvec-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossfp64stack-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootvec-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootcompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootcompact-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifunccompact-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifunccompact-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifuncfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifuncfp64stack-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifuncvec-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootifuncvec-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootfp64stack-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossrootfp64stack-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossleaf-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossleaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossleaf-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_LEAF_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossleaf-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossmid-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_MID_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossleaf-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycrossmid-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_TRANSITIVE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycrossmid-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-needed-transitive-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossneeded-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossneeded-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossvec-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrosscompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrosscompact-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifunccompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossifunccompact-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifuncfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossifuncfp64stack-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossifuncvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossifuncvec-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossfp64stack-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootvec-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootcompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootcompact-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifunccompact-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootifunccompact-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifuncfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootifuncfp64stack-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootifuncvec-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootifuncvec-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycrossrootfp64stack-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolycrossrootfp64stack-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossneeded-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossvec-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-vec128-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrosscompact-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-compact-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossifunccompact-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-ifunc-compact-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossifuncfp64stack-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-ifunc-fp64-stack-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossifuncvec-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-ifunc-vec128-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossfp64stack-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-fp64-stack-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_VEC128_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootvec-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-vec128-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_COMPACT_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootcompact-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-compact-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_COMPACT_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootifunccompact-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-ifunc-compact-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_FP64_STACK_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootifuncfp64stack-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-ifunc-fp64-stack-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_IFUNC_VEC128_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootifuncvec-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-ifunc-vec128-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-E -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_CROSS_FP64_STACK_ROOT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolycrossrootfp64stack-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-fp64-stack-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysymbolic-override-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_OVERRIDE_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-override-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysymbolic-target-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysymbolic-target-bsymbolic-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none -Wl,-Bsymbolic \
    "$POLYCALL_SYMBOLIC_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-bsymbolic-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysymbolic-target-protected-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PROTECTED_TARGET_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolysymbolic-target-protected-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-riscv.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-symbolic-preempt-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-riscv.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-bsymbolic-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-symbolic-bind-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SYMBOLIC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolysymbolic-override-riscv.so \
    -Wl,--no-as-needed -l:libpolysymbolic-target-protected-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-symbolic-protected-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,/usr/lib/polyapps/libpolyabsneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyabsneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyabsneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-abs-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreload-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreload-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadneeded-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadoverride-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_OVERRIDE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadoverride-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadoverride-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_OVERRIDE_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/libpolypreloadoverride-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadsecond-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_SECOND_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadsecond-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadchainleaf-riscv.so \
    -Wl,-init,poly_preload_chain_leaf_init \
    -Wl,-fini,poly_preload_chain_leaf_fini \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_CHAIN_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadchainleaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolypreloadchain-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_CHAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolypreloadchainleaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolypreloadchain-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_MAIN_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-preload-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PRELOAD_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolypreloadneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-preload-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,'$ORIGIN/polydeps/libpolyoriginneeded-riscv.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyoriginneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyoriginneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-origin-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,'$ORIGIN/${PLATFORM}/polydeps/libpolyplatformneeded-riscv.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps/libpolyplatformneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps" \
    -Wl,--no-as-needed -l:libpolyplatformneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-platform-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,'$ORIGIN/${LIB}/polydeps/libpolylibneeded-riscv.so' \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolylibneeded-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolylibneeded-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-lib-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyabsrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyabsrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,/usr/lib/polyapps/polydeps \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyabsrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-abs-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-rpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathinherit-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathinherit/libpolyrpathinherit-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathinherit-mid-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathinherit" \
    -Wl,--no-as-needed -l:libpolyrpathinherit-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrpathinherit-mid-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/rpathinherit' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrpathinherit-mid-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-rpath-inherit-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathorigin-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/leaf/libpolyrpathorigin-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathorigin-mid-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/leaf" \
    -Wl,--no-as-needed -l:libpolyrpathorigin-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/mid/libpolyrpathorigin-mid-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/mid:$ORIGIN/leaf' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/mid" \
    -Wl,--no-as-needed -l:libpolyrpathorigin-mid-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathorigin/riscv/riscv-pcall-rpath-origin-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathrunpath-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/rootleaf/libpolyrpathrunpath-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -DPOLY_RPATH_INHERIT_VALUE=1700 \
    -Wl,-soname,libpolyrpathrunpath-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RPATH_INHERIT_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/runleaf/libpolyrpathrunpath-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrpathrunpath-mid-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../runleaf' \
    "$POLYCALL_RPATH_INHERIT_MID_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/runleaf" \
    -Wl,--no-as-needed -l:libpolyrpathrunpath-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/mid/libpolyrpathrunpath-mid-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,--disable-new-dtags -Wl,-rpath,'$ORIGIN/mid:$ORIGIN/rootleaf' \
    "$POLYCALL_RPATH_INHERIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/mid" \
    -Wl,--no-as-needed -l:libpolyrpathrunpath-mid-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/rpathrunpath/riscv/riscv-pcall-rpath-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysonameonce-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SONAME_ONCE_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafa/libpolysonameonce-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -DPOLY_SONAME_ONCE_VALUE=200 \
    -Wl,-soname,libpolysonameonce-leaf-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_SONAME_ONCE_LEAF_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafb/libpolysonameonce-leaf-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysonameonce-a-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../leafa' \
    "$POLYCALL_SONAME_ONCE_DEP_A_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafa" \
    -Wl,--no-as-needed -l:libpolysonameonce-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/a/libpolysonameonce-a-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolysonameonce-b-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/../leafb' \
    "$POLYCALL_SONAME_ONCE_DEP_B_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/leafb" \
    -Wl,--no-as-needed -l:libpolysonameonce-leaf-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/b/libpolysonameonce-b-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/a:$ORIGIN/b' \
    "$POLYCALL_SONAME_ONCE_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/a" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/b" \
    -Wl,--no-as-needed -l:libpolysonameonce-a-riscv.so \
    -Wl,--no-as-needed -l:libpolysonameonce-b-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/riscv-pcall-soname-once-real.so"
  riscv64-linux-gnu-gcc -O2 -fno-builtin -fno-tree-vectorize -fPIC -shared \
    -nostdlib -nodefaultlibs -march=rv64gc -mabi=lp64d \
    -DPOLY_PROCESS_SONAME_ONCE_MAIN \
    -Wl,-e,_start -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/a:$ORIGIN/b' \
    "$POLYEXEC_PROCESS_NEEDED_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/a" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/b" \
    -Wl,--no-as-needed -l:libpolysonameonce-a-riscv.so \
    -Wl,--no-as-needed -l:libpolysonameonce-b-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/sonameonce/riscv/riscv-process-soname-once-real.elf"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycolonrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolycolonrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/missing:$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolycolonrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-colon-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolybracedorigin-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolybracedorigin-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'${ORIGIN}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedorigin-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-braced-origin-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolylibrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolylibrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/$LIB/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolylibrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-lib-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolybracedlibrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps/libpolybracedlibrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/${LIB}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/lib/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedlibrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-braced-lib-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyplatformrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps/libpolyplatformrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/$PLATFORM/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps" \
    -Wl,--no-as-needed -l:libpolyplatformrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-platform-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolybracedplatformrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps/libpolybracedplatformrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/${PLATFORM}/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv/polydeps" \
    -Wl,--no-as-needed -l:libpolybracedplatformrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-braced-platform-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyenvpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/libpolyenvpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps" \
    -Wl,--no-as-needed -l:libpolyenvpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ld-library-path-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyenvplatform-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/riscv/libpolyenvplatform-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/riscv" \
    -Wl,--no-as-needed -l:libpolyenvplatform-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ld-platform-path-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyenvorigin-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envorigin/libpolyenvorigin-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envorigin" \
    -Wl,--no-as-needed -l:libpolyenvorigin-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ld-origin-path-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyenvprefer-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps/libpolyenvprefer-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyenvprefer-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RUNPATH_PREFER_BAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyenvprefer-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/envdeps" \
    -Wl,--no-as-needed -l:libpolyenvprefer-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-ld-prefer-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrelrunpath-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrelrunpath-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,usr/lib/polyapps/polydeps \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrelrunpath-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relative-runpath-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrunpathprefer-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RUNPATH_PREFER_BAD_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrunpathprefer-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrunpathprefer-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ABS_NEEDED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps/libpolyrunpathprefer-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    -Wl,-rpath,'$ORIGIN/polydeps' \
    "$POLYCALL_ABS_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps/polydeps" \
    -Wl,--no-as-needed -l:libpolyrunpathprefer-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-runpath-prefer-real.so"
  for idx in 1 2 3 4 5 6 7 8 9; do
    riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
      -march=rv64g -mabi=lp64d \
      -DPOLY_MANY_NEEDED_INDEX="$idx" \
      -Wl,-soname,libpolymanyneeded"$idx"-riscv.so \
      -Wl,--hash-style=sysv -Wl,--build-id=none \
      "$POLYCALL_MANY_NEEDED_DEP_REAL_SRC" \
      -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolymanyneeded$idx-riscv.so"
  done
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_MANY_NEEDED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolymanyneeded1-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded2-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded3-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded4-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded5-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded6-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded7-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded8-riscv.so \
    -Wl,--no-as-needed -l:libpolymanyneeded9-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-many-needed-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrootdep-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_EXPORT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootdep-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_EXPORT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootdep-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-root-export-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyroottls-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyroottls-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyroottls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-root-tls-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyroottls-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolyroottls-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolyroottls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-tls-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrootifunc-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootifunc-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootifunc-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-root-ifunc-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrootifunc-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolyrootifunc-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolyrootifunc-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-root-ifunc-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyrootweak-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_ROOT_WEAK_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyrootweak-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_ROOT_WEAK_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyrootweak-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-root-weak-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneededtls-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededtls-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededtls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-tls-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_EXTERNAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededtls-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-tls-external-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneededtls-aarch64.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_DEP_REAL_SRC" \
    -o "$TMP_DIR/poly-link/riscv/libpolyneededtls-aarch64.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_TLS_EXTERNAL_MAIN_REAL_SRC" \
    -L"$TMP_DIR/poly-link/riscv" \
    -Wl,--no-as-needed -l:libpolyneededtls-aarch64.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-cross-needed-tls-external-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyversioned-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none -Wl,--version-script="$POLYCALL_VERSIONED_DEP_REAL_MAP" \
    "$POLYCALL_VERSIONED_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyversioned-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_VERSIONED_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyversioned-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-versioned-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneededifunc-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_NEEDED_IFUNC_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededifunc-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_IFUNC_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededifunc-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-ifunc-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneededdtinit-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none -Wl,-init,poly_needed_dt_init \
    -Wl,-fini,poly_needed_dt_fini \
    "$POLYCALL_NEEDED_DT_INIT_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededdtinit-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_DT_INIT_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededdtinit-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-dt-init-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolyneededrelro-riscv.so \
    -Wl,-z,relro -Wl,-z,now \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_RELRO_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyneededrelro-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_NEEDED_RELRO_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyneededrelro-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-needed-relro-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-soname,libpolycopy-riscv.so -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    "$POLYCALL_COPY_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolycopy-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fno-pie -no-pie -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_COPY_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolycopy-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-copy-reloc.elf"
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
    -Wl,-e,poly_entry -Wl,-init,poly_dt_init -Wl,-fini,poly_dt_fini \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_DT_INIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-dt-init-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -pie -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_PREINIT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-preinit-real.elf"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,-z,relro -Wl,-z,now \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_RELRO_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-relro-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -DPOLY_INIT_ORDER_DIGIT=1 \
    -DPOLY_INIT_ORDER_FUNC=poly_init_order_normal_touch \
    -Wl,-soname,libpolyinitnormal-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyinitnormal-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -DPOLY_INIT_ORDER_DIGIT=2 \
    -DPOLY_INIT_ORDER_FUNC=poly_init_order_first_touch \
    -Wl,-z,initfirst -Wl,-soname,libpolyinitfirst-riscv.so \
    -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_DEP_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/libpolyinitfirst-riscv.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_INIT_ORDER_MAIN_REAL_SRC" \
    -L"$TMP_DIR/initramfs-root/usr/lib/polyapps" \
    -Wl,--no-as-needed -l:libpolyinitnormal-riscv.so \
    -Wl,--no-as-needed -l:libpolyinitfirst-riscv.so \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-initfirst-real.so"
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
    "$POLYCALL_X86_FP64_SUM10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp64-sum10-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp64-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FP64_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fp64-callee-stack-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fpair64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR64_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fpair64-fp64-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR32_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fpair32-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_FPAIR32_FP32_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-fpair32-fp32-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_VEC128_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-vec128-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_VEC128_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-vec128-fp64-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sret-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sret-stack-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_STACK10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sret-stack10-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sret-callee-stack-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SRET_FP64_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sret-fp64-callee-stack-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_MIXED_U64_FP64_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-stack-import-real.so"
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
    "$POLYCALL_X86_SUM10_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sum10-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_SUM14_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-sum14-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_ALIGN14_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-align14-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_I128_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-i128-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_I128_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-i128-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_CALLEE_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-callee-import-real.so"
  riscv64-linux-gnu-gcc -O2 -fPIC -shared -nostdlib -nodefaultlibs \
    -march=rv64g -mabi=lp64d \
    -Wl,-e,poly_entry -Wl,--hash-style=sysv -Wl,--build-id=none \
    "$POLYCALL_X86_CALLEE_STACK_IMPORT_REAL_SRC" \
    -o "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-x86-callee-stack-import-real.so"
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-logical.elf" 0xd2801fe1 0xf2a01fe1 0xf2c01fe1 0xf2e01fe1 0xd281e1e2 0xf2a1e1e2 0xf2c1e1e2 0xf2e1e1e2 0x4e080c21 0x4e080c42 0x4e221c23 0x4ea21c24 0x6e221c25 0x4e621c26 0x4ee21c27 0x0e013c60 0x0e013c88 0x8b080000 0x0e013ca8 0x8b080000 0x0e013cc8 0x8b080000 0x0e013ce8 0x8b080000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-addsub.elf" 0xd2800281 0xd2800062 0x4e010c21 0x4e010c42 0x4e228429 0x6e22842a 0x0e22842b 0x2e22842c 0x4e020c23 0x4e020c44 0x4e64846d 0x6e64846e 0x0e64846f 0x2e648470 0x4e040c25 0x4e040c46 0x4ea684b1 0x6ea684b2 0x0ea684b3 0x2ea684b4 0x4e080c27 0x4e080c48 0x4ee884f5 0x6ee884f6 0x0e013d20 0x0e013d57 0x8b170000 0x0e013d77 0x8b170000 0x0e013d97 0x8b170000 0x0e023db7 0x8b170000 0x0e023dd7 0x8b170000 0x0e023df7 0x8b170000 0x0e023e17 0x8b170000 0x0e043e37 0x8b170000 0x0e043e57 0x8b170000 0x0e043e77 0x8b170000 0x0e043e97 0x8b170000 0x4e083eb7 0x8b170000 0x4e083ed7 0x8b170000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-movi.elf" 0x4f00e4a1 0x4f008522 0x4f0005a3 0x6f05e544 0x6f02e6a5 0x0f00e4e6 0x0f008567 0x0f0005e8 0x6f00e409 0x0e013c20 0x0e023c4a 0x8b0a0000 0x0e043c6a 0x8b0a0000 0x0e013c8a 0x8b0a0000 0x0e013caa 0x8b0a0000 0x0e013cca 0x8b0a0000 0x0e023cea 0x8b0a0000 0x0e043d0a 0x8b0a0000 0x4e083d2a 0x8b0a0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-modimm.elf" 0x6f008641 0x2f01a682 0x6f0206c3 0x2f036704 0x4f008405 0x4f009645 0x0f008406 0x0f01b686 0x6f000407 0x6f0216c7 0x2f000408 0x2f031708 0x0e013c20 0x0e013c4a 0x8b0a0000 0x0e013c6a 0x8b0a0000 0x0e013c8a 0x8b0a0000 0x0e013caa 0x8b0a0000 0x0e023cca 0x8b0a0000 0x0e013cea 0x8b0a0000 0x0e013d0a 0x8b0a0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-compare.elf" 0x0f03e7e1 0x0f03e7e2 0x2e228c23 0x0f008444 0x0f008425 0x0e653486 0x0f000467 0x0f000448 0x2ea834e9 0x0f07e60a 0x0f00e60b 0x0e2b8d4c 0x0e013c60 0x0e023ccd 0x8b0d0000 0x0e043d2d 0x8b0d0000 0x0e013d8d 0x8b0d0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-ext.elf" 0x0f00e621 0x0f01e442 0x2e021823 0x4f01e664 0x4f02e485 0x6e054886 0x0e013c60 0x0e0b3c6a 0x8b0a0000 0x0e013cca 0x8b0a0000 0x0e0f3cca 0x8b0a0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-permute.elf" 0x0f00e421 0x0f00e542 0x0e023823 0x0e026824 0x0e021825 0x4f00e466 0x4f00e687 0x4e0778c8 0x4e0758c9 0x4e0728cb 0x0f00848c 0x0f0087cd 0x0e4d398e 0x0f0004af 0x0f010510 0x0e9019f1 0xd28000d2 0x4e080e52 0xd2800653 0x4e080e73 0x4ed36a54 0x0e033c60 0x0e013c8a 0x8b0a0000 0x0e093caa 0x8b0a0000 0x0e033d0a 0x8b0a0000 0x0e113d2a 0x8b0a0000 0x0e1f3d6a 0x8b0a0000 0x0e063dca 0x8b0a0000 0x0e0c3e2a 0x8b0a0000 0x4e183e8a 0x8b0a0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-tbl.elf" 0xd2802001 0xf2a06041 0xf2c0a081 0xf2e0e0c1 0x4e080c21 0x4f01e402 0x0f00e4a3 0x0e030024 0x4f00e523 0x4e030025 0x0f00e663 0x0e032026 0x0f02e5a7 0x0f03e463 0x0e031027 0x4f02e708 0x4f00e483 0x4e031028 0x4f00e54a 0x4f00e68b 0x4f00e7cc 0x4f01e50d 0x4f01e44e 0x4e0e614f 0x0e013c80 0x0e013ca9 0x8b090000 0x0e013cc9 0x8b090000 0x0e013ce9 0x8b090000 0x0e013d09 0x8b090000 0x0e013de9 0x8b090000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-rev.elf" 0xd2802001 0xf2a06041 0xf2c0a081 0xf2e0e0c1 0x4e080c21 0x0e201822 0x2e200823 0x0e200824 0x4e201825 0x6e200826 0x4e200827 0x2e600828 0x0e600829 0x0ea0082a 0x0e013c40 0x0e013c6b 0x8b0b0000 0x0e013c8b 0x8b0b0000 0x0e113cab 0x8b0b0000 0x0e113ccb 0x8b0b0000 0x0e113ceb 0x8b0b0000 0x0e023d0b 0x8b0b0000 0x0e023d2b 0x8b0b0000 0x0e043d4b 0x8b0b0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-simd-reduce.elf" 0xd2900001 0xf2afffe1 0xf2dfffe1 0xf2e00021 0x4e080c21 0x4e30a822 0x6e30a823 0x4e31a824 0x6e31a825 0x4e70a826 0x6e70a827 0x4e71a828 0x6e71a829 0xd280000a 0xf2b0000a 0xf2dfffea 0xf2efffea 0x4e080d4a 0x4eb0a94c 0x6eb0a94d 0x4eb1a94e 0x6eb1a94f 0x0e013c40 0x0e013c6b 0x8b0b0000 0x0e013c8b 0x8b0b0000 0x0e013cab 0x8b0b0000 0x0e023ccb 0x8b0b0000 0x0e023ceb 0x8b0b0000 0x0e023d0b 0x8b0b0000 0x0e023d2b 0x8b0b0000 0x0e043d8b 0x8b0b0000 0x0e043dab 0x8b0b0000 0x0e043dcb 0x8b0b0000 0x0e043deb 0x8b0b0000 0xd65f03c0
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pcall-none-reloc.elf" --dyn-none64 0x7b --export-at poly_entry 4 0xd65f03c0 0xd0000000 0x91000000 0xf9400000 0xd65f03c0
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pair-frame.elf" 0xd2800360 0xd2800101 0xa9bf07e0 0xd2800000 0xd2800001 0xa8c107e0 0x8b010000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-hints.elf" 0xd2800360 0xd503203f 0xd503205f 0xd503207f 0xd503209f 0xd50320bf 0xd503245f 0xd503233f 0xd50323bf 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-prfm.elf" 0xd2800360 0xf98003e0 0xf88003e0 0xf8a06be0 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-strlen.elf" 0xd4200020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memfill.elf" 0xd2800821 0xd2800082 0xd4200040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcmp.elf" 0x91000401 0xd2800082 0xd4200060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-memcpy.elf" 0x91000401 0xd2800082 0xd4200080
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-eventfd2.elf" 0xd2800060 0xd2800001 0xd2800268 0xd4000001 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-init1.elf" 0xd2800000 0xd2800348 0xd4000001 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-add-watch.elf" 0xd2800000 0xd2800348 0xd4000001 0xaa0003e3 0x91001821 0xd2802002 0xd2800368 0xd4000001 0xaa0003e4 0xaa0303e0 0xd2800728 0xd4000001 0xaa0403e0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-inotify-rm-watch.elf" 0xd2800000 0xd2800348 0xd4000001 0xaa0003e3 0x91001821 0xd2802002 0xd2800368 0xd4000001 0xaa0003e1 0xaa0303e0 0xd2800388 0xd4000001 0xaa0003e4 0xaa0303e0 0xd2800728 0xd4000001 0xaa0403e0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-dup3.elf" 0xd2800060 0xd2800001 0xd2800268 0xd4000001 0xaa0003e3 0xd2800101 0xd2800002 0xd2800308 0xd4000001 0xaa0003e4 0xaa0403e0 0xd2800728 0xd4000001 0xaa0303e0 0xd2800728 0xd4000001 0xaa0403e0
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-symlinkat.elf" 0xaa0103e0 0xaa0103e2 0xd2800001 0xd2800488 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-linkat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0x91020023 0xd2800004 0xd28004a8 0xd4000001
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pipe2.elf" 0xaa0103e2 0xaa0103e0 0xd2800001 0xd2800768 0xd4000001 0xb9400043 0xb9400444 0xaa0303e0 0xd2800728 0xd4000001 0xaa0403e0 0xd2800728 0xd4000001 0xaa0303e0
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-create.elf" 0xd2800020 0xd2800001 0xd2800aa8 0xd4000001 0xaa0003e3 0xd2800728 0xd4000001 0xaa0303e0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-settime.elf" 0xaa0103e4 0xd28001a0 0xd2800001 0xaa0403e2 0xaa0403e3 0xd2800ac8 0xd4000001 0xf9400880
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timerfd-gettime.elf" 0xaa0103e2 0xd28001a0 0xaa0203e1 0xd2800ae8 0xd4000001 0xf9400840
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-create.elf" 0xaa0103e2 0xd2800000 0xd2800001 0xd2800d68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-gettime.elf" 0xd28002e0 0xd2800d88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-getoverrun.elf" 0xd28002e0 0xd2800da8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-settime.elf" 0xaa0103e2 0xaa0103e3 0xd28002e0 0xd2800001 0xd2800dc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-timer-delete.elf" 0xd28002e0 0xd2800de8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-read.elf" 0xd2800060 0x91000021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-readv.elf" 0xaa0103e3 0xf9000023 0xd2800084 0xf9000424 0xd2800060 0xd2800022 0xd2800828 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-write.elf" 0xd2800020 0x91000021 0xd28000a2 0xd2800808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-write-zero.elf" 0xd2800020 0xd2800002 0xd2800808 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-writev.elf" 0xaa0103e3 0xf9000023 0xd28000a4 0xf9000424 0xd2800020 0xd2800022 0xd2800848 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pread64.elf" 0xd2800060 0x91000021 0xd2800082 0xd2800003 0xd2800868 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pwrite64.elf" 0xd2800060 0x91000021 0xd28000a2 0xd2800003 0xd2800888 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-preadv.elf" 0xaa0103e3 0xf9000023 0xd2800084 0xf9000424 0xd2800060 0xd2800022 0xd2800003 0xd2800004 0xd28008a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pwritev.elf" 0xaa0103e3 0xf9000023 0xd28000a4 0xf9000424 0xd2800060 0xd2800022 0xd2800003 0xd2800004 0xd28008c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-pselect6.elf" 0xaa0003e4 0xd2800000 0xd2800001 0xd2800002 0xd2800003 0xd2800005 0xd2800908 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ppoll.elf" 0xaa0003e2 0xd2800000 0xd2800001 0xd2800003 0xd2800928 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-create1.elf" 0xd2800000 0xd2800288 0xd4000001 0xaa0003e3 0xaa0303e0 0xd2800728 0xd4000001 0xaa0303e0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-ctl.elf" 0xaa0003e3 0xd2800080 0xd2800021 0xd2800062 0xd28002a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-epoll-pwait.elf" 0xaa0003e1 0xd2800080 0xd2800022 0xd2800003 0xd2800004 0xd2800005 0xd28002c8 0xd4000001
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-bind.elf" 0xd28000a0 0xaa0103e1 0xd2800202 0xd2801908 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-listen.elf" 0xd28000a0 0xd2800021 0xd2801928 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-accept.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2801948 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-connect.elf" 0xd28000a0 0xaa0103e1 0xd2800202 0xd2801968 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsockname.elf" 0xd28000a0 0xaa0103e1 0x91020022 0xd2801988 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpeername.elf" 0xd28000a0 0xaa0103e1 0x91020022 0xd28019a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sendto.elf" 0xd28000a0 0x91000021 0xd28000a2 0xd2800003 0xd2800004 0xd2800005 0xd28019c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-recvfrom.elf" 0xd28000a0 0x91000021 0xd2800082 0xd2800003 0xd2800004 0xd2800005 0xd28019e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setsockopt.elf" 0xaa0103e3 0xd28000a0 0xd2800021 0xd2800042 0xd2800084 0xd2801a08 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsockopt.elf" 0xaa0103e3 0x91020024 0xd28000a0 0xd2800021 0xd2800042 0xd2801a28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-shutdown.elf" 0xd28000a0 0xd2800041 0xd2801a48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-accept4.elf" 0xd28000a0 0xd2800001 0xd2800002 0xd2800003 0xd2801e48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fcntl.elf" 0xd2800060 0xd2800021 0xd2800002 0xd2800328 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-ioctl.elf" 0xd2800020 0xd2800001 0xd2800002 0xd28003a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-faccessat.elf" 0xd2800000 0xaa0103e1 0xd2800002 0xd2800003 0xd2800608 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-readlinkat.elf" 0xd2800000 0xaa0103e2 0xd2800203 0xd28009c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-newfstatat.elf" 0xd2800000 0xaa0103e2 0xd2800003 0xd28009e8 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fstat.elf" 0xd2800060 0xd2800a08 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-statx.elf" 0xd2800000 0xaa0103e4 0xd2800002 0xd2800003 0xd2802468 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-newfstatat.elf" 0xd2800000 0x91008022 0x91001821 0xd2800003 0xd28009e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-fstat0.elf" 0xd2800000 0x91008021 0xd2800a08 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-statx.elf" 0xd2800000 0x91010024 0x91001821 0xd2800002 0xd2802463 0xd2802468 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getdents64.elf" 0xd2800060 0xaa0103e1 0xd2800302 0xd28007a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-lseek.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xd28000e1 0xd2800002 0xd28007c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read.elf" 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0x91002021 0xd2800082 0xd28007e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-openat-read-close.elf" 0x91000026 0xd2800000 0x91000021 0xd2800002 0xd2800708 0xd4000001 0xf90000c0 0x910020c1 0xd2800082 0xd28007e8 0xd4000001 0xf94000c0 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-openat-read-close.elf" 0x91000026 0xd2800000 0x91001821 0xd2800002 0xd2800708 0xd4000001 0xf90000c0 0x910020c1 0xd2800082 0xd28007e8 0xd4000001 0xf94000c0 0xd2800728 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-gettime.elf" 0xd2800000 0x91000021 0xd2800e28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clock-getres.elf" 0xd2800000 0x91000021 0xd2800e48 0xd4000001 0xf9400420
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-clock-getres.elf" 0xd2800000 0x91000021 0xd2800e48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-times.elf" 0x91000020 0xd2801328 0xd4000001 0xd2800000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getpgid.elf" 0xd2800000 0xd2801368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getsid.elf" 0xd2800000 0xd2801388 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrlimit.elf" 0xd2800060 0xd2801468 0xd4000001 0xf9400020
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setrlimit.elf" 0xd2800060 0xd2801488 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrusage.elf" 0xd2800000 0x91000021 0xd28014a8 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getcpu.elf" 0x91000020 0x91002021 0xd2801508 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-gettimeofday.elf" 0x91000020 0xd2800001 0xd2801528 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-gettimeofday.elf" 0x91000020 0xd2800001 0xd2801528 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sysinfo.elf" 0x91000020 0xd2801668 0xd4000001 0xd503201f
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setregid.elf" 0xd2800000 0xd2800001 0xd28011e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setgid.elf" 0xd2800000 0xd2801208 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setreuid.elf" 0xd2800000 0xd2800001 0xd2801228 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setuid.elf" 0xd2800000 0xd2801248 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setresuid.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2801268 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getresuid.elf" 0xaa0103e3 0xaa0303e0 0x91001061 0x91002062 0xd2801288 0xd4000001 0xb9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setresgid.elf" 0xd2800000 0xd2800001 0xd2800002 0xd28012a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getresgid.elf" 0xaa0103e3 0xaa0303e0 0x91001061 0x91002062 0xd28012c8 0xd4000001 0xb9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setfsuid.elf" 0xd2800000 0xd28012e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setfsgid.elf" 0xd2800000 0xd2801308 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getgroups.elf" 0xaa0103e2 0xd2800020 0xaa0203e1 0xd28013c8 0xd4000001 0xd503201f
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-setgroups.elf" 0xd2800000 0xd2800001 0xd28013e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-kill.elf" 0xd2800000 0xd2800001 0xd2801028 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-tkill.elf" 0xd2800000 0xd2800001 0xd2801048 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-tgkill.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2801068 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sigaltstack.elf" 0xd2800000 0xd2801088 0xd4000001 0xb9400820
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rt-sigaction.elf" 0xd2800040 0xd2800001 0xd2800002 0xd2800103 0xd28010c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rt-sigprocmask.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2800103 0xd28010e8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-futex.elf" 0xaa0103e0 0xd2800021 0xd2800022 0xd2800003 0xd2800c48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-madvise.elf" 0xaa0103e0 0xd2820001 0xd2800002 0xd2801d28 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mremap.elf" 0xaa0103e9 0xaa0903e0 0xd2800801 0xd2801002 0xd2800003 0xd2800004 0xd2801b08 0xd4000001 0xcb090000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-clone.elf" 0xd2a00020 0xd2800001 0xd2801b88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-execve.elf" 0xd2801ba8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-membarrier-query.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2802368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-membarrier-cmd.elf" 0xd2800020 0xd2800001 0xd2800002 0xd2802368 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-rseq.elf" 0xaa0103e0 0xd2800401 0xd2800002 0xd2800003 0xd28024a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlock.elf" 0xd2800001 0xd2801c88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-munlock.elf" 0xd2800001 0xd2801ca8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlockall.elf" 0xd2800000 0xd2801cc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-munlockall.elf" 0xd2801ce8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-get-mempolicy.elf" 0xd2801d88 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-mempolicy.elf" 0xd2801da8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-migrate-pages.elf" 0xd2801dc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-move-pages.elf" 0xd2801de8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-seccomp.elf" 0xd28022a8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-bpf.elf" 0xd2802308 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-userfaultfd.elf" 0xd2802348 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mlock2.elf" 0xd2800001 0xd2800002 0xd2802388 0xd4000001
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-tid-address.elf" 0xaa0103e0 0xd2800c08 0xd4000001 0xd37ffc00
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-set-robust-list.elf" 0xaa0103e0 0xd2800301 0xd2800c68 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-get-robust-list.elf" 0xd2800000 0x91002022 0xd2800c88 0xd4000001 0xf9400040
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-prlimit64.elf" 0xaa0103e3 0xd2800000 0xd2800061 0xd2800002 0xd28020a8 0xd4000001 0xf9400060
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-getrandom.elf" 0xaa0103e0 0xd2800081 0xd2800002 0xd28022c8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap.elf" 0xd2800000 0xd2800001 0xd2800002 0xd2800003 0xd2800004 0xd2800005 0xd2801bc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap6.elf" 0xd2800000 0xd2800201 0xd2800062 0xd2800443 0xd28000a4 0xd28000e5 0xd2801bc8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap-store.elf" 0xd2800000 0xd2820001 0xd2800062 0xd2800443 0x92800004 0xd2800005 0xd2801bc8 0xd4000001 0xd28009a1 0xf9000001 0xf9400000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-mmap-real-store.elf" 0xd2800000 0xd2820001 0xd2800062 0xd2800443 0x92800004 0xd2800005 0xd2801bc8 0xd4000001 0xd28009a1 0xf9000001 0xf9400000
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-mprotect.elf" 0xd2800000 0xd2820001 0xd2800062 0xd2800443 0x92800004 0xd2800005 0xd2801bc8 0xd4000001 0xaa0003e6 0xaa0603e0 0xd2820001 0xd2800022 0xd2801c48 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-real-munmap.elf" 0xd2800000 0xd2820001 0xd2800062 0xd2800443 0x92800004 0xd2800005 0xd2801bc8 0xd4000001 0xaa0003e6 0xaa0603e0 0xd2820001 0xd2801ae8 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-sys-brk.elf" 0xd2800000 0xd2801ac8 0xd4000001 0xaa0003e9 0xd2800000 0xd2801ac8 0xd4000001 0xcb090000
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
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fp-softfloat.elf" 0xd2800000 0xd2a7f001 0x1e270020 0xd2a67002 0x1e270041 0x1e212802 0x1e260043 0x92400063 0xf100007f 0x9a9f17e3 0x8b030000 0xd2a02004 0x91000484 0x9e230083 0x1e260065 0xd357fca5 0x92401ca5 0xf1025cbf 0x9a9f17e5 0x8b050000 0x9e630084 0x1e624085 0x1e2600a6 0x924000c6 0xf10000df 0x9a9f17e6 0x8b060000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fp-minmax-nan.elf" 0xd2800000 0xd2a7f001 0x1e270020 0xd2aff802 0x1e270041 0x1e217802 0x1e260043 0x6b01007f 0x9a9f17e3 0x8b030000 0x1e215804 0x1e260085 0x6b0200bf 0x9a9f17e5 0x8b050000 0xd2e7fe06 0x9e6700c6 0xd2efff07 0x9e6700e7 0x1e6778c8 0x9e660109 0xeb06013f 0x9a9f17e9 0x8b090000 0x1e6758ca 0x9e66014b 0xeb07017f 0x9a9f17eb 0x8b0b0000 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-fpcr-fpsr.elf" 0xd2800000 0xd2a01801 0xd51b4401 0xd53b4402 0xeb01005f 0x9a801400 0x52a7f003 0x1e270060 0x52a67804 0x1e270081 0x1e212802 0x1e260045 0x6b0300bf 0x9a801400 0xd53b4426 0x927c00c6 0xf10040df 0x9a801400 0xd2800007 0xd51b4427 0x52800008 0x1e270103 0x1e231804 0xd53b4429 0x927f0129 0xf100093f 0x9a801400 0xd51b4407 0xd53b440a 0xf100015f 0x9a801400 0xd65f03c0
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-uname.elf" 0x91000020 0xd2801408 0xd4000001
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-exit.elf" 0xd28000e0 0xd2800ba8 0xd4000001 0xd2800c60
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-exit-group.elf" 0xd28000e0 0xd2800bc8 0xd4000001 0xd2800c80
  "$POLY_ELF_GEN_BIN" aarch64 "$TMP_DIR/initramfs-root/usr/lib/polyapps/aarch64-svc.elf" 0xd2807fe8 0xd40000e1
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-sdsp.elf" 0xff010113 h:0x456d h:0xe02a 0x00000513 0x00013503 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-hints.elf" 0x01b00513 h:0x0005 h:0x4005 h:0x8006 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-int-move.elf" 0x80000537 0x02a50513 0xf0050553 0xe0050553 0xf20505d3 0xe2058553 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-class.elf" 0xf0000553 0xe0051553 0xf20005d3 0xe20595d3 0x00b50533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp32-to-int.elf" 0x41400537 0xf0050553 0xc0051553 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-csr.elf" 0x00000513 0x00500293 0x00129373 0x001023f3 0x00650533 0x00750533 0x00300293 0x00229373 0x002023f3 0x00650533 0x00750533 0x00302e73 0x01c50533 0x04400293 0x00329373 0x00650533 0x003023f3 0x00750533 0x0013d073 0x00102373 0x00650533 0x0020d073 0x002023f3 0x00750533 0x00301073 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-round.elf" h:0x4501 0x00301073 0x402002b7 0xf0028053 0xc0000353 h:0x951a 0xc0001353 h:0x951a 0xc0002353 h:0x951a 0xc0003353 h:0x951a 0xc0004353 h:0x951a 0x0021d073 0xc0007353 h:0x951a 0x001023f3 h:0x951e 0x00301073 h:0x8082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-arith-round.elf" h:0x4501 0x00301073 0x3f8002b7 0xf0028053 0x338002b7 0xf00280d3 0x00100153 0xe0010353 0x00137313 h:0x951a 0x00103153 0xe0010353 0x00137313 h:0x951a 0x0021d073 0x00107153 0xe0010353 0x00137313 h:0x951a 0x001023f3 h:0x951e 0x00301073 h:0x8082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-cvt-round.elf" h:0x4501 0x00301073 0x010002b7 h:0x0285 0xd2328053 0x401000d3 0xe0008353 0x00137313 h:0x951a 0x401030d3 0xe0008353 0x00137313 h:0x951a 0x0021d073 0x401070d3 0xe0008353 0x00137313 h:0x951a 0xd032f0d3 0xe0008353 0x00137313 h:0x951a 0x001023f3 h:0x951e 0x00301073 h:0x8082
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fp-nan-flags.elf" h:0x4501 0x00301073 0x7fc002b7 0xf0028053 0x3f800337 0xf00300d3 0x28100153 0xe00103d3 0x0063c3b3 0x0013b393 h:0x951e 0x00102e73 0x001e3e13 h:0x9572 0x00301073 0x7fa002b7 0xf0028053 0x28100153 0xe00103d3 0x0063c3b3 0x0013b393 h:0x951e 0x00102e73 0x010e4e13 0x001e3e13 h:0x9572 0x00301073 0x7fc002b7 0xf0028053 0xa01023d3 0x0013b393 h:0x951e 0x00102e73 0x001e3e13 h:0x9572 0x00301073 0xa01013d3 0x0013b393 h:0x951e 0x00102e73 0x010e4e13 0x001e3e13 h:0x9572 0x00301073 h:0x8082
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-zbb.elf" 0xff000513 0x00500593 0x40b57533 0x40b56533 0x40b54533 0x00750513 0x00300593 0x0ab54533 0x00500593 0x0ab55533 0x0ab56533 0x00900593 0x0ab57533 0x60051513 0x60151513 0x60251513 0xfff00513 0x6005151b 0x01050513 0x6015151b 0xfff50513 0x6025151b 0x00400593 0x60b51533 0x60b55533 0x60155513 0x07e50513 0x60451513 0x00150513 0x60451513 0x09050513 0x00c51513 0xfff50513 0x60551513 0x0805453b 0x28755513 0x6b855513 0x03055513 0x00150513 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-zba.elf" 0x00700513 0x00300593 0x20b52533 0x20b54533 0x20b56533 0xfff00513 0x00900593 0x08b5053b 0x02055613 0x00100593 0x20b5253b 0x20b5453b 0x20b5653b 0x0825151b 0x00c50533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-zbs.elf" 0x00000513 0x00300593 0x28b51533 0x28551513 0x48b55633 0x48555693 0x68b51533 0x68551513 0x28251513 0x48b51533 0x48251513 0x28b01733 0x48b71733 0x00c50533 0x00151513 0x00d50533 0x00e50533 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-zicond.elf" 0x00700593 0x00000613 0x0ec5d533 0x0ec5f6b3 0x00500613 0x0ec5d733 0x0ec5f7b3 0x00169693 0x00271713 0x00379793 0x00d50533 0x00e50533 0x00f50533 0x00008067
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pair-frame.elf" 0x01b00513 0x00800593 0xff010113 0x00a13023 0x00b13423 0x00000513 0x00000593 0x00013503 0x00813583 0x00b50533 0x01010113 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-split-load.elf" --split-data64 0x7b 0x00002517 0x00053503 0x00008067
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pcall-none-reloc.elf" --dyn-none64 0x7b --export-at poly_entry 4 0x00008067 0x00002517 0xffc53503 0x00008067
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memfill.elf" 0x05200593 0x00400613 0x00200893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcmp.elf" 0x00150593 0x00400613 0x00300893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-memcpy.elf" 0x00150593 0x00400613 0x00400893 0x00100073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-eventfd2.elf" 0x00300513 0x00000593 0x01300893 0x00000073 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-init1.elf" 0x00000513 0x01a00893 0x00000073 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-add-watch.elf" 0x00000513 0x01a00893 0x00000073 0x00050293 0x00658593 0x10000613 0x01b00893 0x00000073 0x00050313 0x00028513 0x03900893 0x00000073 0x00030513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-inotify-rm-watch.elf" 0x00000513 0x01a00893 0x00000073 0x00050293 0x00658593 0x10000613 0x01b00893 0x00000073 0x00050593 0x00028513 0x01c00893 0x00000073 0x00050313 0x00028513 0x03900893 0x00000073 0x00030513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-dup3.elf" 0x00300513 0x00000593 0x01300893 0x00000073 0x00050293 0x00800593 0x00000613 0x01800893 0x00000073 0x00050313 0x00030513 0x03900893 0x00000073 0x00028513 0x03900893 0x00000073 0x00030513
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mknodat.elf" 0x00000513 0x00000613 0x00000693 0x02100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mkdirat.elf" 0x00000513 0x00000613 0x02200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-unlinkat.elf" 0x00000513 0x00000613 0x02300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-symlinkat.elf" 0x00058513 0x00058613 0x00000593 0x02400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-linkat.elf" 0x00000513 0x00058613 0x00000613 0x08058693 0x00000713 0x02500893 0x00000073
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pipe2.elf" 0x00058613 0x00058513 0x00000593 0x03b00893 0x00000073 0x00062283 0x00462303 0x00028513 0x03900893 0x00000073 0x00030513 0x03900893 0x00000073 0x00028513
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-create.elf" 0x00100513 0x00000593 0x05500893 0x00000073 0x00050293 0x03900893 0x00000073 0x00028513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-settime.elf" 0x00058293 0x00d00513 0x00000593 0x00028613 0x00028693 0x05600893 0x00000073 0x0102b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timerfd-gettime.elf" 0x00058293 0x00d00513 0x00028593 0x05700893 0x00000073 0x0102b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-create.elf" 0x00058613 0x00000513 0x00000593 0x06b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-gettime.elf" 0x01700513 0x06c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-getoverrun.elf" 0x01700513 0x06d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-settime.elf" 0x00058613 0x00058693 0x01700513 0x00000593 0x06e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-timer-delete.elf" 0x01700513 0x06f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-read.elf" 0x00300513 0x00058593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-readv.elf" 0x00058293 0x0055b023 0x00400313 0x0065b423 0x00300513 0x00100613 0x04100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-write.elf" 0x00100513 0x00058593 0x00500613 0x04000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-write-zero.elf" 0x00100513 0x00000613 0x04000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-writev.elf" 0x00058293 0x0055b023 0x00500313 0x0065b423 0x00100513 0x00100613 0x04200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pread64.elf" 0x00300513 0x00058593 0x00400613 0x00000693 0x04300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pwrite64.elf" 0x00300513 0x00058593 0x00500613 0x00000693 0x04400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-preadv.elf" 0x00058293 0x0055b023 0x00400313 0x0065b423 0x00300513 0x00100613 0x00000693 0x00000713 0x04500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pwritev.elf" 0x00058293 0x0055b023 0x00500313 0x0065b423 0x00300513 0x00100613 0x00000693 0x00000713 0x04600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-pselect6.elf" 0x00050713 0x00000513 0x00000593 0x00000613 0x00000693 0x00000793 0x04800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ppoll.elf" 0x00050613 0x00000513 0x00000593 0x00000693 0x04900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-create1.elf" 0x00000513 0x01400893 0x00000073 0x00050293 0x03900893 0x00000073 0x00028513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-ctl.elf" 0x00050693 0x00400513 0x00100593 0x00300613 0x01500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-epoll-pwait.elf" 0x00050593 0x00400513 0x00100613 0x00000693 0x00000713 0x00000793 0x01600893 0x00000073
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-bind.elf" 0x00500513 0x00058593 0x01000613 0x0c800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-listen.elf" 0x00500513 0x00100593 0x0c900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-accept.elf" 0x00500513 0x00000593 0x00000613 0x0ca00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-connect.elf" 0x00500513 0x00058593 0x01000613 0x0cb00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsockname.elf" 0x00500513 0x00058593 0x08058613 0x0cc00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpeername.elf" 0x00500513 0x00058593 0x08058613 0x0cd00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sendto.elf" 0x00500513 0x00058593 0x00500613 0x00000693 0x00000713 0x00000793 0x0ce00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-recvfrom.elf" 0x00500513 0x00058593 0x00400613 0x00000693 0x00000713 0x00000793 0x0cf00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setsockopt.elf" 0x00058693 0x00500513 0x00100593 0x00200613 0x00400713 0x0d000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsockopt.elf" 0x00058693 0x08058713 0x00500513 0x00100593 0x00200613 0x0d100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-shutdown.elf" 0x00500513 0x00200593 0x0d200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-accept4.elf" 0x00500513 0x00000593 0x00000613 0x00000693 0x0f200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fcntl.elf" 0x00300513 0x00100593 0x00000613 0x01900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-ioctl.elf" 0x00100513 0x00000593 0x00000613 0x01d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-faccessat.elf" 0x00000513 0x00000613 0x00000693 0x03000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-readlinkat.elf" 0x00000513 0x00058613 0x01000693 0x04e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-newfstatat.elf" 0x00000513 0x00058613 0x00000693 0x04f00893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-fstat.elf" 0x00300513 0x05000893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-statx.elf" 0x00000513 0x00058713 0x00000613 0x00000693 0x12300893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-newfstatat.elf" 0x02058613 0x00658593 0x00000513 0x00000693 0x04f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-fstat0.elf" 0x02058593 0x00000513 0x05000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-statx.elf" 0x04058713 0x00658593 0x00000513 0x00000613 0x12300693 0x12300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getdents64.elf" 0x00300513 0x00058593 0x01800613 0x03d00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-lseek.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00700593 0x00000613 0x03e00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read.elf" 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00858593 0x00400613 0x03f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-openat-read-close.elf" 0x00058813 0x00000513 0x00058593 0x00000613 0x03800893 0x00000073 0x00a83023 0x00880593 0x00400613 0x03f00893 0x00000073 0x00083503 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-openat-read-close.elf" 0x00058813 0x00000513 0x00658593 0x00000613 0x03800893 0x00000073 0x00a83023 0x00880593 0x00400613 0x03f00893 0x00000073 0x00083503 0x03900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-gettime.elf" 0x00000513 0x00058593 0x07100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clock-getres.elf" 0x00000513 0x00058593 0x07200893 0x00000073 0x0085b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-clock-getres.elf" 0x00000513 0x00058593 0x07200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-times.elf" 0x00058513 0x09900893 0x00000073 0x00000513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getpgid.elf" 0x00000513 0x09b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getsid.elf" 0x00000513 0x09c00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrlimit.elf" 0x00300513 0x0a300893 0x00000073 0x0005b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setrlimit.elf" 0x00300513 0x0a400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrusage.elf" 0x00000513 0x00058593 0x0a500893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getcpu.elf" 0x00058513 0x00858593 0x0a800893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-gettimeofday.elf" 0x00058513 0x00000593 0x0a900893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-gettimeofday.elf" 0x00058513 0x00000593 0x0a900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sysinfo.elf" 0x00058513 0x0b300893 0x00000073 0x00000013
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setregid.elf" 0x00000513 0x00000593 0x08f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setgid.elf" 0x00000513 0x09000893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setreuid.elf" 0x00000513 0x00000593 0x09100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setuid.elf" 0x00000513 0x09200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setresuid.elf" 0x00000513 0x00000593 0x00000613 0x09300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getresuid.elf" 0x00058293 0x00028513 0x00428593 0x00828613 0x09400893 0x00000073 0x0002a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setresgid.elf" 0x00000513 0x00000593 0x00000613 0x09500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getresgid.elf" 0x00058293 0x00028513 0x00428593 0x00828613 0x09600893 0x00000073 0x0002a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setfsuid.elf" 0x00000513 0x09700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setfsgid.elf" 0x00000513 0x09800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getgroups.elf" 0x00058293 0x00100513 0x00028593 0x09e00893 0x00000073 0x00000013
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-setgroups.elf" 0x00000513 0x00000593 0x09f00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-kill.elf" 0x00000513 0x00000593 0x08100893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-tkill.elf" 0x00000513 0x00000593 0x08200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-tgkill.elf" 0x00000513 0x00000593 0x00000613 0x08300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sigaltstack.elf" 0x00000513 0x08400893 0x00000073 0x0085a503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rt-sigaction.elf" 0x00200513 0x00000593 0x00000613 0x00800693 0x08600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rt-sigprocmask.elf" 0x00000513 0x00000593 0x00000613 0x00800693 0x08700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-futex.elf" 0x00058513 0x00100593 0x00100613 0x00000693 0x06200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-madvise.elf" 0x00058513 0x000015b7 0x00000613 0x0e900893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mremap.elf" 0x00058293 0x00028513 0x04000593 0x08000613 0x00000693 0x00000713 0x0d800893 0x00000073 0x40550533
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-clone.elf" 0x00010537 0x00000593 0x0dc00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-execve.elf" 0x0dd00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-membarrier-query.elf" 0x00000513 0x00000593 0x00000613 0x11b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-membarrier-cmd.elf" 0x00100513 0x00000593 0x00000613 0x11b00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-rseq.elf" 0x00058513 0x02000593 0x00000613 0x00000693 0x12500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlock.elf" 0x00000593 0x0e400893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-munlock.elf" 0x00000593 0x0e500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlockall.elf" 0x00000513 0x0e600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-munlockall.elf" 0x0e700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-get-mempolicy.elf" 0x0ec00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-mempolicy.elf" 0x0ed00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-migrate-pages.elf" 0x0ee00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-move-pages.elf" 0x0ef00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-seccomp.elf" 0x11500893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-bpf.elf" 0x11800893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-userfaultfd.elf" 0x11a00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mlock2.elf" 0x00000593 0x00000613 0x11c00893 0x00000073
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-tid-address.elf" 0x00058513 0x06000893 0x00000073 0x03f55513
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-set-robust-list.elf" 0x00058513 0x01800593 0x06300893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-get-robust-list.elf" 0x00000513 0x00858613 0x06400893 0x00000073 0x00063503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-prlimit64.elf" 0x00058693 0x00000513 0x00300593 0x00000613 0x10500893 0x00000073 0x0006b503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-getrandom.elf" 0x00058513 0x00400593 0x00000613 0x11600893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap.elf" 0x00000513 0x00000593 0x00000613 0x00000693 0x00000713 0x00000793 0x0de00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap6.elf" 0x00000513 0x01000593 0x00300613 0x02200693 0x00500713 0x00700793 0x0de00893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap-store.elf" 0x00000513 0x000015b7 0x00300613 0x02200693 0xfff00713 0x00000793 0x0de00893 0x00000073 0x04d00593 0x00b53023 0x00053503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-mmap-real-store.elf" 0x00000513 0x000015b7 0x00300613 0x02200693 0xfff00713 0x00000793 0x0de00893 0x00000073 0x04d00593 0x00b53023 0x00053503
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-mprotect.elf" 0x00000513 0x000015b7 0x00300613 0x02200693 0xfff00713 0x00000793 0x0de00893 0x00000073 0x00050813 0x00080513 0x000015b7 0x00100613 0x0e200893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-real-munmap.elf" 0x00000513 0x000015b7 0x00300613 0x02200693 0xfff00713 0x00000793 0x0de00893 0x00000073 0x00050813 0x00080513 0x000015b7 0x0d700893 0x00000073
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-sys-brk.elf" 0x00000513 0x0d600893 0x00000073 0x00050293 0x00000513 0x0d600893 0x00000073 0x40550533
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
  "$POLY_ELF_GEN_BIN" riscv "$TMP_DIR/initramfs-root/usr/lib/polyapps/riscv-compressed-ebreak.elf" 0x00500893 h:0x9002
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

  local linux_virt_version
  local linux_virt_apk
  local module_path

  prepare_alpine_index
  linux_virt_version="$({
    apk_package_version linux-virt
  } || true)"
  if [[ -z "$linux_virt_version" ]]; then
    echo "Unable to determine Alpine linux-virt version." >&2
    exit 1
  fi

  linux_virt_apk="$CACHE_DIR/linux-virt-$linux_virt_version.apk"
  download \
    "$ALPINE_X86_64_MAIN_URL/linux-virt-$linux_virt_version.apk" \
    "$linux_virt_apk"
  tar -xOzf "$linux_virt_apk" boot/vmlinuz-virt \
    > "$KERNEL_IMAGE" 2>/dev/null
  module_path="$(
    tar -tzf "$linux_virt_apk" 2>/dev/null |
      awk '
        /lib\/modules\/.*\/kernel\/fs\/binfmt_misc\.ko/ && found == 0 {
          print;
          found = 1;
        }
      '
  )"
  if [[ -z "$module_path" ]]; then
    echo "Unable to find binfmt_misc module in $linux_virt_apk." >&2
    exit 1
  fi

  mkdir -p "$TMP_DIR/initramfs-root/lib/modules/poly"
  case "$module_path" in
    *.gz)
      tar -xOzf "$linux_virt_apk" "$module_path" 2>/dev/null |
        gzip -dc > "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko"
      ;;
    *.xz)
      tar -xOzf "$linux_virt_apk" "$module_path" 2>/dev/null |
        xz -dc > "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko"
      ;;
    *.ko)
      tar -xOzf "$linux_virt_apk" "$module_path" 2>/dev/null \
        > "$TMP_DIR/initramfs-root/lib/modules/poly/binfmt_misc.ko"
      ;;
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
  if [[ "$REQUIRE_POLY_REAL_XSAVE" == "1" && -f "$POLY_XCR0_MODULE" ]]; then
    mkdir -p "$TMP_DIR/initramfs-root/lib/modules/poly"
    cp "$POLY_XCR0_MODULE" "$TMP_DIR/initramfs-root/lib/modules/poly/poly_xcr0.ko"
  fi

  cat > "$TMP_DIR/initramfs-root/init" <<EOF
#!/bin/busybox sh
set -eu
RUN_POLY_PROBE="$RUN_POLY_PROBE"
RUN_POLY_APPS="$RUN_POLY_APPS"
RUN_POLY_NEUTRAL="$RUN_POLY_NEUTRAL"
RUN_POLY_EXEC="$RUN_POLY_EXEC"
RUN_POLY_CALL="$RUN_POLY_CALL"
RUN_POLY_THREAD="$RUN_POLY_THREAD"
RUN_POLY_SIGNAL="$RUN_POLY_SIGNAL"
RUN_POLY_BENCH="$RUN_POLY_BENCH"
RUN_POLY_BINFMT="$RUN_POLY_BINFMT"
RUN_POLY_BINFMT_ARCH_TRAPS="$RUN_POLY_BINFMT_ARCH_TRAPS"
RUN_NATIVE_CHECK="$RUN_NATIVE_CHECK"
EXPECT_POLY_CPUID="$EXPECT_POLY_CPUID"
REQUIRE_POLY_REAL_XSAVE="$REQUIRE_POLY_REAL_XSAVE"

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

if [ "$REQUIRE_POLY_REAL_XSAVE" = "1" ]; then
  if [ -f /lib/modules/poly/poly_xcr0.ko ]; then
    insmod /lib/modules/poly/poly_xcr0.ko >/dev/ttyS0 2>&1 || true
  else
    echo "NATIVE_POLY_REAL_XSAVE_MODULE_MISSING" >/dev/ttyS0
  fi
fi

if [ "$RUN_NATIVE_CHECK" = "1" ]; then
  EXPECT_POLY_CPUID="$EXPECT_POLY_CPUID" \
    REQUIRE_POLY_REAL_XSAVE="$REQUIRE_POLY_REAL_XSAVE" \
    /usr/bin/nativecheck.elf >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_PROBE" = "1" ]; then
  /usr/bin/polyprobe >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_APPS" = "1" ]; then
  /usr/bin/polyapp /usr/lib/polyapps/*.poly >/dev/ttyS0 2>&1
  /usr/bin/polyapp \
    /usr/lib/polyapps/aarch64-brk.poly \
    /usr/lib/polyapps/riscv-ebreak.poly >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_NEUTRAL" = "1" ]; then
  /usr/bin/polyapp \
    /usr/lib/polyapps/aarch64-generic-call-riscv.poly \
    /usr/lib/polyapps/aarch64-generic-switch-riscv.poly \
    /usr/lib/polyapps/riscv-generic-call-aarch64.poly \
    /usr/lib/polyapps/riscv-generic-switch-aarch64.poly >/dev/ttyS0 2>&1
  echo "POLY_NEUTRAL_OK" >/dev/ttyS0
fi

if [ "$RUN_POLY_EXEC" = "1" ]; then
    POLYEXEC_TRAP_VECTOR=1 \
    /usr/bin/polyexec \
    /usr/lib/polyapps/aarch64-add.elf=132 \
    /usr/lib/polyapps/aarch64-regadd.elf=123 \
    /usr/lib/polyapps/aarch64-movwide.elf=0xffff6543edcb5678 \
    /usr/lib/polyapps/aarch64-mul.elf=42 \
    /usr/lib/polyapps/aarch64-logical.elf=60 \
    /usr/lib/polyapps/aarch64-shifted.elf=123 \
    /usr/lib/polyapps/aarch64-simd-logical.elf=1005 \
    /usr/lib/polyapps/aarch64-simd-addsub.elf=280 \
    /usr/lib/polyapps/aarch64-simd-movi.elf=315 \
    /usr/lib/polyapps/aarch64-simd-modimm.elf=14550 \
    /usr/lib/polyapps/aarch64-simd-compare.elf=0x1000101fc \
    /usr/lib/polyapps/aarch64-simd-ext.elf=170 \
    /usr/lib/polyapps/aarch64-simd-permute.elf=201 \
    /usr/lib/polyapps/aarch64-simd-tbl.elf=149 \
    /usr/lib/polyapps/aarch64-simd-rev.elf=117837602 \
    /usr/lib/polyapps/aarch64-simd-reduce.elf=8590066171 \
    /usr/lib/polyapps/aarch64-regmix.elf=12 \
    /usr/lib/polyapps/aarch64-branch.elf=42 \
    /usr/lib/polyapps/aarch64-condbranch.elf=91 \
    /usr/lib/polyapps/aarch64-loop.elf=0 \
    /usr/lib/polyapps/aarch64-ret.elf=55 \
    /usr/lib/polyapps/aarch64-mem.elf=77 \
    /usr/lib/polyapps/aarch64-memwidth.elf=0x100001324 \
    /usr/lib/polyapps/aarch64-pair-frame.elf=35 \
    /usr/lib/polyapps/aarch64-hints.elf=27 \
    /usr/lib/polyapps/aarch64-prfm.elf=27 \
    /usr/lib/polyapps/aarch64-pcall-split-load.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-none-reloc.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-eventfd2.elf=0 \
    /usr/lib/polyapps/aarch64-inotify-init1.elf=0 \
    /usr/lib/polyapps/aarch64-inotify-add-watch.elf=1 \
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
    /usr/lib/polyapps/aarch64-umount2.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/aarch64-mount.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/aarch64-pivot-root.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/aarch64-chroot.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/aarch64-renameat2.elf=0 \
    /usr/lib/polyapps/aarch64-open-tree.elf=0xffffffffffffffec \
    /usr/lib/polyapps/aarch64-move-mount.elf=0xffffffffffffffec \
    /usr/lib/polyapps/aarch64-fsopen.elf=0xffffffffffffffed \
    /usr/lib/polyapps/aarch64-fsconfig.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-fsmount.elf=0xfffffffffffffff7 \
    /usr/lib/polyapps/aarch64-fspick.elf=0xffffffffffffffec \
    /usr/lib/polyapps/aarch64-mount-setattr.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-pipe2.elf=4 \
    /usr/lib/polyapps/aarch64-fsync.elf=0 \
    /usr/lib/polyapps/aarch64-fdatasync.elf=0 \
    /usr/lib/polyapps/aarch64-sync-file-range.elf=0 \
    /usr/lib/polyapps/aarch64-fadvise64.elf=0 \
    /usr/lib/polyapps/aarch64-statfs.elf=0x1021994 \
    /usr/lib/polyapps/aarch64-fstatfs.elf=0x1021994 \
    /usr/lib/polyapps/aarch64-truncate.elf=0 \
    /usr/lib/polyapps/aarch64-ftruncate.elf=0 \
    /usr/lib/polyapps/aarch64-fallocate.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-chdir.elf=0 \
    /usr/lib/polyapps/aarch64-fchdir.elf=0 \
    /usr/lib/polyapps/aarch64-fchmod.elf=0 \
    /usr/lib/polyapps/aarch64-fchmodat.elf=0 \
    /usr/lib/polyapps/aarch64-fchownat.elf=0 \
    /usr/lib/polyapps/aarch64-fchown.elf=0 \
    /usr/lib/polyapps/aarch64-timerfd-create.elf=4 \
    /usr/lib/polyapps/aarch64-timerfd-settime.elf=0 \
    /usr/lib/polyapps/aarch64-timerfd-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-timer-create.elf=0 \
    /usr/lib/polyapps/aarch64-timer-gettime.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-timer-getoverrun.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-timer-settime.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-timer-delete.elf=0xffffffffffffffea \
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
    /usr/lib/polyapps/aarch64-socket.elf=4 \
    /usr/lib/polyapps/aarch64-socketpair.elf=4 \
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
    /usr/lib/polyapps/aarch64-ioctl.elf=0xffffffffffffffe7 \
    /usr/lib/polyapps/aarch64-openat.elf=4 \
    /usr/lib/polyapps/aarch64-faccessat.elf=0 \
    /usr/lib/polyapps/aarch64-readlinkat.elf=5 \
    /usr/lib/polyapps/aarch64-newfstatat.elf=0 \
    /usr/lib/polyapps/aarch64-fstat.elf=0 \
    /usr/lib/polyapps/aarch64-statx.elf=0 \
    /usr/lib/polyapps/aarch64-getdents64.elf=24 \
    /usr/lib/polyapps/aarch64-openat-lseek.elf=7 \
    /usr/lib/polyapps/aarch64-openat-read.elf=4 \
    /usr/lib/polyapps/aarch64-openat-read-close.elf=0 \
    /usr/lib/polyapps/aarch64-clock-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-clock-getres.elf=1 \
    /usr/lib/polyapps/aarch64-times.elf=0 \
    /usr/lib/polyapps/aarch64-getpgid.elf=pgid \
    /usr/lib/polyapps/aarch64-getsid.elf=sid \
    /usr/lib/polyapps/aarch64-getrlimit.elf=8388608 \
    /usr/lib/polyapps/aarch64-setrlimit.elf=0 \
    /usr/lib/polyapps/aarch64-getrusage.elf=0 \
    /usr/lib/polyapps/aarch64-getcpu.elf=0 \
    /usr/lib/polyapps/aarch64-gettimeofday.elf=0 \
    /usr/lib/polyapps/aarch64-sysinfo.elf=0 \
    /usr/lib/polyapps/aarch64-capget.elf=0 \
    /usr/lib/polyapps/aarch64-capset.elf=0xffffffffffffffff \
    /usr/lib/polyapps/aarch64-personality.elf=0 \
    /usr/lib/polyapps/aarch64-waitid.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-wait4.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-setpriority.elf=0 \
    /usr/lib/polyapps/aarch64-getpriority.elf=20 \
    /usr/lib/polyapps/aarch64-setpgid.elf=0 \
    /usr/lib/polyapps/aarch64-setsid.elf=0xffffffffffffffff \
    /usr/lib/polyapps/aarch64-umask.elf=18 \
    /usr/lib/polyapps/aarch64-prctl-set-name.elf=0 \
    /usr/lib/polyapps/aarch64-setregid.elf=0 \
    /usr/lib/polyapps/aarch64-setgid.elf=0 \
    /usr/lib/polyapps/aarch64-setreuid.elf=0 \
    /usr/lib/polyapps/aarch64-setuid.elf=0 \
    /usr/lib/polyapps/aarch64-setresuid.elf=0 \
    /usr/lib/polyapps/aarch64-getresuid.elf=0 \
    /usr/lib/polyapps/aarch64-setresgid.elf=0 \
    /usr/lib/polyapps/aarch64-getresgid.elf=0 \
    /usr/lib/polyapps/aarch64-setfsuid.elf=0 \
    /usr/lib/polyapps/aarch64-setfsgid.elf=0 \
    /usr/lib/polyapps/aarch64-getgroups.elf=0 \
    /usr/lib/polyapps/aarch64-setgroups.elf=0 \
    /usr/lib/polyapps/aarch64-kill.elf=0 \
    /usr/lib/polyapps/aarch64-tkill.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-tgkill.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-sigaltstack.elf=2 \
    /usr/lib/polyapps/aarch64-rt-sigaction.elf=0 \
    /usr/lib/polyapps/aarch64-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/aarch64-futex.elf=0 \
    /usr/lib/polyapps/aarch64-madvise.elf=0 \
    /usr/lib/polyapps/aarch64-mremap.elf=0 \
    /usr/lib/polyapps/aarch64-clone.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-execve.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/aarch64-membarrier-query.elf=1023 \
    /usr/lib/polyapps/aarch64-membarrier-cmd.elf=0 \
    /usr/lib/polyapps/aarch64-rseq.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-mlock.elf=0 \
    /usr/lib/polyapps/aarch64-munlock.elf=0 \
    /usr/lib/polyapps/aarch64-mlockall.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-munlockall.elf=0 \
    /usr/lib/polyapps/aarch64-get-mempolicy.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-set-mempolicy.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-migrate-pages.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-move-pages.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-seccomp.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-bpf.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/aarch64-userfaultfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-mlock2.elf=0 \
    /usr/lib/polyapps/aarch64-pkey-mprotect.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pkey-alloc.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pkey-free.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-pidfd-send-signal.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-io-uring-setup.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-io-uring-enter.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-io-uring-register.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-pidfd-open.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-clone3.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/aarch64-close-range.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-openat2.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/aarch64-pidfd-getfd.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-process-madvise.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-landlock-create-ruleset.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-landlock-add-rule.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-landlock-restrict-self.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-process-mrelease.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-futex-waitv.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-set-mempolicy-home-node.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-set-tid-address.elf=0 \
    /usr/lib/polyapps/aarch64-set-robust-list.elf=0 \
    /usr/lib/polyapps/aarch64-get-robust-list.elf=24 \
    /usr/lib/polyapps/aarch64-prlimit64.elf=8388608 \
    /usr/lib/polyapps/aarch64-getrandom.elf=4 \
    /usr/lib/polyapps/aarch64-mmap.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-mmap6.elf=0xffffffffffffffea \
    /usr/lib/polyapps/aarch64-mmap-store.elf=77 \
    /usr/lib/polyapps/aarch64-sys-brk.elf=0 \
    /usr/lib/polyapps/aarch64-munmap.elf=0 \
    /usr/lib/polyapps/aarch64-mprotect.elf=0 \
    /usr/lib/polyapps/aarch64-getpid.elf=pid \
    /usr/lib/polyapps/aarch64-getppid.elf=ppid \
    /usr/lib/polyapps/aarch64-getuid.elf=uid \
    /usr/lib/polyapps/aarch64-geteuid.elf=euid \
    /usr/lib/polyapps/aarch64-getgid.elf=gid \
    /usr/lib/polyapps/aarch64-getegid.elf=egid \
    /usr/lib/polyapps/aarch64-gettid.elf=tid \
    /usr/lib/polyapps/aarch64-getcwd.elf=cwd \
    /usr/lib/polyapps/aarch64-fp-int-move.elf=0x41400000 \
    /usr/lib/polyapps/aarch64-fp32-to-int.elf=12 \
    /usr/lib/polyapps/aarch64-fp32-to-int64.elf=12 \
    /usr/lib/polyapps/aarch64-fp-softfloat.elf=3 \
    /usr/lib/polyapps/aarch64-fp-minmax-nan.elf=4 \
    /usr/lib/polyapps/aarch64-fpcr-fpsr.elf=5 \
    /usr/lib/polyapps/aarch64-uname.elf=0 \
    /usr/lib/polyapps/aarch64-exit.elf=7 \
    /usr/lib/polyapps/aarch64-exit-group.elf=7 \
    /usr/lib/polyapps/aarch64-brk.elf=0x4c000105 \
    /usr/lib/polyapps/aarch64-svc.elf=0xffffffffffffffda \
    /usr/lib/polyapps/aarch64-long.elf=80 \
    /usr/lib/polyapps/riscv-add.elf=27 \
    /usr/lib/polyapps/riscv-compressed.elf=27 \
    /usr/lib/polyapps/riscv-compressed-half.elf=27 \
    /usr/lib/polyapps/riscv-compressed-jalr.elf=27 \
    /usr/lib/polyapps/riscv-compressed-word.elf=27 \
    /usr/lib/polyapps/riscv-compressed-alu.elf=42 \
    /usr/lib/polyapps/riscv-compressed-fp.elf=64 \
    /usr/lib/polyapps/riscv-compressed-sdsp.elf=27 \
    /usr/lib/polyapps/riscv-compressed-hints.elf=27 \
    /usr/lib/polyapps/riscv-fp-int-move.elf=0xffffffff8000002a \
    /usr/lib/polyapps/riscv-fp-class.elf=32 \
    /usr/lib/polyapps/riscv-fp32-to-int.elf=12 \
    /usr/lib/polyapps/riscv-fp-csr.elf=286 \
    /usr/lib/polyapps/riscv-fp-round.elf=16 \
    /usr/lib/polyapps/riscv-fp-arith-round.elf=3 \
    /usr/lib/polyapps/riscv-fp-cvt-round.elf=4 \
    /usr/lib/polyapps/riscv-fp-nan-flags.elf=8 \
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
    /usr/lib/polyapps/riscv-zbb.elf=65536 \
    /usr/lib/polyapps/riscv-zba.elf=2213 \
    /usr/lib/polyapps/riscv-zbs.elf=3 \
    /usr/lib/polyapps/riscv-zicond.elf=42 \
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
    /usr/lib/polyapps/riscv-pair-frame.elf=35 \
    /usr/lib/polyapps/riscv-pcall-split-load.elf=123 \
    /usr/lib/polyapps/riscv-pcall-none-reloc.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-eventfd2.elf=0 \
    /usr/lib/polyapps/riscv-inotify-init1.elf=0 \
    /usr/lib/polyapps/riscv-inotify-add-watch.elf=1 \
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
    /usr/lib/polyapps/riscv-umount2.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/riscv-mount.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/riscv-pivot-root.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/riscv-chroot.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/riscv-renameat2.elf=0 \
    /usr/lib/polyapps/riscv-open-tree.elf=0xffffffffffffffec \
    /usr/lib/polyapps/riscv-move-mount.elf=0xffffffffffffffec \
    /usr/lib/polyapps/riscv-fsopen.elf=0xffffffffffffffed \
    /usr/lib/polyapps/riscv-fsconfig.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-fsmount.elf=0xfffffffffffffff7 \
    /usr/lib/polyapps/riscv-fspick.elf=0xffffffffffffffec \
    /usr/lib/polyapps/riscv-mount-setattr.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-pipe2.elf=4 \
    /usr/lib/polyapps/riscv-fsync.elf=0 \
    /usr/lib/polyapps/riscv-fdatasync.elf=0 \
    /usr/lib/polyapps/riscv-sync-file-range.elf=0 \
    /usr/lib/polyapps/riscv-fadvise64.elf=0 \
    /usr/lib/polyapps/riscv-statfs.elf=0x1021994 \
    /usr/lib/polyapps/riscv-fstatfs.elf=0x1021994 \
    /usr/lib/polyapps/riscv-truncate.elf=0 \
    /usr/lib/polyapps/riscv-ftruncate.elf=0 \
    /usr/lib/polyapps/riscv-fallocate.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-chdir.elf=0 \
    /usr/lib/polyapps/riscv-fchdir.elf=0 \
    /usr/lib/polyapps/riscv-fchmod.elf=0 \
    /usr/lib/polyapps/riscv-fchmodat.elf=0 \
    /usr/lib/polyapps/riscv-fchownat.elf=0 \
    /usr/lib/polyapps/riscv-fchown.elf=0 \
    /usr/lib/polyapps/riscv-timerfd-create.elf=4 \
    /usr/lib/polyapps/riscv-timerfd-settime.elf=0 \
    /usr/lib/polyapps/riscv-timerfd-gettime.elf=0 \
    /usr/lib/polyapps/riscv-timer-create.elf=0 \
    /usr/lib/polyapps/riscv-timer-gettime.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-timer-getoverrun.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-timer-settime.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-timer-delete.elf=0xffffffffffffffea \
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
    /usr/lib/polyapps/riscv-socket.elf=4 \
    /usr/lib/polyapps/riscv-socketpair.elf=4 \
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
    /usr/lib/polyapps/riscv-ioctl.elf=0xffffffffffffffe7 \
    /usr/lib/polyapps/riscv-openat.elf=4 \
    /usr/lib/polyapps/riscv-faccessat.elf=0 \
    /usr/lib/polyapps/riscv-readlinkat.elf=5 \
    /usr/lib/polyapps/riscv-newfstatat.elf=0 \
    /usr/lib/polyapps/riscv-fstat.elf=0 \
    /usr/lib/polyapps/riscv-statx.elf=0 \
    /usr/lib/polyapps/riscv-getdents64.elf=24 \
    /usr/lib/polyapps/riscv-openat-lseek.elf=7 \
    /usr/lib/polyapps/riscv-openat-read.elf=4 \
    /usr/lib/polyapps/riscv-openat-read-close.elf=0 \
    /usr/lib/polyapps/riscv-clock-gettime.elf=0 \
    /usr/lib/polyapps/riscv-clock-getres.elf=1 \
    /usr/lib/polyapps/riscv-times.elf=0 \
    /usr/lib/polyapps/riscv-getpgid.elf=pgid \
    /usr/lib/polyapps/riscv-getsid.elf=sid \
    /usr/lib/polyapps/riscv-getrlimit.elf=8388608 \
    /usr/lib/polyapps/riscv-setrlimit.elf=0 \
    /usr/lib/polyapps/riscv-getrusage.elf=0 \
    /usr/lib/polyapps/riscv-getcpu.elf=0 \
    /usr/lib/polyapps/riscv-gettimeofday.elf=0 \
    /usr/lib/polyapps/riscv-sysinfo.elf=0 \
    /usr/lib/polyapps/riscv-capget.elf=0 \
    /usr/lib/polyapps/riscv-capset.elf=0xffffffffffffffff \
    /usr/lib/polyapps/riscv-personality.elf=0 \
    /usr/lib/polyapps/riscv-waitid.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-wait4.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-setpriority.elf=0 \
    /usr/lib/polyapps/riscv-getpriority.elf=20 \
    /usr/lib/polyapps/riscv-setpgid.elf=0 \
    /usr/lib/polyapps/riscv-setsid.elf=0xffffffffffffffff \
    /usr/lib/polyapps/riscv-umask.elf=18 \
    /usr/lib/polyapps/riscv-prctl-set-name.elf=0 \
    /usr/lib/polyapps/riscv-setregid.elf=0 \
    /usr/lib/polyapps/riscv-setgid.elf=0 \
    /usr/lib/polyapps/riscv-setreuid.elf=0 \
    /usr/lib/polyapps/riscv-setuid.elf=0 \
    /usr/lib/polyapps/riscv-setresuid.elf=0 \
    /usr/lib/polyapps/riscv-getresuid.elf=0 \
    /usr/lib/polyapps/riscv-setresgid.elf=0 \
    /usr/lib/polyapps/riscv-getresgid.elf=0 \
    /usr/lib/polyapps/riscv-setfsuid.elf=0 \
    /usr/lib/polyapps/riscv-setfsgid.elf=0 \
    /usr/lib/polyapps/riscv-getgroups.elf=0 \
    /usr/lib/polyapps/riscv-setgroups.elf=0 \
    /usr/lib/polyapps/riscv-kill.elf=0 \
    /usr/lib/polyapps/riscv-tkill.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-tgkill.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-sigaltstack.elf=2 \
    /usr/lib/polyapps/riscv-rt-sigaction.elf=0 \
    /usr/lib/polyapps/riscv-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/riscv-futex.elf=0 \
    /usr/lib/polyapps/riscv-madvise.elf=0 \
    /usr/lib/polyapps/riscv-mremap.elf=0 \
    /usr/lib/polyapps/riscv-clone.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-execve.elf=0xfffffffffffffffe \
    /usr/lib/polyapps/riscv-membarrier-query.elf=1023 \
    /usr/lib/polyapps/riscv-membarrier-cmd.elf=0 \
    /usr/lib/polyapps/riscv-rseq.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-mlock.elf=0 \
    /usr/lib/polyapps/riscv-munlock.elf=0 \
    /usr/lib/polyapps/riscv-mlockall.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-munlockall.elf=0 \
    /usr/lib/polyapps/riscv-get-mempolicy.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-set-mempolicy.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-migrate-pages.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-move-pages.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-seccomp.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-bpf.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/riscv-userfaultfd.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-mlock2.elf=0 \
    /usr/lib/polyapps/riscv-pkey-mprotect.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pkey-alloc.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pkey-free.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-pidfd-send-signal.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-io-uring-setup.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-io-uring-enter.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-io-uring-register.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-pidfd-open.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-clone3.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/riscv-close-range.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-openat2.elf=0xfffffffffffffff9 \
    /usr/lib/polyapps/riscv-pidfd-getfd.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-process-madvise.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-landlock-create-ruleset.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-landlock-add-rule.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-landlock-restrict-self.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-process-mrelease.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-futex-waitv.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-set-mempolicy-home-node.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-set-tid-address.elf=0 \
    /usr/lib/polyapps/riscv-set-robust-list.elf=0 \
    /usr/lib/polyapps/riscv-get-robust-list.elf=24 \
    /usr/lib/polyapps/riscv-prlimit64.elf=8388608 \
    /usr/lib/polyapps/riscv-getrandom.elf=4 \
    /usr/lib/polyapps/riscv-mmap.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-mmap6.elf=0xffffffffffffffea \
    /usr/lib/polyapps/riscv-mmap-store.elf=77 \
    /usr/lib/polyapps/riscv-sys-brk.elf=0 \
    /usr/lib/polyapps/riscv-munmap.elf=0 \
    /usr/lib/polyapps/riscv-mprotect.elf=0 \
    /usr/lib/polyapps/riscv-getpid.elf=pid \
    /usr/lib/polyapps/riscv-getppid.elf=ppid \
    /usr/lib/polyapps/riscv-getuid.elf=uid \
    /usr/lib/polyapps/riscv-geteuid.elf=euid \
    /usr/lib/polyapps/riscv-getgid.elf=gid \
    /usr/lib/polyapps/riscv-getegid.elf=egid \
    /usr/lib/polyapps/riscv-gettid.elf=tid \
    /usr/lib/polyapps/riscv-getcwd.elf=cwd \
    /usr/lib/polyapps/riscv-uname.elf=0 \
    /usr/lib/polyapps/riscv-exit.elf=7 \
    /usr/lib/polyapps/riscv-exit-group.elf=7 \
    /usr/lib/polyapps/riscv-ebreak.elf=0x4c000205 \
    /usr/lib/polyapps/riscv-compressed-ebreak.elf=0x4c000205 \
    /usr/lib/polyapps/riscv-ecall.elf=0xffffffffffffffda \
    /usr/lib/polyapps/riscv-long.elf=80 >/dev/ttyS0 2>&1
    echo "POLY_EXEC_BLOCK_OK" >/dev/ttyS0 2>&1
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
    /usr/lib/polyapps/aarch64-getpgid.elf=pgid \
    /usr/lib/polyapps/riscv-getpgid.elf=pgid \
    /usr/lib/polyapps/aarch64-getsid.elf=sid \
    /usr/lib/polyapps/riscv-getsid.elf=sid \
    /usr/lib/polyapps/aarch64-getrlimit.elf=stackrlim \
    /usr/lib/polyapps/riscv-getrlimit.elf=stackrlim \
    /usr/lib/polyapps/aarch64-set-tid-address.elf=0 \
    /usr/lib/polyapps/riscv-set-tid-address.elf=0 \
    /usr/lib/polyapps/aarch64-getcwd.elf=cwd \
    /usr/lib/polyapps/riscv-getcwd.elf=cwd \
    /usr/lib/polyapps/aarch64-uname.elf=0 \
    /usr/lib/polyapps/riscv-uname.elf=0 \
    /usr/lib/polyapps/aarch64-clock-gettime.elf=0 \
    /usr/lib/polyapps/riscv-clock-gettime.elf=0 \
    /usr/lib/polyapps/aarch64-clock-getres.elf=clockresnsec \
    /usr/lib/polyapps/riscv-clock-getres.elf=clockresnsec \
    /usr/lib/polyapps/aarch64-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/riscv-rt-sigprocmask.elf=0 \
    /usr/lib/polyapps/aarch64-set-robust-list.elf=0 \
    /usr/lib/polyapps/riscv-set-robust-list.elf=0 \
    /usr/lib/polyapps/aarch64-get-robust-list.elf=24 \
    /usr/lib/polyapps/riscv-get-robust-list.elf=24 \
    /usr/lib/polyapps/aarch64-prlimit64.elf=8388608 \
    /usr/lib/polyapps/riscv-prlimit64.elf=8388608 \
    /usr/lib/polyapps/aarch64-getrandom.elf=4 \
    /usr/lib/polyapps/riscv-getrandom.elf=4 \
    /usr/lib/polyapps/aarch64-mmap-real-store.elf=77 \
    /usr/lib/polyapps/riscv-mmap-real-store.elf=77 \
    /usr/lib/polyapps/aarch64-real-mprotect.elf=0 \
    /usr/lib/polyapps/riscv-real-mprotect.elf=0 \
    /usr/lib/polyapps/aarch64-real-munmap.elf=0 \
    /usr/lib/polyapps/riscv-real-munmap.elf=0 \
    /usr/lib/polyapps/aarch64-real-openat-read-close.elf=0 \
    /usr/lib/polyapps/riscv-real-openat-read-close.elf=0 \
    /usr/lib/polyapps/aarch64-real-newfstatat.elf=0 \
    /usr/lib/polyapps/riscv-real-newfstatat.elf=0 \
    /usr/lib/polyapps/aarch64-real-fstat0.elf=0 \
    /usr/lib/polyapps/riscv-real-fstat0.elf=0 \
    /usr/lib/polyapps/aarch64-real-statx.elf=0 \
    /usr/lib/polyapps/riscv-real-statx.elf=0 \
    /usr/lib/polyapps/aarch64-real-write-zero.elf=0 \
    /usr/lib/polyapps/riscv-real-write-zero.elf=0 \
    /usr/lib/polyapps/aarch64-real-clock-getres.elf=0 \
    /usr/lib/polyapps/riscv-real-clock-getres.elf=0 \
    /usr/lib/polyapps/aarch64-real-gettimeofday.elf=0 \
    /usr/lib/polyapps/riscv-real-gettimeofday.elf=0 \
    /usr/lib/polyapps/aarch64-eventfd2.elf=0 \
    /usr/lib/polyapps/riscv-eventfd2.elf=0 \
    /usr/lib/polyapps/aarch64-inotify-init1.elf=0 \
    /usr/lib/polyapps/riscv-inotify-init1.elf=0 \
    /usr/lib/polyapps/aarch64-inotify-add-watch.elf=1 \
    /usr/lib/polyapps/riscv-inotify-add-watch.elf=1 \
    /usr/lib/polyapps/aarch64-inotify-rm-watch.elf=0 \
    /usr/lib/polyapps/riscv-inotify-rm-watch.elf=0 \
    /usr/lib/polyapps/aarch64-dup3.elf=8 \
    /usr/lib/polyapps/riscv-dup3.elf=8 \
    /usr/lib/polyapps/aarch64-polyexec-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-none-reloc.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/aarch64-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-polyexec-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-polyexec-gnu-hash-rv64gc.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-none-reloc.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynrel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-dynsym.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-rel.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-irelative.elf#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-pcall-rel-jumprel.elf=123 \
    /usr/lib/polyapps/riscv-compressed-ebreak.elf=0x4c000205 >/dev/ttyS0 2>&1
    POLY_PROCESS_ENV=present /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-argv-envp-real.elf=42 \
      alpha beta >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-syscall-real.elf=42 \
      probe >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-reloc-real.elf=42 \
      reloc >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-real.elf=42 \
      needed >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolyprocesspreload-aarch64.so' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload-origin-platform >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-aarch64.so:/usr/lib/polyapps/libpolyprocesspreloadsecond-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload-first-wins >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreloadsecond-aarch64.so:/usr/lib/polyapps/libpolyprocesspreload-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-second-real.elf=42 \
      preload-second-wins >/dev/ttyS0 2>&1
    POLY_LD_LIBRARY_PATH='/usr/lib/polyapps/processenvdeps/\$PLATFORM' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-envpath-real.elf=42 \
      env-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-cross-needed-real.elf=42 \
      cross-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-ifunc-real.elf=42 \
      ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-cross-needed-ifunc-real.elf=42 \
      cross-ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-runpath-real.elf=42 \
      runpath-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-rpath-real.elf=42 \
      rpath-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-transitive-real.elf=42 \
      transitive-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-indirect-real.elf=42 \
      indirect-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-root-export-real.elf=42 \
      root-export-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-root-ifunc-real.elf=42 \
      root-ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-weak-real.elf=42 \
      weak-unresolved >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-weak-needed-real.elf=42 \
      weak-needed-unresolved >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-init-real.elf=42 \
      init-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preinit-real.elf=42 \
      preinit-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-init-needed-real.elf=42 \
      init-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-init-real.elf=42 \
      dt-init >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-real.elf=42 \
      fini-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-exit-group-real.elf=42 \
      fini-exit-group >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-order-real.elf=42 \
      fini-order >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-fini-real.elf=42 \
      dt-fini >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-needed-real.elf=42 \
      fini-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-fini-needed-real.elf=42 \
      dt-fini-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-init-needed-real.elf=42 \
      dt-init-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-versioned-needed-real.elf=42 \
      versioned-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-real.elf=42 \
      tls >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-needed-real.elf=42 \
      tls-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-default-real.elf=42 \
      tls-default >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-default-needed-real.elf=42 \
      tls-default-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-trad-real.elf=42 \
      tls-trad >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-trad-needed-real.elf=42 \
      tls-trad-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-copy-reloc-real.elf=42 \
      copy-reloc >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/sonameonce/aarch64/aarch64-process-soname-once-real.elf=42 \
      soname-once >/dev/ttyS0 2>&1
    POLY_PROCESS_ENV=present /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-argv-envp-real.elf=42 \
      alpha beta >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-syscall-real.elf=42 \
      probe >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-reloc-real.elf=42 \
      reloc >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-real.elf=42 \
      needed >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolyprocesspreload-riscv.so' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload-origin-platform >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-riscv.so:/usr/lib/polyapps/libpolyprocesspreloadsecond-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload-first-wins >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreloadsecond-riscv.so:/usr/lib/polyapps/libpolyprocesspreload-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-second-real.elf=42 \
      preload-second-wins >/dev/ttyS0 2>&1
    POLY_LD_LIBRARY_PATH='/usr/lib/polyapps/processenvdeps/\$PLATFORM' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-envpath-real.elf=42 \
      env-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-cross-needed-real.elf=42 \
      cross-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-ifunc-real.elf=42 \
      ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-cross-needed-ifunc-real.elf=42 \
      cross-ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-runpath-real.elf=42 \
      runpath-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-rpath-real.elf=42 \
      rpath-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-transitive-real.elf=42 \
      transitive-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-indirect-real.elf=42 \
      indirect-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-root-export-real.elf=42 \
      root-export-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-root-ifunc-real.elf=42 \
      root-ifunc-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-weak-real.elf=42 \
      weak-unresolved >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-weak-needed-real.elf=42 \
      weak-needed-unresolved >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-init-real.elf=42 \
      init-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preinit-real.elf=42 \
      preinit-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-init-needed-real.elf=42 \
      init-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-init-real.elf=42 \
      dt-init >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-real.elf=42 \
      fini-array >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-exit-group-real.elf=42 \
      fini-exit-group >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-order-real.elf=42 \
      fini-order >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-fini-real.elf=42 \
      dt-fini >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-needed-real.elf=42 \
      fini-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-fini-needed-real.elf=42 \
      dt-fini-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-init-needed-real.elf=42 \
      dt-init-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-versioned-needed-real.elf=42 \
      versioned-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-real.elf=42 \
      tls >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-needed-real.elf=42 \
      tls-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-default-real.elf=42 \
      tls-default >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-default-needed-real.elf=42 \
      tls-default-needed >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-copy-reloc-real.elf=42 \
      copy-reloc >/dev/ttyS0 2>&1
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/sonameonce/riscv/riscv-process-soname-once-real.elf=42 \
      soname-once >/dev/ttyS0 2>&1
    echo "POLY_ARCH_TRAP_EXEC_OK" >/dev/ttyS0 2>&1
fi

if [ "$RUN_POLY_CALL" = "1" ]; then
    LD_LIBRARY_PATH='/usr/lib/polyapps/missing-envdeps:\$ORIGIN/envorigin:/usr/lib/polyapps/envdeps:/usr/lib/polyapps/envdeps/\$PLATFORM' /usr/bin/polycall \
    /usr/lib/polyapps/aarch64-pcall-sum.elf=21 \
    /usr/lib/polyapps/riscv-pcall-sum.elf=21 \
    sigregs:/usr/lib/polyapps/aarch64-pcall-sum.elf=21 \
    sigregs:/usr/lib/polyapps/riscv-pcall-sum.elf=21 \
    /usr/lib/polyapps/aarch64-pcall-sum8.elf=36 \
    /usr/lib/polyapps/riscv-pcall-sum8.elf=36 \
    /usr/lib/polyapps/aarch64-pcall-sum9.elf=45 \
    /usr/lib/polyapps/aarch64-pcall-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-gnu-hash-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-state.so#poly_entry=83 \
    /usr/lib/polyapps/aarch64-pcall-import-real.so#poly_entry=145 \
    /usr/lib/polyapps/aarch64-pcall-libc-import-real.so#poly_entry=201721 \
    /usr/lib/polyapps/aarch64-pcall-qsort-real.so#poly_entry=128 \
    /usr/lib/polyapps/aarch64-pcall-bsearch-real.so#poly_entry=136 \
    /usr/lib/polyapps/aarch64-pcall-qsort-r-real.so#poly_entry=194 \
    /usr/lib/polyapps/aarch64-pcall-pthread-once-real.so#poly_entry=77 \
    /usr/lib/polyapps/aarch64-pcall-pthread-key-real.so#poly_entry=91 \
    /usr/lib/polyapps/aarch64-pcall-pthread-mutex-real.so#poly_entry=98 \
    /usr/lib/polyapps/aarch64-pcall-pthread-self-real.so#poly_entry=109 \
    /usr/lib/polyapps/aarch64-pcall-pthread-rwlock-real.so#poly_entry=123 \
    /usr/lib/polyapps/aarch64-pcall-pthread-mutexattr-real.so#poly_entry=132 \
    /usr/lib/polyapps/aarch64-pcall-pthread-spin-real.so#poly_entry=141 \
    /usr/lib/polyapps/aarch64-pcall-pthread-cond-real.so#poly_entry=152 \
    /usr/lib/polyapps/aarch64-pcall-time-real.so#poly_entry=176 \
    /usr/lib/polyapps/aarch64-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/aarch64-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/aarch64-pcall-gnu-unique-real.so#poly_entry=745 \
    /usr/lib/polyapps/aarch64-pcall-ifunc-real.so#poly_entry=745 \
    /usr/lib/polyapps/aarch64-pcall-stack-protector-real.so#poly_entry=49 \
    /usr/lib/polyapps/aarch64-pcall-errno-real.so#poly_entry=29 \
    /usr/lib/polyapps/aarch64-pcall-getauxval-real.so#poly_entry=45 \
    /usr/lib/polyapps/aarch64-pcall-getpagesize-real.so#poly_entry=4141 \
    /usr/lib/polyapps/aarch64-pcall-sysconf-real.so#poly_entry=4141 \
    /usr/lib/polyapps/aarch64-pcall-env-real.so#poly_entry=53 \
    /usr/lib/polyapps/aarch64-pcall-puts-real.so#poly_entry=145 \
    /usr/lib/polyapps/aarch64-pcall-snprintf-real.so#poly_entry=706 \
    /usr/lib/polyapps/aarch64-pcall-integer-parse-real.so#poly_entry=1508 \
    /usr/lib/polyapps/aarch64-pcall-ctype-real.so#poly_entry=1540 \
    /usr/lib/polyapps/aarch64-pcall-abs-real.so#poly_entry=1524 \
    /usr/lib/polyapps/aarch64-pcall-atol-real.so#poly_entry=1532 \
    /usr/lib/polyapps/aarch64-pcall-ffs-real.so#poly_entry=1564 \
    /usr/lib/polyapps/aarch64-pcall-strtod-real.so#poly_entry=1376 \
    /usr/lib/polyapps/aarch64-pcall-strtof-real.so#poly_entry=1384 \
    /usr/lib/polyapps/aarch64-pcall-fabsf-real.so#poly_entry=1396 \
    /usr/lib/polyapps/aarch64-pcall-fabs-real.so#poly_entry=1404 \
    /usr/lib/polyapps/aarch64-pcall-sqrtf-real.so#poly_entry=1420 \
    /usr/lib/polyapps/aarch64-pcall-sqrt-real.so#poly_entry=1412 \
    /usr/lib/polyapps/aarch64-pcall-rounding-real.so#poly_entry=1496 \
    /usr/lib/polyapps/aarch64-pcall-string-search-real.so#poly_entry=1468 \
    /usr/lib/polyapps/aarch64-pcall-alloc-real.so#poly_entry=90 \
    /usr/lib/polyapps/aarch64-pcall-strdup-real.so#poly_entry=911 \
    /usr/lib/polyapps/aarch64-pcall-aligned-alloc-real.so#poly_entry=177 \
    /usr/lib/polyapps/aarch64-pcall-atexit-real.so#poly_entry=1122 \
    /usr/lib/polyapps/aarch64-pcall-cxa-guard-real.so#poly_entry=101 \
    /usr/lib/polyapps/aarch64-pcall-cxx-static-guard-real.so#poly_entry=113 \
    /usr/lib/polyapps/aarch64-pcall-cxx-virtual-real.so#poly_entry=246 \
    fini:/usr/lib/polyapps/aarch64-pcall-cxx-global-dtor-real.so#poly_entry=2345 \
    /usr/lib/polyapps/aarch64-pcall-cxx-finalize-real.so#poly_entry=3445 \
    /usr/lib/polyapps/aarch64-pcall-cxx-dep-dtor-real.so#poly_entry=868 \
    depfini:/usr/lib/polyapps/aarch64-pcall-cxx-dep-dtor-real.so#poly_entry=3420 \
    /usr/lib/polyapps/aarch64-pcall-cxx-guard-needed-real.so#poly_entry=219 \
    /usr/lib/polyapps/aarch64-pcall-process-real.so#poly_entry=16771 \
    /usr/lib/polyapps/aarch64-pcall-needed-real.so#poly_entry=397 \
    depfini:/usr/lib/polyapps/aarch64-pcall-needed-real.so#poly_entry=103 \
    /usr/lib/polyapps/aarch64-pcall-cross-needed-real.so#poly_entry=536 \
    depfini:/usr/lib/polyapps/aarch64-pcall-cross-needed-real.so#poly_entry=0x4158000000000056 \
    /usr/lib/polyapps/aarch64-pcall-cross-needed-transitive-real.so#poly_entry=1770 \
    /usr/lib/polyapps/aarch64-pcall-cross-compact-real.so#poly_entry=0x426d00d642020070 \
    /usr/lib/polyapps/aarch64-pcall-cross-ifunc-compact-real.so#poly_entry=0x431f020242a50138 \
    /usr/lib/polyapps/aarch64-pcall-cross-ifunc-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/aarch64-pcall-cross-ifunc-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/aarch64-pcall-cross-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/aarch64-pcall-cross-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/aarch64-pcall-cross-root-compact-real.so#poly_entry=0x415c019d41b0013d \
    /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-compact-real.so#poly_entry=0x4355026d42d7019f \
    /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/aarch64-pcall-cross-root-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/aarch64-pcall-cross-root-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/aarch64-pcall-symbolic-preempt-real.so#poly_entry=1005 \
    /usr/lib/polyapps/aarch64-pcall-symbolic-bind-real.so#poly_entry=15 \
    /usr/lib/polyapps/aarch64-pcall-symbolic-protected-real.so#poly_entry=15 \
    /usr/lib/polyapps/aarch64-pcall-abs-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-origin-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-platform-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-lib-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-abs-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-rpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-rpath-inherit-real.so#poly_entry=745 \
    /usr/lib/polyapps/rpathorigin/aarch64/aarch64-pcall-rpath-origin-real.so#poly_entry=745 \
    /usr/lib/polyapps/rpathrunpath/aarch64/aarch64-pcall-rpath-runpath-real.so#poly_entry=1745 \
    /usr/lib/polyapps/sonameonce/aarch64/aarch64-pcall-soname-once-real.so#poly_entry=235 \
    /usr/lib/polyapps/aarch64-pcall-colon-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-braced-origin-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-lib-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-braced-lib-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-platform-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-braced-platform-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-ld-library-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-ld-platform-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-ld-origin-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-ld-prefer-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-relative-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-runpath-prefer-real.so#poly_entry=945 \
    /usr/lib/polyapps/aarch64-pcall-many-needed-real.so#poly_entry=4545 \
    /usr/lib/polyapps/aarch64-pcall-root-export-real.so#poly_entry=1345 \
    /usr/lib/polyapps/aarch64-pcall-root-tls-real.so#poly_entry=2348 \
    /usr/lib/polyapps/aarch64-pcall-cross-root-tls-real.so#poly_entry=2348 \
    /usr/lib/polyapps/aarch64-pcall-root-ifunc-real.so#poly_entry=2045 \
    /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-real.so#poly_entry=2045 \
    /usr/lib/polyapps/aarch64-pcall-root-weak-real.so#poly_entry=955 \
    /usr/lib/polyapps/aarch64-pcall-needed-tls-real.so#poly_entry=545 \
    repeat:/usr/lib/polyapps/aarch64-pcall-needed-tls-real.so#poly_entry=548 \
    /usr/lib/polyapps/aarch64-pcall-needed-tls-external-real.so#poly_entry=1045 \
    /usr/lib/polyapps/aarch64-pcall-cross-needed-tls-external-real.so#poly_entry=1045 \
    /usr/lib/polyapps/aarch64-pcall-versioned-real.so#poly_entry=1045 \
    /usr/lib/polyapps/aarch64-pcall-needed-ifunc-real.so#poly_entry=845 \
    /usr/lib/polyapps/aarch64-pcall-needed-dt-init-real.so#poly_entry=945 \
    depfini:/usr/lib/polyapps/aarch64-pcall-needed-dt-init-real.so#poly_entry=1945 \
    /usr/lib/polyapps/aarch64-pcall-needed-relro-real.so#poly_entry=745 \
    /usr/lib/polyapps/aarch64-pcall-copy-reloc.elf#poly_entry=78 \
    /usr/lib/polyapps/aarch64-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/aarch64-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/aarch64-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/aarch64-pcall-ctor-real.so#poly_entry=245 \
    fini:/usr/lib/polyapps/aarch64-pcall-fini-real.so#poly_entry=1145 \
    /usr/lib/polyapps/aarch64-pcall-dt-init-real.so#poly_entry=345 \
    fini:/usr/lib/polyapps/aarch64-pcall-dt-init-real.so#poly_entry=1345 \
    /usr/lib/polyapps/aarch64-pcall-preinit-real.elf#poly_entry=345 \
    /usr/lib/polyapps/aarch64-pcall-relro-real.so#poly_entry=745 \
    /usr/lib/polyapps/aarch64-pcall-initfirst-real.so#poly_entry=21 \
    /usr/lib/polyapps/aarch64-pcall-tls-real.so#poly_entry=55 \
    repeat:/usr/lib/polyapps/aarch64-pcall-tls-real.so#poly_entry=100 \
    /usr/lib/polyapps/aarch64-pcall-tls-trad-real.so#poly_entry=55 \
    repeat:/usr/lib/polyapps/aarch64-pcall-tls-trad-real.so#poly_entry=100 \
    /usr/lib/polyapps/aarch64-pcall-tls-ie-real.so#poly_entry=55 \
    repeat:/usr/lib/polyapps/aarch64-pcall-tls-ie-real.so#poly_entry=100 \
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
    sigregs-fp64:/usr/lib/polyapps/aarch64-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
    fpair:/usr/lib/polyapps/aarch64-pcall-fpair-real.so#poly_entry=0x40268000400b0000 \
    hfa3f64:/usr/lib/polyapps/aarch64-pcall-hfa3-real.so#poly_entry=0x0000400e40154016 \
    hfa4f64:/usr/lib/polyapps/aarch64-pcall-hfa4-real.so#poly_entry=0x400e401540164021 \
    hfa3f32:/usr/lib/polyapps/aarch64-pcall-hfa3-f32-real.so#poly_entry=0x0000407040a840b0 \
    hfa4f32:/usr/lib/polyapps/aarch64-pcall-hfa4-f32-real.so#poly_entry=0x407040a840b0410c \
    hfa3f64arg:/usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so#poly_hfa3_f64_arg=0x00003ff840024008 \
    hfa4f64arg:/usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so#poly_hfa4_f64_arg=0x3ff8400240084010 \
    hfa3f32arg:/usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so#poly_hfa3_f32_arg=0x00003fc040104040 \
    hfa4f32arg:/usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so#poly_hfa4_f32_arg=0x3fc0401040404080 \
    fpair32:/usr/lib/polyapps/aarch64-pcall-fpair32-real.so#poly_entry=0x40d8000040700000 \
    fpairarg:/usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so#poly_entry=0x4026800000000000 \
    fpair32arg:/usr/lib/polyapps/aarch64-pcall-fpair32-arg-real.so#poly_entry=0x41340000 \
    vec128u32:/usr/lib/polyapps/aarch64-pcall-vec128-real.so#poly_entry=0x002c00210016000b \
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
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fp64-sum10-import-real.so#poly_entry=0x4070040000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-import-real.so#poly_entry=0x40993e0000000000 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-stack-import-real.so#poly_entry=0x409a0e0000000000 \
    fpair:/usr/lib/polyapps/aarch64-pcall-x86-fpair64-import-real.so#poly_entry=0x405ad0004069e800 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-fpair64-fp64-callee-import-real.so#poly_entry=0x409ae50000000000 \
    fpair32:/usr/lib/polyapps/aarch64-pcall-x86-fpair32-import-real.so#poly_entry=0x41da0000418a0000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-x86-fpair32-fp32-callee-import-real.so#poly_entry=0x44b56800 \
    vec128u32:/usr/lib/polyapps/aarch64-pcall-x86-vec128-import-real.so#poly_entry=0x01e4016b00f20079 \
    vec128u32:/usr/lib/polyapps/aarch64-pcall-x86-vec128-fp64-callee-import-real.so#poly_entry=0x0208018f0116009d \
    /usr/lib/polyapps/aarch64-pcall-x86-sret-import-real.so#poly_entry=0x000b00160021002e \
    /usr/lib/polyapps/aarch64-pcall-x86-sret-stack-import-real.so#poly_entry=0x000b001600210042 \
    sret10:/usr/lib/polyapps/aarch64-pcall-x86-sret-stack10-import-real.so#poly_entry=0x000b00160021008c \
    /usr/lib/polyapps/aarch64-pcall-x86-sret-callee-stack-import-real.so#poly_entry=7356 \
    fp64:/usr/lib/polyapps/aarch64-pcall-x86-sret-fp64-callee-stack-import-real.so#poly_entry=0x4099450000000000 \
    mixedargs:/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-import-real.so#poly_entry=0x406aa80000000000 \
    mixedargs:/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-callee-import-real.so#poly_entry=0x40b2f1c000000000 \
    mixedstack:/usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-stack-import-real.so#poly_entry=0x40788c0000000000 \
    fp32:/usr/lib/polyapps/aarch64-pcall-x86-fp32-import-real.so#poly_entry=0x434f4000 \
    /usr/lib/polyapps/aarch64-pcall-x86-sum8-import-real.so#poly_entry=236 \
    /usr/lib/polyapps/aarch64-pcall-x86-sum10-import-real.so#poly_entry=255 \
    /usr/lib/polyapps/aarch64-pcall-x86-sum14-import-real.so#poly_entry=305 \
    /usr/lib/polyapps/aarch64-pcall-x86-align14-import-real.so#poly_entry=305 \
    /usr/lib/polyapps/aarch64-pcall-x86-i128-import-real.so#poly_entry=0x22421121 \
    /usr/lib/polyapps/aarch64-pcall-x86-i128-callee-import-real.so#poly_entry=7201 \
    /usr/lib/polyapps/aarch64-pcall-x86-callee-import-real.so#poly_entry=7386 \
    /usr/lib/polyapps/aarch64-pcall-x86-callee-stack-import-real.so#poly_entry=7455 \
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
    /usr/lib/polyapps/riscv-pcall-libc-import-real.so#poly_entry=201721 \
    /usr/lib/polyapps/riscv-pcall-qsort-real.so#poly_entry=128 \
    /usr/lib/polyapps/riscv-pcall-bsearch-real.so#poly_entry=136 \
    /usr/lib/polyapps/riscv-pcall-qsort-r-real.so#poly_entry=194 \
    /usr/lib/polyapps/riscv-pcall-pthread-once-real.so#poly_entry=77 \
    /usr/lib/polyapps/riscv-pcall-pthread-key-real.so#poly_entry=91 \
    /usr/lib/polyapps/riscv-pcall-pthread-mutex-real.so#poly_entry=98 \
    /usr/lib/polyapps/riscv-pcall-pthread-self-real.so#poly_entry=109 \
    /usr/lib/polyapps/riscv-pcall-pthread-rwlock-real.so#poly_entry=123 \
    /usr/lib/polyapps/riscv-pcall-pthread-mutexattr-real.so#poly_entry=132 \
    /usr/lib/polyapps/riscv-pcall-pthread-spin-real.so#poly_entry=141 \
    /usr/lib/polyapps/riscv-pcall-pthread-cond-real.so#poly_entry=152 \
    /usr/lib/polyapps/riscv-pcall-time-real.so#poly_entry=176 \
    /usr/lib/polyapps/riscv-pcall-import-value-real.so#poly_entry=168 \
    /usr/lib/polyapps/riscv-pcall-weak-import-real.so#poly_entry=8 \
    /usr/lib/polyapps/riscv-pcall-gnu-unique-real.so#poly_entry=745 \
    /usr/lib/polyapps/riscv-pcall-ifunc-real.so#poly_entry=745 \
    /usr/lib/polyapps/riscv-pcall-stack-protector-real.so#poly_entry=49 \
    /usr/lib/polyapps/riscv-pcall-errno-real.so#poly_entry=29 \
    /usr/lib/polyapps/riscv-pcall-getauxval-real.so#poly_entry=45 \
    /usr/lib/polyapps/riscv-pcall-getpagesize-real.so#poly_entry=4141 \
    /usr/lib/polyapps/riscv-pcall-sysconf-real.so#poly_entry=4141 \
    /usr/lib/polyapps/riscv-pcall-env-real.so#poly_entry=53 \
    /usr/lib/polyapps/riscv-pcall-puts-real.so#poly_entry=145 \
    /usr/lib/polyapps/riscv-pcall-snprintf-real.so#poly_entry=706 \
    /usr/lib/polyapps/riscv-pcall-integer-parse-real.so#poly_entry=1508 \
    /usr/lib/polyapps/riscv-pcall-ctype-real.so#poly_entry=1540 \
    /usr/lib/polyapps/riscv-pcall-abs-real.so#poly_entry=1524 \
    /usr/lib/polyapps/riscv-pcall-atol-real.so#poly_entry=1532 \
    /usr/lib/polyapps/riscv-pcall-ffs-real.so#poly_entry=1564 \
    /usr/lib/polyapps/riscv-pcall-strtod-real.so#poly_entry=1376 \
    /usr/lib/polyapps/riscv-pcall-strtof-real.so#poly_entry=1384 \
    /usr/lib/polyapps/riscv-pcall-fabsf-real.so#poly_entry=1396 \
    /usr/lib/polyapps/riscv-pcall-fabs-real.so#poly_entry=1404 \
    /usr/lib/polyapps/riscv-pcall-sqrtf-real.so#poly_entry=1420 \
    /usr/lib/polyapps/riscv-pcall-sqrt-real.so#poly_entry=1412 \
    /usr/lib/polyapps/riscv-pcall-rounding-real.so#poly_entry=1496 \
    /usr/lib/polyapps/riscv-pcall-string-search-real.so#poly_entry=1468 \
    /usr/lib/polyapps/riscv-pcall-alloc-real.so#poly_entry=90 \
    /usr/lib/polyapps/riscv-pcall-strdup-real.so#poly_entry=911 \
    /usr/lib/polyapps/riscv-pcall-aligned-alloc-real.so#poly_entry=177 \
    /usr/lib/polyapps/riscv-pcall-atexit-real.so#poly_entry=1122 \
    /usr/lib/polyapps/riscv-pcall-cxa-guard-real.so#poly_entry=101 \
    /usr/lib/polyapps/riscv-pcall-cxx-static-guard-real.so#poly_entry=113 \
    /usr/lib/polyapps/riscv-pcall-cxx-virtual-real.so#poly_entry=246 \
    fini:/usr/lib/polyapps/riscv-pcall-cxx-global-dtor-real.so#poly_entry=2345 \
    /usr/lib/polyapps/riscv-pcall-cxx-finalize-real.so#poly_entry=3445 \
    /usr/lib/polyapps/riscv-pcall-cxx-dep-dtor-real.so#poly_entry=868 \
    depfini:/usr/lib/polyapps/riscv-pcall-cxx-dep-dtor-real.so#poly_entry=3420 \
    /usr/lib/polyapps/riscv-pcall-cxx-guard-needed-real.so#poly_entry=219 \
    /usr/lib/polyapps/riscv-pcall-process-real.so#poly_entry=16771 \
    /usr/lib/polyapps/riscv-pcall-needed-real.so#poly_entry=397 \
    depfini:/usr/lib/polyapps/riscv-pcall-needed-real.so#poly_entry=103 \
    /usr/lib/polyapps/riscv-pcall-cross-needed-real.so#poly_entry=536 \
    depfini:/usr/lib/polyapps/riscv-pcall-cross-needed-real.so#poly_entry=0x4158000000000056 \
    /usr/lib/polyapps/riscv-pcall-cross-needed-transitive-real.so#poly_entry=1770 \
    /usr/lib/polyapps/riscv-pcall-cross-compact-real.so#poly_entry=0x426d00d642020070 \
    /usr/lib/polyapps/riscv-pcall-cross-ifunc-compact-real.so#poly_entry=0x431f020242a50138 \
    /usr/lib/polyapps/riscv-pcall-cross-ifunc-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/riscv-pcall-cross-ifunc-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/riscv-pcall-cross-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/riscv-pcall-cross-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/riscv-pcall-cross-root-compact-real.so#poly_entry=0x415c019d41b0013d \
    /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-compact-real.so#poly_entry=0x4355026d42d7019f \
    /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/riscv-pcall-cross-root-fp64-stack-real.so#poly_entry=0x4061100000000000 \
    /usr/lib/polyapps/riscv-pcall-cross-root-vec128-real.so#poly_entry=0x01bc014d00de006f \
    /usr/lib/polyapps/riscv-pcall-symbolic-preempt-real.so#poly_entry=1005 \
    /usr/lib/polyapps/riscv-pcall-symbolic-bind-real.so#poly_entry=15 \
    /usr/lib/polyapps/riscv-pcall-symbolic-protected-real.so#poly_entry=15 \
    /usr/lib/polyapps/riscv-pcall-abs-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-origin-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-platform-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-lib-needed-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-abs-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-rpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-rpath-inherit-real.so#poly_entry=745 \
    /usr/lib/polyapps/rpathorigin/riscv/riscv-pcall-rpath-origin-real.so#poly_entry=745 \
    /usr/lib/polyapps/rpathrunpath/riscv/riscv-pcall-rpath-runpath-real.so#poly_entry=1745 \
    /usr/lib/polyapps/sonameonce/riscv/riscv-pcall-soname-once-real.so#poly_entry=235 \
    /usr/lib/polyapps/riscv-pcall-colon-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-braced-origin-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-lib-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-braced-lib-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-platform-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-braced-platform-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-ld-library-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-ld-platform-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-ld-origin-path-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-ld-prefer-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-relative-runpath-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-runpath-prefer-real.so#poly_entry=945 \
    /usr/lib/polyapps/riscv-pcall-many-needed-real.so#poly_entry=4545 \
    /usr/lib/polyapps/riscv-pcall-root-export-real.so#poly_entry=1345 \
    /usr/lib/polyapps/riscv-pcall-root-tls-real.so#poly_entry=2348 \
    /usr/lib/polyapps/riscv-pcall-cross-root-tls-real.so#poly_entry=2348 \
    /usr/lib/polyapps/riscv-pcall-root-ifunc-real.so#poly_entry=2045 \
    /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-real.so#poly_entry=2045 \
    /usr/lib/polyapps/riscv-pcall-root-weak-real.so#poly_entry=955 \
    /usr/lib/polyapps/riscv-pcall-needed-tls-real.so#poly_entry=545 \
    repeat:/usr/lib/polyapps/riscv-pcall-needed-tls-real.so#poly_entry=548 \
    /usr/lib/polyapps/riscv-pcall-needed-tls-external-real.so#poly_entry=1045 \
    /usr/lib/polyapps/riscv-pcall-cross-needed-tls-external-real.so#poly_entry=1045 \
    /usr/lib/polyapps/riscv-pcall-versioned-real.so#poly_entry=1045 \
    /usr/lib/polyapps/riscv-pcall-needed-ifunc-real.so#poly_entry=845 \
    /usr/lib/polyapps/riscv-pcall-needed-dt-init-real.so#poly_entry=945 \
    depfini:/usr/lib/polyapps/riscv-pcall-needed-dt-init-real.so#poly_entry=1945 \
    /usr/lib/polyapps/riscv-pcall-needed-relro-real.so#poly_entry=745 \
    /usr/lib/polyapps/riscv-pcall-copy-reloc.elf#poly_entry=78 \
    /usr/lib/polyapps/riscv-pcall-funcptr-real.so#poly_entry=124 \
    pair:/usr/lib/polyapps/riscv-pcall-pair-real.so#poly_entry=0x620000002d \
    sret:/usr/lib/polyapps/riscv-pcall-sret-real.so#poly_entry=0x000a001a005102a6 \
    /usr/lib/polyapps/riscv-pcall-ctor-real.so#poly_entry=245 \
    fini:/usr/lib/polyapps/riscv-pcall-fini-real.so#poly_entry=0x4158000000000479 \
    /usr/lib/polyapps/riscv-pcall-dt-init-real.so#poly_entry=345 \
    fini:/usr/lib/polyapps/riscv-pcall-dt-init-real.so#poly_entry=1345 \
    /usr/lib/polyapps/riscv-pcall-preinit-real.elf#poly_entry=345 \
    /usr/lib/polyapps/riscv-pcall-relro-real.so#poly_entry=745 \
    /usr/lib/polyapps/riscv-pcall-initfirst-real.so#poly_entry=21 \
    /usr/lib/polyapps/riscv-pcall-tls-real.so#poly_entry=55 \
    repeat:/usr/lib/polyapps/riscv-pcall-tls-real.so#poly_entry=100 \
    /usr/lib/polyapps/riscv-pcall-tls-ie-real.so#poly_entry=55 \
    repeat:/usr/lib/polyapps/riscv-pcall-tls-ie-real.so#poly_entry=100 \
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
    sigregs-fp64:/usr/lib/polyapps/riscv-pcall-fp64-real.so#poly_entry=0x4026800000000000 \
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
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fp64-sum10-import-real.so#poly_entry=0x4070040000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fp64-callee-import-real.so#poly_entry=0x40993e0000000000 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fp64-callee-stack-import-real.so#poly_entry=0x409a0e0000000000 \
    fpair:/usr/lib/polyapps/riscv-pcall-x86-fpair64-import-real.so#poly_entry=0x405ad0004069e800 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-fpair64-fp64-callee-import-real.so#poly_entry=0x409ae50000000000 \
    fpair32:/usr/lib/polyapps/riscv-pcall-x86-fpair32-import-real.so#poly_entry=0x41da0000418a0000 \
    fp32:/usr/lib/polyapps/riscv-pcall-x86-fpair32-fp32-callee-import-real.so#poly_entry=0x44b56800 \
    vec128u32:/usr/lib/polyapps/riscv-pcall-x86-vec128-import-real.so#poly_entry=0x01e4016b00f20079 \
    vec128u32:/usr/lib/polyapps/riscv-pcall-x86-vec128-fp64-callee-import-real.so#poly_entry=0x0208018f0116009d \
    /usr/lib/polyapps/riscv-pcall-x86-sret-import-real.so#poly_entry=0x000b00160021002e \
    /usr/lib/polyapps/riscv-pcall-x86-sret-stack-import-real.so#poly_entry=0x000b001600210042 \
    sret10:/usr/lib/polyapps/riscv-pcall-x86-sret-stack10-import-real.so#poly_entry=0x000b00160021008c \
    /usr/lib/polyapps/riscv-pcall-x86-sret-callee-stack-import-real.so#poly_entry=7356 \
    fp64:/usr/lib/polyapps/riscv-pcall-x86-sret-fp64-callee-stack-import-real.so#poly_entry=0x4099450000000000 \
    mixedargs:/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-import-real.so#poly_entry=0x406aa80000000000 \
    mixedargs:/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-callee-import-real.so#poly_entry=0x40b2f1c000000000 \
    mixedstack:/usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-stack-import-real.so#poly_entry=0x40788c0000000000 \
    fp32:/usr/lib/polyapps/riscv-pcall-x86-fp32-import-real.so#poly_entry=0x434f4000 \
    /usr/lib/polyapps/riscv-pcall-x86-sum8-import-real.so#poly_entry=236 \
    /usr/lib/polyapps/riscv-pcall-x86-sum10-import-real.so#poly_entry=255 \
    /usr/lib/polyapps/riscv-pcall-x86-sum14-import-real.so#poly_entry=305 \
    /usr/lib/polyapps/riscv-pcall-x86-align14-import-real.so#poly_entry=305 \
    /usr/lib/polyapps/riscv-pcall-x86-i128-import-real.so#poly_entry=0x22421121 \
    /usr/lib/polyapps/riscv-pcall-x86-i128-callee-import-real.so#poly_entry=7201 \
    /usr/lib/polyapps/riscv-pcall-x86-callee-import-real.so#poly_entry=7386 \
    /usr/lib/polyapps/riscv-pcall-x86-callee-stack-import-real.so#poly_entry=7455 \
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
    /usr/lib/polyapps/aarch64-pair-frame.elf=35 \
    /usr/lib/polyapps/aarch64-pcall-bl.elf=3 \
    /usr/lib/polyapps/aarch64-pcall-adrp.elf=42 \
    /usr/lib/polyapps/aarch64-pcall-cond.elf=42 \
    /usr/lib/polyapps/aarch64-hints.elf=27 \
    /usr/lib/polyapps/aarch64-pcall-split-load.elf=123 \
    /usr/lib/polyapps/aarch64-prfm.elf=27 \
    /usr/lib/polyapps/aarch64-pcall-none-reloc.elf#poly_entry=123 \
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
    /usr/lib/polyapps/riscv-pair-frame.elf=35 \
    /usr/lib/polyapps/riscv-pcall-split-load.elf=123 \
    /usr/lib/polyapps/riscv-pcall-none-reloc.elf#poly_entry=123 \
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
    POLY_LD_LIBRARY_PATH='/usr/lib/polyapps/missing-envdeps:\$ORIGIN/envorigin:/usr/lib/polyapps/envdeps:/usr/lib/polyapps/envdeps/\$PLATFORM' /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-ld-library-path-real.so#poly_entry=945 \
      /usr/lib/polyapps/aarch64-pcall-ld-platform-path-real.so#poly_entry=945 \
      /usr/lib/polyapps/aarch64-pcall-ld-origin-path-real.so#poly_entry=945 \
      /usr/lib/polyapps/riscv-pcall-ld-library-path-real.so#poly_entry=945 \
      /usr/lib/polyapps/riscv-pcall-ld-platform-path-real.so#poly_entry=945 \
      /usr/lib/polyapps/riscv-pcall-ld-origin-path-real.so#poly_entry=945 \
      >/dev/ttyS0 2>&1
    /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=945 \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreload-aarch64.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-real.so#poly_entry=945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreload-riscv.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-real.so#poly_entry=945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadoverride-aarch64.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/libpolypreloadoverride-aarch64.so' \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolypreloadoverride-aarch64.so' \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadoverride-aarch64.so:/usr/lib/polyapps/libpolypreloadsecond-aarch64.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadsecond-aarch64.so:/usr/lib/polyapps/libpolypreloadoverride-aarch64.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=2945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadchain-aarch64.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=3945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadchain-aarch64.so \
      /usr/bin/polycall \
      depfini:/usr/lib/polyapps/aarch64-pcall-preload-needed-real.so#poly_entry=4900 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadoverride-riscv.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/libpolypreloadoverride-riscv.so' \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolypreloadoverride-riscv.so' \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadoverride-riscv.so:/usr/lib/polyapps/libpolypreloadsecond-riscv.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=1945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadsecond-riscv.so:/usr/lib/polyapps/libpolypreloadoverride-riscv.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=2945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadchain-riscv.so \
      /usr/bin/polycall \
      /usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=3945 \
      >/dev/ttyS0 2>&1
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolypreloadchain-riscv.so \
      /usr/bin/polycall \
      depfini:/usr/lib/polyapps/riscv-pcall-preload-needed-real.so#poly_entry=4900 \
      >/dev/ttyS0 2>&1
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
  echo ':poly-aarch64:M:18:\xb7::/usr/bin/polybinfmt:P' 2>/dev/ttyS0 > /proc/sys/fs/binfmt_misc/register || {
    echo "POLYBINFMT_FAIL: register aarch64" >/dev/ttyS0
    exit 1
  }
  echo ':poly-riscv:M:18:\xf3::/usr/bin/polybinfmt:P' 2>/dev/ttyS0 > /proc/sys/fs/binfmt_misc/register || {
    echo "POLYBINFMT_FAIL: register riscv" >/dev/ttyS0
    exit 1
  }
  echo "POLYBINFMT_REGISTERED" >/dev/ttyS0
  /usr/bin/nativecheck.elf >/dev/ttyS0 2>&1 || {
    echo "POLYBINFMT_FAIL: native x86 elf" >/dev/ttyS0
    exit 1
  }
  if [ "$RUN_POLY_BINFMT_ARCH_TRAPS" = "1" ]; then
    POLY_PROCESS_ENV=present /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-argv-envp-real.elf=42 \
      alpha beta >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process argv/env" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-syscall-real.elf=42 \
      probe >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process syscalls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-reloc-real.elf=42 \
      reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process relocations" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-real.elf=42 \
      needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process needed" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process preload" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolyprocesspreload-aarch64.so' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload-origin-platform >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process preload origin platform" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-aarch64.so:/usr/lib/polyapps/libpolyprocesspreloadsecond-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-real.elf=42 \
      preload-first-wins >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process preload first wins" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreloadsecond-aarch64.so:/usr/lib/polyapps/libpolyprocesspreload-aarch64.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preload-second-real.elf=42 \
      preload-second-wins >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process preload second wins" >/dev/ttyS0
      exit 1
    }
    POLY_LD_LIBRARY_PATH='/usr/lib/polyapps/processenvdeps/\$PLATFORM' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-envpath-real.elf=42 \
      env-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process env library path" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-runpath-real.elf=42 \
      runpath-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process runpath needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-rpath-real.elf=42 \
      rpath-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process rpath needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-transitive-real.elf=42 \
      transitive-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process transitive needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-indirect-real.elf=42 \
      indirect-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process indirect needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-root-export-real.elf=42 \
      root-export-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process root export needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-needed-root-ifunc-real.elf=42 \
      root-ifunc-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process root ifunc needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-weak-real.elf=42 \
      weak-unresolved >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process weak unresolved" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-weak-needed-real.elf=42 \
      weak-needed-unresolved >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process weak needed unresolved" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-init-real.elf=42 \
      init-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process init array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-preinit-real.elf=42 \
      preinit-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process preinit array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-init-needed-real.elf=42 \
      init-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency init array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-init-real.elf=42 \
      dt-init >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dt init" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-real.elf=42 \
      fini-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process fini array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-exit-group-real.elf=42 \
      fini-exit-group >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process fini exit group" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-order-real.elf=42 \
      fini-order >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process fini array order" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-fini-real.elf=42 \
      dt-fini >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dt fini" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-fini-needed-real.elf=42 \
      fini-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency fini array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-fini-needed-real.elf=42 \
      dt-fini-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency dt fini" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-dt-init-needed-real.elf=42 \
      dt-init-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency dt init" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-versioned-needed-real.elf=42 \
      versioned-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process versioned needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-real.elf=42 \
      tls >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-needed-real.elf=42 \
      tls-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-default-real.elf=42 \
      tls-default >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process default tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-default-needed-real.elf=42 \
      tls-default-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency default tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-trad-real.elf=42 \
      tls-trad >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process traditional tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-tls-trad-needed-real.elf=42 \
      tls-trad-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process dependency traditional tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/aarch64-process-copy-reloc-real.elf=42 \
      copy-reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process copy relocation" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/sonameonce/aarch64/aarch64-process-soname-once-real.elf=42 \
      soname-once >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 process soname once" >/dev/ttyS0
      exit 1
    }
    POLY_PROCESS_ENV=present \
      /usr/lib/polyapps/aarch64-process-argv-envp-real.elf \
      alpha beta >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 direct process argv/env" >/dev/ttyS0
      exit 1
    }
    /usr/lib/polyapps/aarch64-process-syscall-real.elf \
      probe >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 direct process syscalls" >/dev/ttyS0
      exit 1
    }
    /usr/lib/polyapps/aarch64-process-reloc-real.elf \
      reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: aarch64 direct process relocations" >/dev/ttyS0
      exit 1
    }
    POLY_PROCESS_ENV=present /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-argv-envp-real.elf=42 \
      alpha beta >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process argv/env" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-syscall-real.elf=42 \
      probe >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process syscalls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-reloc-real.elf=42 \
      reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process relocations" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-real.elf=42 \
      needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process needed" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process preload" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD='\$ORIGIN/\$PLATFORM/libpolyprocesspreload-riscv.so' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload-origin-platform >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process preload origin platform" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreload-riscv.so:/usr/lib/polyapps/libpolyprocesspreloadsecond-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-real.elf=42 \
      preload-first-wins >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process preload first wins" >/dev/ttyS0
      exit 1
    }
    POLY_LD_PRELOAD=/usr/lib/polyapps/libpolyprocesspreloadsecond-riscv.so:/usr/lib/polyapps/libpolyprocesspreload-riscv.so \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preload-second-real.elf=42 \
      preload-second-wins >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process preload second wins" >/dev/ttyS0
      exit 1
    }
    POLY_LD_LIBRARY_PATH='/usr/lib/polyapps/processenvdeps/\$PLATFORM' \
      /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-envpath-real.elf=42 \
      env-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process env library path" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-runpath-real.elf=42 \
      runpath-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process runpath needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-rpath-real.elf=42 \
      rpath-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process rpath needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-transitive-real.elf=42 \
      transitive-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process transitive needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-indirect-real.elf=42 \
      indirect-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process indirect needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-root-export-real.elf=42 \
      root-export-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process root export needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-needed-root-ifunc-real.elf=42 \
      root-ifunc-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process root ifunc needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-weak-real.elf=42 \
      weak-unresolved >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process weak unresolved" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-weak-needed-real.elf=42 \
      weak-needed-unresolved >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process weak needed unresolved" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-init-real.elf=42 \
      init-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process init array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-preinit-real.elf=42 \
      preinit-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process preinit array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-init-needed-real.elf=42 \
      init-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency init array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-init-real.elf=42 \
      dt-init >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dt init" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-real.elf=42 \
      fini-array >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process fini array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-exit-group-real.elf=42 \
      fini-exit-group >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process fini exit group" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-order-real.elf=42 \
      fini-order >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process fini array order" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-fini-real.elf=42 \
      dt-fini >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dt fini" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-fini-needed-real.elf=42 \
      fini-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency fini array" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-fini-needed-real.elf=42 \
      dt-fini-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency dt fini" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-dt-init-needed-real.elf=42 \
      dt-init-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency dt init" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-versioned-needed-real.elf=42 \
      versioned-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process versioned needed" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-real.elf=42 \
      tls >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-needed-real.elf=42 \
      tls-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-default-real.elf=42 \
      tls-default >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process default tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-tls-default-needed-real.elf=42 \
      tls-default-needed >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process dependency default tls" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/riscv-process-copy-reloc-real.elf=42 \
      copy-reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process copy relocation" >/dev/ttyS0
      exit 1
    }
    /usr/bin/polyexec --process \
      /usr/lib/polyapps/sonameonce/riscv/riscv-process-soname-once-real.elf=42 \
      soname-once >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv process soname once" >/dev/ttyS0
      exit 1
    }
    POLY_PROCESS_ENV=present \
      /usr/lib/polyapps/riscv-process-argv-envp-real.elf \
      alpha beta >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv direct process argv/env" >/dev/ttyS0
      exit 1
    }
    /usr/lib/polyapps/riscv-process-syscall-real.elf \
      probe >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv direct process syscalls" >/dev/ttyS0
      exit 1
    }
    /usr/lib/polyapps/riscv-process-reloc-real.elf \
      reloc >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: riscv direct process relocations" >/dev/ttyS0
      exit 1
    }
    for foreign in \
      /usr/lib/polyapps/aarch64-add.elf \
      /usr/lib/polyapps/aarch64-regadd.elf \
      /usr/lib/polyapps/aarch64-movwide.elf \
      /usr/lib/polyapps/aarch64-mul.elf \
      /usr/lib/polyapps/aarch64-logical.elf \
      /usr/lib/polyapps/aarch64-shifted.elf \
      /usr/lib/polyapps/aarch64-simd-logical.elf \
      /usr/lib/polyapps/aarch64-simd-addsub.elf \
      /usr/lib/polyapps/aarch64-simd-movi.elf \
      /usr/lib/polyapps/aarch64-simd-modimm.elf \
      /usr/lib/polyapps/aarch64-simd-compare.elf \
      /usr/lib/polyapps/aarch64-simd-ext.elf \
      /usr/lib/polyapps/aarch64-simd-permute.elf \
      /usr/lib/polyapps/aarch64-simd-tbl.elf \
      /usr/lib/polyapps/aarch64-simd-rev.elf \
      /usr/lib/polyapps/aarch64-simd-reduce.elf \
      /usr/lib/polyapps/aarch64-regmix.elf \
      /usr/lib/polyapps/aarch64-branch.elf \
      /usr/lib/polyapps/aarch64-condbranch.elf \
      /usr/lib/polyapps/aarch64-loop.elf \
      /usr/lib/polyapps/aarch64-ret.elf \
      /usr/lib/polyapps/aarch64-mem.elf \
      /usr/lib/polyapps/aarch64-memwidth.elf \
      /usr/lib/polyapps/aarch64-pair-frame.elf \
      /usr/lib/polyapps/aarch64-hints.elf \
      /usr/lib/polyapps/aarch64-prfm.elf \
      /usr/lib/polyapps/aarch64-pcall-split-load.elf \
      /usr/lib/polyapps/aarch64-polyexec-gnu-hash-real.so \
      /usr/lib/polyapps/aarch64-pcall-dynrel.elf \
      /usr/lib/polyapps/aarch64-pcall-rel.elf \
      /usr/lib/polyapps/aarch64-pcall-relr.elf \
      /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf \
      /usr/lib/polyapps/aarch64-pcall-irelative.elf \
      /usr/lib/polyapps/aarch64-pcall-jumprel.elf \
      /usr/lib/polyapps/aarch64-pcall-rel-jumprel.elf \
      /usr/lib/polyapps/aarch64-strlen.elf \
      /usr/lib/polyapps/aarch64-memfill.elf \
      /usr/lib/polyapps/aarch64-memcmp.elf \
      /usr/lib/polyapps/aarch64-memcpy.elf \
      /usr/lib/polyapps/aarch64-mmap-real-store.elf \
      /usr/lib/polyapps/aarch64-real-mprotect.elf \
      /usr/lib/polyapps/aarch64-real-munmap.elf \
      /usr/lib/polyapps/aarch64-real-openat-read-close.elf \
      /usr/lib/polyapps/aarch64-real-newfstatat.elf \
      /usr/lib/polyapps/aarch64-real-fstat0.elf \
      /usr/lib/polyapps/aarch64-real-statx.elf \
      /usr/lib/polyapps/aarch64-real-write-zero.elf \
      /usr/lib/polyapps/aarch64-real-clock-getres.elf \
      /usr/lib/polyapps/aarch64-real-gettimeofday.elf \
      /usr/lib/polyapps/aarch64-eventfd2.elf \
      /usr/lib/polyapps/aarch64-inotify-init1.elf \
      /usr/lib/polyapps/aarch64-inotify-add-watch.elf \
      /usr/lib/polyapps/aarch64-inotify-rm-watch.elf \
      /usr/lib/polyapps/aarch64-dup3.elf \
      /usr/lib/polyapps/aarch64-getpgid.elf \
      /usr/lib/polyapps/aarch64-getsid.elf \
      /usr/lib/polyapps/aarch64-getrlimit.elf \
      /usr/lib/polyapps/aarch64-fp-int-move.elf \
      /usr/lib/polyapps/aarch64-fp32-to-int.elf \
      /usr/lib/polyapps/aarch64-fp32-to-int64.elf \
      /usr/lib/polyapps/aarch64-fp-softfloat.elf \
      /usr/lib/polyapps/aarch64-fp-minmax-nan.elf \
      /usr/lib/polyapps/aarch64-fpcr-fpsr.elf \
      /usr/lib/polyapps/aarch64-brk.elf \
      /usr/lib/polyapps/aarch64-svc.elf \
      /usr/lib/polyapps/aarch64-pcall-needed-real.so \
      /usr/lib/polyapps/aarch64-pcall-lib-needed-real.so \
      /usr/lib/polyapps/aarch64-pcall-many-needed-real.so \
      /usr/lib/polyapps/aarch64-pcall-needed-tls-real.so \
      /usr/lib/polyapps/aarch64-pcall-needed-ifunc-real.so \
      /usr/lib/polyapps/aarch64-pcall-needed-dt-init-real.so \
      /usr/lib/polyapps/aarch64-pcall-libc-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-qsort-real.so \
      /usr/lib/polyapps/aarch64-pcall-bsearch-real.so \
      /usr/lib/polyapps/aarch64-pcall-qsort-r-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-once-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-key-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-mutex-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-self-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-rwlock-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-mutexattr-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-spin-real.so \
      /usr/lib/polyapps/aarch64-pcall-pthread-cond-real.so \
      /usr/lib/polyapps/aarch64-pcall-time-real.so \
      /usr/lib/polyapps/aarch64-pcall-stack-protector-real.so \
      /usr/lib/polyapps/aarch64-pcall-getauxval-real.so \
      /usr/lib/polyapps/aarch64-pcall-errno-real.so \
      /usr/lib/polyapps/aarch64-pcall-env-real.so \
      /usr/lib/polyapps/aarch64-pcall-alloc-real.so \
      /usr/lib/polyapps/aarch64-pcall-snprintf-real.so \
      /usr/lib/polyapps/aarch64-pcall-integer-parse-real.so \
      /usr/lib/polyapps/aarch64-pcall-ctype-real.so \
      /usr/lib/polyapps/aarch64-pcall-abs-real.so \
      /usr/lib/polyapps/aarch64-pcall-atol-real.so \
      /usr/lib/polyapps/aarch64-pcall-ffs-real.so \
      /usr/lib/polyapps/aarch64-pcall-strtod-real.so \
      /usr/lib/polyapps/aarch64-pcall-strtof-real.so \
      /usr/lib/polyapps/aarch64-pcall-fabsf-real.so \
      /usr/lib/polyapps/aarch64-pcall-fabs-real.so \
      /usr/lib/polyapps/aarch64-pcall-sqrtf-real.so \
      /usr/lib/polyapps/aarch64-pcall-sqrt-real.so \
      /usr/lib/polyapps/aarch64-pcall-rounding-real.so \
      /usr/lib/polyapps/aarch64-pcall-string-search-real.so \
      /usr/lib/polyapps/aarch64-pcall-strdup-real.so \
      /usr/lib/polyapps/aarch64-pcall-atexit-real.so \
      /usr/lib/polyapps/aarch64-pcall-cxa-guard-real.so \
      /usr/lib/polyapps/aarch64-pcall-cxx-static-guard-real.so \
      /usr/lib/polyapps/aarch64-pcall-cxx-virtual-real.so \
      /usr/lib/polyapps/aarch64-pcall-fp64-real.so \
      /usr/lib/polyapps/aarch64-pcall-fp64-stack-real.so \
      /usr/lib/polyapps/aarch64-pcall-fpair-real.so \
      /usr/lib/polyapps/aarch64-pcall-fpair32-real.so \
      /usr/lib/polyapps/aarch64-pcall-fpair-arg-real.so \
      /usr/lib/polyapps/aarch64-pcall-fpair32-arg-real.so \
      /usr/lib/polyapps/aarch64-pcall-hfa3-real.so \
      /usr/lib/polyapps/aarch64-pcall-hfa4-real.so \
      /usr/lib/polyapps/aarch64-pcall-hfa3-f32-real.so \
      /usr/lib/polyapps/aarch64-pcall-hfa4-f32-real.so \
      /usr/lib/polyapps/aarch64-pcall-hfa-arg-real.so \
      /usr/lib/polyapps/aarch64-pcall-vec128-real.so \
      /usr/lib/polyapps/aarch64-pcall-mixed-args-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-rev-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero32-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero32-rev-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-u32-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-u32-rev-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-u32-f32-real.so \
      /usr/lib/polyapps/aarch64-pcall-hetero-f32-u32-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-needed-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-needed-transitive-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-compact-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-ifunc-compact-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-ifunc-fp64-stack-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-ifunc-vec128-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-fp64-stack-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-vec128-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-compact-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-compact-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-fp64-stack-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-vec128-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-fp64-stack-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-vec128-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-tls-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-root-ifunc-real.so \
      /usr/lib/polyapps/aarch64-pcall-cross-needed-tls-external-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sret-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sret-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sret-callee-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sum8-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sum10-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sum14-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-align14-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-i128-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-i128-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-callee-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sum8-post-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp64-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp64-sum8-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp64-sum10-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp64-callee-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fpair64-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fpair64-fp64-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fpair32-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fpair32-fp32-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-vec128-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-vec128-fp64-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sret-stack10-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-sret-fp64-callee-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-callee-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-mixed-u64-fp64-stack-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-x86-fp32-import-real.so \
      /usr/lib/polyapps/aarch64-pcall-abs-runpath-real.so \
      /usr/lib/polyapps/aarch64-pcall-rpath-real.so \
      /usr/lib/polyapps/aarch64-pcall-rpath-inherit-real.so \
      /usr/lib/polyapps/aarch64-pcall-colon-runpath-real.so \
      /usr/lib/polyapps/aarch64-pcall-lib-runpath-real.so \
      /usr/lib/polyapps/aarch64-pcall-platform-runpath-real.so \
      /usr/lib/polyapps/aarch64-pcall-runpath-prefer-real.so \
      /usr/lib/polyapps/aarch64-pcall-needed-relro-real.so \
      /usr/lib/polyapps/aarch64-pcall-relro-real.so \
      /usr/lib/polyapps/riscv-add.elf \
      /usr/lib/polyapps/riscv-compressed.elf \
      /usr/lib/polyapps/riscv-compressed-half.elf \
      /usr/lib/polyapps/riscv-compressed-jalr.elf \
      /usr/lib/polyapps/riscv-compressed-word.elf \
      /usr/lib/polyapps/riscv-compressed-alu.elf \
      /usr/lib/polyapps/riscv-compressed-fp.elf \
      /usr/lib/polyapps/riscv-compressed-sdsp.elf \
      /usr/lib/polyapps/riscv-compressed-hints.elf \
      /usr/lib/polyapps/riscv-fp-int-move.elf \
      /usr/lib/polyapps/riscv-fp-class.elf \
      /usr/lib/polyapps/riscv-fp32-to-int.elf \
      /usr/lib/polyapps/riscv-fp-csr.elf \
      /usr/lib/polyapps/riscv-fp-round.elf \
      /usr/lib/polyapps/riscv-fp-arith-round.elf \
      /usr/lib/polyapps/riscv-fp-cvt-round.elf \
      /usr/lib/polyapps/riscv-fp-nan-flags.elf \
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
      /usr/lib/polyapps/riscv-zbb.elf \
      /usr/lib/polyapps/riscv-zba.elf \
      /usr/lib/polyapps/riscv-zbs.elf \
      /usr/lib/polyapps/riscv-zicond.elf \
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
      /usr/lib/polyapps/riscv-pair-frame.elf \
      /usr/lib/polyapps/riscv-pcall-split-load.elf \
      /usr/lib/polyapps/riscv-polyexec-gnu-hash-real.so \
      /usr/lib/polyapps/riscv-polyexec-gnu-hash-rv64gc.so \
      /usr/lib/polyapps/riscv-pcall-dynrel.elf \
      /usr/lib/polyapps/riscv-pcall-rel.elf \
      /usr/lib/polyapps/riscv-pcall-relr.elf \
      /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf \
      /usr/lib/polyapps/riscv-pcall-irelative.elf \
      /usr/lib/polyapps/riscv-pcall-jumprel.elf \
      /usr/lib/polyapps/riscv-pcall-rel-jumprel.elf \
      /usr/lib/polyapps/riscv-strlen.elf \
      /usr/lib/polyapps/riscv-memfill.elf \
      /usr/lib/polyapps/riscv-memcmp.elf \
      /usr/lib/polyapps/riscv-memcpy.elf \
      /usr/lib/polyapps/riscv-mmap-real-store.elf \
      /usr/lib/polyapps/riscv-real-mprotect.elf \
      /usr/lib/polyapps/riscv-real-munmap.elf \
      /usr/lib/polyapps/riscv-real-openat-read-close.elf \
      /usr/lib/polyapps/riscv-real-newfstatat.elf \
      /usr/lib/polyapps/riscv-real-fstat0.elf \
      /usr/lib/polyapps/riscv-real-statx.elf \
      /usr/lib/polyapps/riscv-real-write-zero.elf \
      /usr/lib/polyapps/riscv-real-clock-getres.elf \
      /usr/lib/polyapps/riscv-real-gettimeofday.elf \
      /usr/lib/polyapps/riscv-eventfd2.elf \
      /usr/lib/polyapps/riscv-inotify-init1.elf \
      /usr/lib/polyapps/riscv-inotify-add-watch.elf \
      /usr/lib/polyapps/riscv-inotify-rm-watch.elf \
      /usr/lib/polyapps/riscv-dup3.elf \
      /usr/lib/polyapps/riscv-getpgid.elf \
      /usr/lib/polyapps/riscv-getsid.elf \
      /usr/lib/polyapps/riscv-getrlimit.elf \
      /usr/lib/polyapps/riscv-ebreak.elf \
      /usr/lib/polyapps/riscv-compressed-ebreak.elf \
      /usr/lib/polyapps/riscv-ecall.elf \
      /usr/lib/polyapps/riscv-pcall-needed-real.so \
      /usr/lib/polyapps/riscv-pcall-lib-needed-real.so \
      /usr/lib/polyapps/riscv-pcall-many-needed-real.so \
      /usr/lib/polyapps/riscv-pcall-needed-tls-real.so \
      /usr/lib/polyapps/riscv-pcall-needed-ifunc-real.so \
      /usr/lib/polyapps/riscv-pcall-needed-dt-init-real.so \
      /usr/lib/polyapps/riscv-pcall-libc-import-real.so \
      /usr/lib/polyapps/riscv-pcall-qsort-real.so \
      /usr/lib/polyapps/riscv-pcall-bsearch-real.so \
      /usr/lib/polyapps/riscv-pcall-qsort-r-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-once-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-key-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-mutex-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-self-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-rwlock-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-mutexattr-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-spin-real.so \
      /usr/lib/polyapps/riscv-pcall-pthread-cond-real.so \
      /usr/lib/polyapps/riscv-pcall-time-real.so \
      /usr/lib/polyapps/riscv-pcall-stack-protector-real.so \
      /usr/lib/polyapps/riscv-pcall-getauxval-real.so \
      /usr/lib/polyapps/riscv-pcall-errno-real.so \
      /usr/lib/polyapps/riscv-pcall-env-real.so \
      /usr/lib/polyapps/riscv-pcall-alloc-real.so \
      /usr/lib/polyapps/riscv-pcall-snprintf-real.so \
      /usr/lib/polyapps/riscv-pcall-integer-parse-real.so \
      /usr/lib/polyapps/riscv-pcall-ctype-real.so \
      /usr/lib/polyapps/riscv-pcall-abs-real.so \
      /usr/lib/polyapps/riscv-pcall-atol-real.so \
      /usr/lib/polyapps/riscv-pcall-ffs-real.so \
      /usr/lib/polyapps/riscv-pcall-strtod-real.so \
      /usr/lib/polyapps/riscv-pcall-strtof-real.so \
      /usr/lib/polyapps/riscv-pcall-fabsf-real.so \
      /usr/lib/polyapps/riscv-pcall-fabs-real.so \
      /usr/lib/polyapps/riscv-pcall-sqrtf-real.so \
      /usr/lib/polyapps/riscv-pcall-sqrt-real.so \
      /usr/lib/polyapps/riscv-pcall-rounding-real.so \
      /usr/lib/polyapps/riscv-pcall-string-search-real.so \
      /usr/lib/polyapps/riscv-pcall-strdup-real.so \
      /usr/lib/polyapps/riscv-pcall-atexit-real.so \
      /usr/lib/polyapps/riscv-pcall-cxa-guard-real.so \
      /usr/lib/polyapps/riscv-pcall-cxx-static-guard-real.so \
      /usr/lib/polyapps/riscv-pcall-cxx-virtual-real.so \
      /usr/lib/polyapps/riscv-pcall-fp64-real.so \
      /usr/lib/polyapps/riscv-pcall-fp64-stack-real.so \
      /usr/lib/polyapps/riscv-pcall-fpair-real.so \
      /usr/lib/polyapps/riscv-pcall-fpair32-real.so \
      /usr/lib/polyapps/riscv-pcall-fpair-arg-real.so \
      /usr/lib/polyapps/riscv-pcall-fpair32-arg-real.so \
      /usr/lib/polyapps/riscv-pcall-mixed-args-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-rev-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero32-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero32-rev-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-u32-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-u32-rev-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-u32-f32-real.so \
      /usr/lib/polyapps/riscv-pcall-hetero-f32-u32-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-needed-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-needed-transitive-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-compact-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-ifunc-compact-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-ifunc-fp64-stack-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-ifunc-vec128-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-fp64-stack-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-vec128-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-compact-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-compact-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-fp64-stack-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-vec128-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-fp64-stack-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-vec128-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-tls-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-root-ifunc-real.so \
      /usr/lib/polyapps/riscv-pcall-cross-needed-tls-external-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sret-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sret-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sret-callee-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sum8-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sum10-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sum14-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-align14-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-i128-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-i128-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-callee-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sum8-post-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp64-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp64-sum8-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp64-sum10-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp64-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp64-callee-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fpair64-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fpair64-fp64-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fpair32-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fpair32-fp32-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-vec128-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-vec128-fp64-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sret-stack10-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-sret-fp64-callee-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-callee-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-mixed-u64-fp64-stack-import-real.so \
      /usr/lib/polyapps/riscv-pcall-x86-fp32-import-real.so \
      /usr/lib/polyapps/riscv-pcall-abs-runpath-real.so \
      /usr/lib/polyapps/riscv-pcall-rpath-real.so \
      /usr/lib/polyapps/riscv-pcall-rpath-inherit-real.so \
      /usr/lib/polyapps/riscv-pcall-colon-runpath-real.so \
      /usr/lib/polyapps/riscv-pcall-lib-runpath-real.so \
      /usr/lib/polyapps/riscv-pcall-platform-runpath-real.so \
      /usr/lib/polyapps/riscv-pcall-runpath-prefer-real.so \
      /usr/lib/polyapps/riscv-pcall-needed-relro-real.so \
      /usr/lib/polyapps/riscv-pcall-relro-real.so
    do
      echo "POLYBINFMT_STEP: \$foreign" >/dev/ttyS0
      "\$foreign" >/dev/ttyS0 2>&1 || {
        echo "POLYBINFMT_FAIL: exec \$foreign" >/dev/ttyS0
        exit 1
      }
    done
  else
    for foreign in \
    /usr/lib/polyapps/aarch64-add.elf \
    /usr/lib/polyapps/aarch64-regadd.elf \
    /usr/lib/polyapps/aarch64-movwide.elf \
    /usr/lib/polyapps/aarch64-mul.elf \
    /usr/lib/polyapps/aarch64-logical.elf \
    /usr/lib/polyapps/aarch64-shifted.elf \
    /usr/lib/polyapps/aarch64-simd-logical.elf \
    /usr/lib/polyapps/aarch64-simd-addsub.elf \
    /usr/lib/polyapps/aarch64-simd-movi.elf \
    /usr/lib/polyapps/aarch64-simd-modimm.elf \
    /usr/lib/polyapps/aarch64-simd-compare.elf \
    /usr/lib/polyapps/aarch64-simd-ext.elf \
    /usr/lib/polyapps/aarch64-simd-permute.elf \
    /usr/lib/polyapps/aarch64-simd-tbl.elf \
    /usr/lib/polyapps/aarch64-simd-rev.elf \
    /usr/lib/polyapps/aarch64-simd-reduce.elf \
    /usr/lib/polyapps/aarch64-regmix.elf \
    /usr/lib/polyapps/aarch64-branch.elf \
    /usr/lib/polyapps/aarch64-condbranch.elf \
    /usr/lib/polyapps/aarch64-loop.elf \
    /usr/lib/polyapps/aarch64-ret.elf \
    /usr/lib/polyapps/aarch64-mem.elf \
    /usr/lib/polyapps/aarch64-memwidth.elf \
    /usr/lib/polyapps/aarch64-pair-frame.elf \
    /usr/lib/polyapps/aarch64-hints.elf \
    /usr/lib/polyapps/aarch64-prfm.elf \
    /usr/lib/polyapps/aarch64-pcall-split-load.elf \
    /usr/lib/polyapps/aarch64-pcall-dynrel.elf \
    /usr/lib/polyapps/aarch64-pcall-rel.elf \
    /usr/lib/polyapps/aarch64-pcall-relr.elf \
    /usr/lib/polyapps/aarch64-pcall-relr-bitmap.elf \
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
    /usr/lib/polyapps/aarch64-fp-softfloat.elf \
    /usr/lib/polyapps/aarch64-fp-minmax-nan.elf \
    /usr/lib/polyapps/aarch64-fpcr-fpsr.elf \
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
    /usr/lib/polyapps/riscv-compressed-sdsp.elf \
    /usr/lib/polyapps/riscv-compressed-hints.elf \
    /usr/lib/polyapps/riscv-fp-int-move.elf \
    /usr/lib/polyapps/riscv-fp-class.elf \
    /usr/lib/polyapps/riscv-fp32-to-int.elf \
    /usr/lib/polyapps/riscv-fp-csr.elf \
    /usr/lib/polyapps/riscv-fp-round.elf \
    /usr/lib/polyapps/riscv-fp-arith-round.elf \
    /usr/lib/polyapps/riscv-fp-cvt-round.elf \
    /usr/lib/polyapps/riscv-fp-nan-flags.elf \
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
    /usr/lib/polyapps/riscv-zbb.elf \
    /usr/lib/polyapps/riscv-zba.elf \
    /usr/lib/polyapps/riscv-zbs.elf \
    /usr/lib/polyapps/riscv-zicond.elf \
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
    /usr/lib/polyapps/riscv-pair-frame.elf \
    /usr/lib/polyapps/riscv-pcall-split-load.elf \
    /usr/lib/polyapps/riscv-pcall-dynrel.elf \
    /usr/lib/polyapps/riscv-pcall-rel.elf \
    /usr/lib/polyapps/riscv-pcall-relr.elf \
    /usr/lib/polyapps/riscv-pcall-relr-bitmap.elf \
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
    /usr/lib/polyapps/riscv-compressed-ebreak.elf \
    /usr/lib/polyapps/riscv-ecall.elf \
    /usr/lib/polyapps/riscv-long.elf
  do
    echo "POLYBINFMT_STEP: \$foreign" >/dev/ttyS0
    "\$foreign" >/dev/ttyS0 2>&1 || {
      echo "POLYBINFMT_FAIL: exec \$foreign" >/dev/ttyS0
      exit 1
    }
    done
  fi
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
  local fatal_pattern='Kernel panic|Segmentation fault|segfault|Oops|general protection|BUG:|poly_raw: unhandled|NATIVE_CHECK_FAIL|POLY[A-Z_]*_FAIL'
  while (( SECONDS < deadline )); do
    if grep -Eiq "$fatal_pattern" "$SERIAL_LOG" "$BOCHS_LOG" 2>/dev/null; then
      success=-1
      break
    fi

    if grep -q "BOOT_OK: initramfs reached userspace" "$SERIAL_LOG"; then
      if [[ "$RUN_POLY_PROBE" == "1" ]]; then
        if ! grep -q "POLY_PROBE_CROSS_RETURN_XSAVE_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROBE_MONITOR_PACKETS_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
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
      if [[ "$RUN_POLY_NEUTRAL" == "1" ]]; then
        if ! grep -q "POLY_NEUTRAL_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYAPP_RESULT: arch=aarch64 value=45 path=/usr/lib/polyapps/aarch64-generic-call-riscv\\.poly" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYAPP_RESULT: arch=aarch64 value=45 path=/usr/lib/polyapps/aarch64-generic-switch-riscv\\.poly" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYAPP_RESULT: arch=riscv value=45 path=/usr/lib/polyapps/riscv-generic-call-aarch64\\.poly" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYAPP_RESULT: arch=riscv value=45 path=/usr/lib/polyapps/riscv-generic-switch-aarch64\\.poly" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_EXEC" == "1" ]]; then
        if ! grep -q "POLY_EXEC_BLOCK_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYEXEC_STATE_KEY: explicit=1" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYEXEC_CROSS_STUB_STATE_KEY: explicit=1" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_MONITOR_PACKETS: count=[1-9][0-9]*" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-cross-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-cross-needed-ifunc-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-preload-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-preload-second-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-preinit-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-fini-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-fini-exit-group-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-fini-order-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-dt-fini-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-fini-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-dt-fini-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_FINI_ARRAY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_FINI_EXIT_GROUP_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_FINI_ORDER_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_DT_FINI_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_DEP_FINI_ARRAY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_AARCH64_DEP_DT_FINI_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/sonameonce/aarch64/aarch64-process-soname-once-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_DEP_SONAME_REUSE: arch=aarch64 soname=libpolysonameonce-leaf-aarch64\\.so .* path=.*leafa/libpolysonameonce-leaf-aarch64\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-needed-envpath-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-cross-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-cross-needed-ifunc-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-preload-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-preload-second-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-preinit-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-fini-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-fini-exit-group-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-fini-order-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-dt-fini-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-fini-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-dt-fini-needed-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_FINI_ARRAY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_FINI_EXIT_GROUP_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_FINI_ORDER_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_DT_FINI_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_DEP_FINI_ARRAY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLY_PROCESS_RISCV_DEP_DT_FINI_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/sonameonce/riscv/riscv-process-soname-once-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_DEP_SONAME_REUSE: arch=riscv soname=libpolysonameonce-leaf-riscv\\.so .* path=.*leafa/libpolysonameonce-leaf-riscv\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-needed-envpath-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_ARCH_TRAP_EXEC" == "1" ]]; then
        if ! grep -q "POLY_ARCH_TRAP_EXEC_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_MONITOR_PACKETS: count=[1-9][0-9]*" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_CALL" == "1" ]]; then
        if ! grep -q "POLYCALL_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYCALL_STATE_KEY: explicit=1" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYCALL_STUB_STATE_KEY: explicit=1" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_X86_IMPORT_STUBS: arch=aarch64 .*direct_sigregs=[1-9]" "$SERIAL_LOG" ||
            ! grep -Eq "POLYCALL_RESULT_FP64: arch=aarch64 .*aarch64-pcall-x86-fp64-import-real\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_X86_IMPORT_STUBS: arch=riscv .*direct_sigregs=[1-9]" "$SERIAL_LOG" ||
            ! grep -Eq "POLYCALL_RESULT_FP64: arch=riscv .*riscv-pcall-x86-fp64-import-real\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_CROSS_STUBS: arch=aarch64 .*a64_to_rv_sigregs=[1-9].*aarch64-pcall-cross-needed-real\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_CROSS_STUBS: arch=riscv .*rv_to_a64_sigregs=[1-9].*riscv-pcall-cross-needed-real\\.so" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_ROOT_PCALL: arch=aarch64 exchange_u64=1 .*aarch64-pcall-sum9\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYCALL_ROOT_PCALL: arch=riscv exchange_u64=1 .*riscv-pcall-sum9\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_THREAD" == "1" ]]; then
        if ! grep -q "POLYTHREAD_STATE_KEY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYTHREAD_STATE_ISOLATION_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if [[ "$REQUIRE_POLY_REAL_XSAVE" == "1" ]]; then
          if ! grep -q "POLYTHREAD_REAL_XSAVE_CONTEXT_OK" "$SERIAL_LOG"; then
            sleep 1
            continue
          fi
        fi
        if ! grep -q "POLYTHREAD_MIXED_ATOMIC_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -q "POLYTHREAD_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_POLY_SIGNAL" == "1" ]]; then
        if ! grep -q "POLYSIGNAL_STATE_KEY_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
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
        polybench_patterns=(
          "POLYBENCH_RESULT: arch=aarch64 .*switch_delta=3"
          "POLYBENCH_RESULT: arch=riscv .*switch_delta=3"
          "POLYBENCH_MIXED_RESULT: direction=aarch64-to-riscv .*switch_delta=4"
          "POLYBENCH_MIXED_RESULT: direction=aarch64-to-compressed-riscv .*switch_delta=4"
          "POLYBENCH_MIXED_RESULT: direction=riscv-to-aarch64 .*switch_delta=4"
          "POLYBENCH_MIXED_RESULT: direction=riscv-compressed-to-aarch64 .*switch_delta=4"
          "POLYBENCH_CROSS_CALL_RESULT: direction=aarch64-calls-riscv .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_RESULT: direction=riscv-calls-aarch64 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_RESULT: direction=nested-aarch64-riscv-aarch64 .*switch_delta=7"
          "POLYBENCH_DIRECT_X86_PCALL_RESULT: direction=aarch64-calls-x86-direct .*switch_delta=5"
          "POLYBENCH_DIRECT_X86_PCALL_RESULT: direction=riscv-calls-x86-direct .*switch_delta=5"
          "POLYBENCH_DIRECT_X86_FP64_RESULT: direction=aarch64-calls-x86-direct-fp64 .*switch_delta=5"
          "POLYBENCH_DIRECT_X86_FP64_RESULT: direction=riscv-calls-x86-direct-fp64 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP_RESULT: direction=aarch64-calls-riscv-fp .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP_RESULT: direction=riscv-calls-aarch64-fp .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP8_RESULT: direction=aarch64-calls-riscv-fp8 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP8_RESULT: direction=riscv-calls-aarch64-fp8 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP64_STACK_RESULT: direction=aarch64-calls-riscv-fp64-stack .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_FP64_STACK_RESULT: direction=riscv-calls-aarch64-fp64-stack .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_VEC128_RESULT: direction=aarch64-calls-riscv-vec128 .*switch_delta=4"
          "POLYBENCH_CROSS_CALL_VEC128_RESULT: direction=riscv-calls-aarch64-vec128 .*switch_delta=4"
          "POLYBENCH_CROSS_CALL_MIXED_RESULT: direction=aarch64-calls-riscv-mixed .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_MIXED_RESULT: direction=riscv-calls-aarch64-mixed .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_STACK_RESULT: direction=aarch64-calls-riscv-stack .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_STACK_RESULT: direction=riscv-calls-aarch64-stack .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SAVED_RESULT: direction=aarch64-calls-riscv-saved .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SAVED_RESULT: direction=riscv-calls-aarch64-saved .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SAVED_FP_RESULT: direction=aarch64-calls-riscv-saved-fp .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SAVED_FP_RESULT: direction=riscv-calls-aarch64-saved-fp .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_PAIR_RESULT: direction=aarch64-calls-riscv-pair .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_PAIR_RESULT: direction=riscv-calls-aarch64-pair .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=aarch64-calls-riscv-compact-u32-f32 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=aarch64-calls-riscv-compact-f32-u32 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=riscv-calls-aarch64-compact-u32-f32 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_COMPACT_RESULT: direction=riscv-calls-aarch64-compact-f32-u32 .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SYSCALL_RESULT: direction=aarch64-calls-riscv-syscall .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_SYSCALL_RESULT: direction=riscv-calls-aarch64-syscall .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_BREAK_RESULT: direction=aarch64-calls-riscv-break .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_BREAK_RESULT: direction=riscv-calls-aarch64-break .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_IMPORT_RESULT: direction=aarch64-calls-riscv-import .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_IMPORT_RESULT: direction=riscv-calls-aarch64-import .*switch_delta=5"
          "POLYBENCH_CROSS_CALL_STRING_RESULT: direction=aarch64-calls-riscv-direct-x86 .*switch_delta=7"
          "POLYBENCH_CROSS_CALL_STRING_RESULT: direction=riscv-calls-aarch64-direct-x86 .*switch_delta=7"
          "POLYBENCH_CROSS_CALL_DIRECT_X86_MEMCMP_RESULT: direction=aarch64-calls-riscv-direct-x86-memcmp .*switch_delta=7"
          "POLYBENCH_CROSS_CALL_DIRECT_X86_MEMCMP_RESULT: direction=riscv-calls-aarch64-direct-x86-memcmp .*switch_delta=7"
          "POLYBENCH_CROSS_CALL_DIRECT_X86_MEMOPS_RESULT: direction=aarch64-calls-riscv-direct-x86-memops .*switch_delta=11"
          "POLYBENCH_CROSS_CALL_DIRECT_X86_MEMOPS_RESULT: direction=riscv-calls-aarch64-direct-x86-memops .*switch_delta=11"
        )
        for pattern in "${polybench_patterns[@]}"; do
          if ! grep -Eq "$pattern" "$SERIAL_LOG"; then
            sleep 1
            continue 2
          fi
        done
      fi
      if [[ "$RUN_POLY_BINFMT" == "1" ]]; then
        if ! grep -q "POLYBINFMT_OK" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_ROOT_PENTER: arch=aarch64 generic=1 process=1 .*aarch64-process-argv-envp-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
        if ! grep -Eq "POLYEXEC_ROOT_PENTER: arch=riscv generic=1 process=1 .*riscv-process-argv-envp-real\\.elf" "$SERIAL_LOG"; then
          sleep 1
          continue
        fi
      fi
      if [[ "$RUN_NATIVE_CHECK" == "1" ]]; then
        if [[ "$EXPECT_POLY_CPUID" == "1" ]]; then
          native_markers=(
            "NATIVE_POLY_TRAP_VECTOR_OK"
            "NATIVE_POLY_NO_VECTOR_SIGNALS_OK"
            "NATIVE_POLY_INVALID_GENERIC_CONTROLS_OK"
            "NATIVE_POLY_CPUID_ARCH_STATE_OK"
            "NATIVE_POLY_LANDING_POLICY_OK"
            "NATIVE_POLY_STATE_KEY_OK"
            "NATIVE_POLY_STATE_SAVE_RESTORE_OK"
            "NATIVE_POLY_CROSS_RETURN_XSAVE_OK"
            "NATIVE_POLY_FRONTEND_TLS_OK"
            "NATIVE_POLY_IMPORT_RETURN_XSAVE_OK"
            "NATIVE_POLY_DIRECT_X86_PCALL_OK"
            "NATIVE_POLY_FOREIGN_SIGNATURE_PCALL_OK"
            "NATIVE_POLY_STATE_REGISTER_BANK_OK"
          )
          for marker in "${native_markers[@]}"; do
            if ! grep -q "$marker" "$SERIAL_LOG"; then
              sleep 1
              continue 2
            fi
          done
          if [[ "$REQUIRE_POLY_REAL_XSAVE" == "1" ]]; then
            if ! grep -q "NATIVE_POLY_REAL_XSAVE_OK" "$SERIAL_LOG"; then
              sleep 1
              continue
            fi
          elif ! grep -Eq "NATIVE_POLY_REAL_XSAVE_(OK|SKIPPED)" "$SERIAL_LOG"; then
            sleep 1
            continue
          fi
        else
          if ! grep -q "NATIVE_CPUID_POLY_ABSENT" "$SERIAL_LOG"; then
            sleep 1
            continue
          fi
        fi
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
