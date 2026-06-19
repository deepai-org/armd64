#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOCHS_CPU="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/proc_ctrl.cc"
BOCHS_EXCEPTION="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/exception.cc"
BOCHS_CTRL_XFER64="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/ctrl_xfer64.cc"
BOCHS_FETCHDECODE32="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode32.cc"
BOCHS_FETCHDECODE64="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode64.cc"
BOCHS_OPMAP="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/fetchdecode_opmap_0f3a.cc"
BOCHS_OPCODES="$ROOT_DIR/bochs-prepoly-src/bochs/cpu/decoder/ia_opcodes.def"
BOCHS_DIR="$ROOT_DIR/bochs-prepoly-src/bochs"
README="$ROOT_DIR/README.md"
POLY_ISA_DOC="$ROOT_DIR/docs/poly-isa.md"
POLYPROBE="$ROOT_DIR/tools/programs/polyprobe.c"
POLYBENCH="$ROOT_DIR/tools/programs/polybench.c"
POLYAPP="$ROOT_DIR/tools/programs/polyapp.c"
POLYTHREAD="$ROOT_DIR/tools/programs/polythread.c"
NATIVECHECK="$ROOT_DIR/tools/programs/nativecheck.c"
POLYBINFMT_EXEC="$ROOT_DIR/tools/programs/polybinfmt_exec.c"
POLYEXEC="$ROOT_DIR/tools/runtime/polyexec.c"
RTL_INTERRUPT_BOUNDARY="$ROOT_DIR/rtl/poly_interrupt_boundary.sv"
RTL_FRONTEND_CORE="$ROOT_DIR/rtl/poly_frontend_core.sv"
POLYEXEC_PREEMPT_STRESS_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_preempt_stress_real.c"
POLYEXEC_THREAD_PREEMPT_STRESS_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_thread_preempt_stress_real.c"
POLYEXEC_SMP_ATOMIC_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_smp_atomic_real.c"
POLYEXEC_FPU_TORTURE_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_fpu_torture_real.c"
POLYEXEC_JIT_SELFMOD_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_jit_selfmod_real.c"
POLYEXEC_PROCESS_EXCEPTION_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_exception_real.cc"
POLYEXEC_PROCESS_SETJMP_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_setjmp_real.c"
POLYEXEC_PROCESS_SYSCALL_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_syscall_real.c"
POLYEXEC_PROCESS_SIGNAL_MASK_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_signal_mask_real.c"
POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_signal_handler_real.c"
POLYEXEC_PROCESS_VDSO_TIME_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_process_vdso_time_real.c"
POLYEXEC_AARCH64_VDSO_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_aarch64_vdso.S"
POLYEXEC_AARCH64_VDSO_MAP="$ROOT_DIR/tools/fixtures/polyexec/polyexec_aarch64_vdso.map"
POLYEXEC_AARCH64_VDSO_LD="$ROOT_DIR/tools/fixtures/polyexec/polyexec_aarch64_vdso.ld"
POLYEXEC_PYTHON_EPOLL_SERVER_SRC="$ROOT_DIR/tools/fixtures/polyexec/polyexec_python_epoll_server.py"
POLYEXEC_NONROOT_RUNNER_SRC="$ROOT_DIR/tools/programs/polyexec_nonroot_runner.c"
BOOT_SCRIPT="$ROOT_DIR/scripts/boot.sh"
TMP_DIR="${TMPDIR:-/tmp}/poly-arch-contract.$$"

mkdir -p "$TMP_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

fail() {
  echo "poly architecture contract check failed: $*" >&2
  exit 1
}

extract_function_from_file() {
  local name="$1"
  local file="$2"
  local out="$3"

  awk -v name="$name" '
    index($0, "BX_CPU_C::" name "(") {
      in_sig = 1
    }
    in_sig {
      print
      if (index($0, "{")) {
        in_func = 1
        in_sig = 0
      }
      next
    }
    in_func {
      print
      if ($0 ~ /^}/)
        exit
    }
  ' "$file" > "$out"

  [[ -s "$out" ]] || fail "could not extract $name"
}

extract_function() {
  extract_function_from_file "$1" "$BOCHS_CPU" "$2"
}

assert_contains() {
  local pattern="$1"
  local file="$2"
  local description="$3"

  if ! grep -Eq -- "$pattern" "$file"; then
    fail "$description"
  fi
}

assert_not_contains() {
  local pattern="$1"
  local file="$2"
  local description="$3"

  if grep -Eq -- "$pattern" "$file"; then
    fail "$description"
  fi
}

assert_file_exists() {
  local file="$1"
  local description="$2"

  if [[ ! -e "$file" ]]; then
    fail "$description"
  fi
}

assert_line_order() {
  local first_pattern="$1"
  local second_pattern="$2"
  local file="$3"
  local description="$4"
  local first_line
  local second_line

  first_line=$(grep -nE -- "$first_pattern" "$file" | head -n1 | cut -d: -f1 || true)
  second_line=$(grep -nE -- "$second_pattern" "$file" | head -n1 | cut -d: -f1 || true)
  if [[ -z "$first_line" || -z "$second_line" || "$first_line" -ge "$second_line" ]]; then
    fail "$description"
  fi
}

assert_file_not_exists() {
  local file="$1"
  local description="$2"

  if [[ -e "$file" ]]; then
    fail "$description"
  fi
}

assert_contains "BxOpcodeTable0F3AFC\\[\\].*BX_IA_POLYMODE" "$BOCHS_OPMAP" \
  "x86 poly opcode family must be decoded as BX_IA_POLYMODE, not #UD"
assert_contains "0F 3A.*decoder32_modrm" "$BOCHS_FETCHDECODE32" \
  "32-bit x86 decode must route 0f 3a to the 3-byte opcode table"
assert_contains "0F 3A.*decoder64_modrm" "$BOCHS_FETCHDECODE64" \
  "64-bit x86 decode must route 0f 3a to the 3-byte opcode table"
assert_contains "BX_IA_POLYMODE.*BX_CPU_C::POLYMODE" "$BOCHS_OPCODES" \
  "BX_IA_POLYMODE must dispatch to the dedicated POLYMODE handler"

BXERROR_FUNC="$TMP_DIR/BxError.cc"
UNDEFINED_FUNC="$TMP_DIR/UndefinedOpcode.cc"
POLYMODE_FUNC="$TMP_DIR/POLYMODE.cc"
extract_function "BxError" "$BXERROR_FUNC"
extract_function "UndefinedOpcode" "$UNDEFINED_FUNC"
extract_function "POLYMODE" "$POLYMODE_FUNC"
assert_contains "handle_poly_opcode" "$POLYMODE_FUNC" \
  "POLYMODE must handle x86 poly opcodes through the dedicated decoded path"
assert_not_contains "handle_poly_(opcode|ud)" "$BXERROR_FUNC" \
  "BxError must not mask decoder regressions by accepting poly opcodes from #UD"
assert_not_contains "handle_poly_(opcode|ud)" "$UNDEFINED_FUNC" \
  "UndefinedOpcode must not mask decoder regressions by accepting poly opcodes from #UD"

SYSCALL_FUNC="$TMP_DIR/handle_poly_syscall_trap.cc"
extract_function "handle_poly_syscall_trap" "$SYSCALL_FUNC"
assert_contains "bx_poly_record_syscall_trap" "$SYSCALL_FUNC" \
  "foreign syscalls must capture an OS-neutral architectural event"
assert_contains "deliver_poly_architectural_trap" "$SYSCALL_FUNC" \
  "foreign syscalls must exit through the architectural trap path"
assert_not_contains "write_poly_(aarch64|riscv)_reg|RAX[[:space:]]*=|read_virtual_|write_virtual_|switch[[:space:]]*\\(|case[[:space:]]" \
  "$SYSCALL_FUNC" \
  "foreign syscall handler must not synthesize guest results or decode Linux policy"

ARCH_TRAP_FUNC="$TMP_DIR/deliver_poly_architectural_trap.cc"
extract_function "deliver_poly_architectural_trap" "$ARCH_TRAP_FUNC"
assert_contains "bx_poly_write_v2_event_frame" "$ARCH_TRAP_FUNC" \
  "architectural trap delivery must publish the canonical v2 event frame"
assert_not_contains "bx_poly_monitor_packet_addr|POLY_EVENT_RECORD_FLAG_MONITOR_MEMORY|write_virtual_qword" \
  "$ARCH_TRAP_FUNC" \
  "architectural trap delivery must not retain legacy monitor-packet compatibility writes"

IMPORT_CALL_FUNC="$TMP_DIR/handle_poly_import_call.cc"
extract_function "handle_poly_import_call" "$IMPORT_CALL_FUNC"
assert_contains "read_poly_aarch64_reg\\(0, &arg0\\)" "$IMPORT_CALL_FUNC" \
  "AArch64 import call gate must capture native ABI argument lane x0"
assert_contains "read_poly_aarch64_reg\\(5, &arg5\\)" "$IMPORT_CALL_FUNC" \
  "AArch64 import call gate must capture native ABI argument lane x5"
assert_contains "read_poly_aarch64_reg\\(6, &arg6\\)" "$IMPORT_CALL_FUNC" \
  "AArch64 unresolved import traps must preserve native ABI argument lane x6"
assert_contains "read_poly_aarch64_reg\\(7, &arg7\\)" "$IMPORT_CALL_FUNC" \
  "AArch64 unresolved import traps must preserve native ABI argument lane x7"
assert_contains "read_poly_riscv_reg\\(10, &arg0\\)" "$IMPORT_CALL_FUNC" \
  "RISC-V import call gate must capture native ABI argument lane a0"
assert_contains "read_poly_riscv_reg\\(15, &arg5\\)" "$IMPORT_CALL_FUNC" \
  "RISC-V import call gate must capture native ABI argument lane a5"
assert_contains "read_poly_riscv_reg\\(16, &arg6\\)" "$IMPORT_CALL_FUNC" \
  "RISC-V unresolved import traps must preserve native ABI argument lane a6"
assert_contains "read_poly_riscv_reg\\(17, &arg7\\)" "$IMPORT_CALL_FUNC" \
  "RISC-V unresolved import traps must preserve native ABI argument lane a7"
assert_contains "bx_poly_record_import_trap" "$IMPORT_CALL_FUNC" \
  "unresolved descriptor imports must capture an architectural import event"
assert_contains "arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7" "$IMPORT_CALL_FUNC" \
  "unresolved descriptor imports must record all eight native ABI argument lanes"
assert_contains "deliver_poly_architectural_trap" "$IMPORT_CALL_FUNC" \
  "unresolved descriptor imports must exit through the architectural trap path"
assert_contains 'cmpq \$88, %r13' "$NATIVECHECK" \
  "nativecheck x86 trap vector must verify delivered trap argument lane 6"
assert_contains 'cmpq \$99, %r14' "$NATIVECHECK" \
  "nativecheck x86 trap vector must verify delivered trap argument lane 7"
assert_contains 'expect_event_packet(_args)?\("aarch64 import"' "$NATIVECHECK" \
  "nativecheck must verify AArch64 unresolved import trap source mode from v2 event frames"
assert_contains 'expect_event_packet(_args)?\("riscv import"' "$NATIVECHECK" \
  "nativecheck must verify RISC-V unresolved import trap source mode from v2 event frames"
assert_contains 'expect_event_packet(_args)?\("riscv compressed import"' "$NATIVECHECK" \
  "nativecheck must exercise RISC-V unresolved import trap argument lanes 6 and 7 from v2 event frames"
assert_contains "POLY_V2_EVENT_ARG_COUNT[[:space:]]*=[[:space:]]*8" \
  "$ROOT_DIR/tools/include/polycpuid.h" \
  "v2 event-frame ABI must carry eight ABI argument lanes"
assert_contains "OS-neutral v2 event frames" "$README" \
  "README must describe OS-neutral v2 event frames"
assert_not_contains "monitor packet addresses" "$README" \
  "README must not describe legacy monitor-packet address rules as active ISA"
assert_not_contains "six[[:space:]]+ABI arguments" "$README" \
  "README must not describe the old six-argument POLYTRAP packet"
assert_contains "pcall-needed-tls-external-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency-exported TLS relocations"
assert_contains "pcall-abs-needed-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover absolute DT_NEEDED paths"
assert_contains "needed\\[0\\][[:space:]]*==[[:space:]]*'/'" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must accept absolute DT_NEEDED paths"
assert_contains "pcall-origin-needed-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover ORIGIN token expansion in DT_NEEDED strings"
assert_contains "pcall-platform-needed-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover PLATFORM token expansion in DT_NEEDED strings"
assert_contains "pcall-lib-needed-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover LIB token expansion in DT_NEEDED strings"
assert_contains "expanded_needed" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must expand dynamic-string tokens in DT_NEEDED entries"
assert_contains "pcall-abs-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover absolute DT_RUNPATH dependency directories"
assert_contains "build_runpath_entry_needed_path" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must accept absolute DT_RUNPATH dependency directories"
assert_contains "pcall-rpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover old-style DT_RPATH dependency directories"
assert_contains "DT_RPATH" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must parse old-style DT_RPATH dependency directories"
assert_contains "pcall-colon-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover colon-separated DT_RUNPATH fallback directories"
assert_contains "runpath\\[end\\][[:space:]]*!=[[:space:]]*':'" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must tokenize colon-separated DT_RUNPATH entries"
assert_contains "pcall-braced-origin-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover braced ORIGIN DT_RUNPATH directories"
assert_contains "\\$\\{ORIGIN\\}" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must accept braced ORIGIN DT_RUNPATH entries"
assert_contains "pcall-relative-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover relative DT_RUNPATH dependency directories"
assert_contains "expand_runpath_entry" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must accept relative DT_RUNPATH dependency directories"
assert_contains "pcall-lib-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover LIB token expansion in DT_RUNPATH directories"
assert_contains "pcall-braced-lib-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover braced LIB token expansion in DT_RUNPATH directories"
assert_contains '\$LIB' "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must expand LIB dynamic-string tokens in DT_RUNPATH entries"
assert_contains "pcall-platform-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover PLATFORM token expansion in DT_RUNPATH directories"
assert_contains "pcall-braced-platform-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover braced PLATFORM token expansion in DT_RUNPATH directories"
assert_contains '\$PLATFORM' "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must expand PLATFORM dynamic-string tokens in DT_RUNPATH entries"
assert_contains "pcall-ld-library-path-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover LD_LIBRARY_PATH dependency lookup"
assert_contains "LD_LIBRARY_PATH" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must honor LD_LIBRARY_PATH for foreign dependency lookup"
assert_contains "pcall-ld-platform-path-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover PLATFORM token expansion in LD_LIBRARY_PATH"
assert_contains "pcall-ld-origin-path-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover ORIGIN token expansion in LD_LIBRARY_PATH"
assert_contains "pcall-ld-prefer-runpath-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover LD_LIBRARY_PATH precedence over DT_RUNPATH"
assert_contains "missing-envdeps" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover colon-separated LD_LIBRARY_PATH fallback"
assert_contains "pcall-runpath-prefer-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover DT_RUNPATH precedence over fallback directories"
assert_contains "found_needed" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must prefer declared DT_RUNPATH/DT_RPATH before fallback directories"
assert_contains "MAX_NEEDED_DEPS[[:space:]]*=[[:space:]]*32" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must support dependency fans larger than eight libraries"
assert_contains "pcall-many-needed-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency fans larger than eight libraries"
assert_contains "RELOC_BASE_DEP_TLS_OFFSET" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must have a relocation base for dependency-exported TLS symbols"
assert_contains "pcall-versioned-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency-exported symbol versions"
assert_contains "DT_VERNEED" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must parse GNU version requirements"
assert_contains "symbol_export_version_matches" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must match dependency symbol versions"
assert_contains "DT_SONAME" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must parse dependency SONAMEs"
assert_contains "dependency_matches_version_file" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must match GNU version provider filenames"
assert_contains "symbol_is_dependency_export" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must filter dependency exports by ELF binding and visibility"
assert_contains "ELF64_ST_VISIBILITY" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must inspect ELF symbol visibility"
assert_contains "pcall-root-export-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency relocations bound to root exports"
assert_contains "RELOC_BASE_ROOT_LOAD_BIAS" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must have a relocation base for root-object exports"
assert_contains "resolve_root_symbol" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must resolve dependency relocations against root exports"
assert_contains "pcall-root-tls-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency TLS relocations bound to root exports"
assert_contains "RELOC_BASE_ROOT_TLS_OFFSET" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must have a relocation base for root-object TLS exports"
assert_contains "resolve_root_tls_symbol" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must resolve dependency TLS relocations against root exports"
assert_contains "pcall-root-ifunc-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover dependency relocations bound to root IFUNC exports"
assert_contains "RELOC_BASE_ROOT_IFUNC" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must have a relocation base for root-object IFUNC exports"
assert_contains "pcall-root-weak-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover weak dependency relocations bound to root exports"
assert_contains "resolve_root_symbol\\(program, symbol_name, &required_version" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall weak undefined relocations must search root exports before resolving to zero"
assert_contains "pcall-gnu-unique-real" "$ROOT_DIR/scripts/boot.sh" \
  "polycall boot matrix must cover GNU unique object exports"
assert_contains "STB_GNU_UNIQUE" "$ROOT_DIR/tools/runtime/polycall.c" \
  "polycall loader must treat GNU unique object symbols as exportable dynamic symbols"
assert_not_contains "requires_software_descriptor|is_x86_descriptor" "$BOCHS_CPU" \
  "import descriptor/trap routing must not use CPU-side helper classification"
assert_not_contains "BX_POLY_IMPORT_FUNC_(STR|MEM|BCMP|BCOPY|BZERO|RAWMEMCHR|STACK_CHK|ERRNO|GET[A-Z]|MALLOC|CALLOC|REALLOC|FREE|ATEXIT|CXA|POSIX|ALIGNED)" \
  "$IMPORT_CALL_FUNC" \
  "import call gate must not encode libc/process/helper-specific argument policy"
assert_not_contains "BX_POLY_IMPORT_FUNC_ATOMIC_STORE_16" "$IMPORT_CALL_FUNC" \
  "AArch64 __int128 argument alignment must be handled by runtime descriptors, not CPU import mapping"
assert_not_contains "uses_fp128_.*arg" "$IMPORT_CALL_FUNC" \
  "RISC-V __float128 argument reconstruction must be handled by runtime descriptors, not CPU import mapping"
assert_not_contains "bx_poly_import_uses_x86_stack_args|bx_poly_import_x86_returns_i128|bx_poly_import_x86_returns_fp128" "$BOCHS_CPU" \
  "x86 import stack and return ABI shape must come from runtime descriptor flags"

assert_not_contains "poly_raw: import x86 call|BX_POLY_IMPORT_X86_ADD_HELPER_SIZE|BX_POLY_IMPORT_FUNC_X86_ADD" \
  "$BOCHS_CPU" \
  "legacy fixed x86 import helper fallback must stay removed"

assert_contains "bx_poly_aarch64_barrier_name" "$BOCHS_CPU" \
  "AArch64 barrier decoder must remain present for the x86 TSO contract"
assert_contains "0xd503309f" "$BOCHS_CPU" \
  "AArch64 DSB barrier mask must remain decoded"
assert_contains "0xd50330bf" "$BOCHS_CPU" \
  "AArch64 DMB barrier mask must remain decoded"
assert_contains "0xd50330df" "$BOCHS_CPU" \
  "AArch64 ISB barrier mask must remain decoded"
assert_contains "bx_poly_riscv_fence_name" "$BOCHS_CPU" \
  "RISC-V fence decoder must remain present for the x86 TSO contract"
assert_contains "0x0000000f" "$BOCHS_CPU" \
  "RISC-V FENCE must remain decoded"
assert_contains "0x0000100f" "$BOCHS_CPU" \
  "RISC-V FENCE.I must remain decoded"
assert_contains "aarch64 .*x86-tso no-op" "$BOCHS_CPU" \
  "AArch64 barriers must remain explicit x86-TSO no-ops"
assert_contains "riscv .*x86-tso no-op" "$BOCHS_CPU" \
  "RISC-V fences must remain explicit x86-TSO no-ops"
assert_contains "0xd5033fbf|0xd5033f9f|0xd5033fdf" "$POLYPROBE" \
  "polyprobe must exercise AArch64 DMB/DSB/ISB barrier decode"
assert_contains "0x0ff0000f" "$POLYPROBE" \
  "polyprobe must exercise RISC-V FENCE decode"
assert_contains "0x0000100f" "$POLYPROBE" \
  "polyprobe must exercise RISC-V FENCE.I decode"

INTERRUPT_FUNC="$TMP_DIR/poly_interrupt_enter.cc"
RESTORE_FUNC="$TMP_DIR/poly_restore_raw_return_to_user.cc"
IRET64_FUNC="$TMP_DIR/IRET64.cc"
SYSRET_FUNC="$TMP_DIR/SYSRET.cc"
SYSEXIT_FUNC="$TMP_DIR/SYSEXIT.cc"
extract_function "poly_interrupt_enter" "$INTERRUPT_FUNC"
extract_function "poly_restore_raw_return_to_user" "$RESTORE_FUNC"
extract_function_from_file "IRET64" "$BOCHS_CTRL_XFER64" "$IRET64_FUNC"
extract_function "SYSRET" "$SYSRET_FUNC"
extract_function "SYSEXIT" "$SYSEXIT_FUNC"
assert_contains "CPL[[:space:]]*!=[[:space:]]*3" "$INTERRUPT_FUNC" \
  "raw interrupt capture must be restricted to userspace foreign execution"
assert_contains "bx_poly_is_raw_mode\\(bx_poly_current_mode\\)" "$INTERRUPT_FUNC" \
  "raw interrupt capture must only arm for raw foreign frontends"
assert_contains "bx_poly_interrupted_raw_valid[[:space:]]*=[[:space:]]*true" "$INTERRUPT_FUNC" \
  "raw interrupt capture must mark interrupted foreign state valid"
assert_contains "bx_poly_interrupted_raw_mode[[:space:]]*=[[:space:]]*bx_poly_current_mode" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save interrupted foreign frontend mode"
assert_contains "bx_poly_interrupted_raw_rip[[:space:]]*=[[:space:]]*RIP" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save interrupted foreign RIP"
assert_contains "bx_poly_save_current_reg_state" "$INTERRUPT_FUNC" \
  "raw interrupt capture must save synthetic foreign state before x86 kernel entry"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*BX_POLY_MODE_X86" "$INTERRUPT_FUNC" \
  "raw interrupt capture must route interrupt handling through x86 decode"
assert_contains "bx_poly_update_raw_owner" "$INTERRUPT_FUNC" \
  "raw interrupt capture must update the keyed raw owner state"
assert_not_contains "bx_poly_spill_buffer|bx_poly_spill_resume_rip|bx_poly_auto_spill_" "$INTERRUPT_FUNC" \
  "raw interrupt capture must not retain the retired auto-spill trampoline path"
assert_contains "bx_poly_state_dirty" "$BOCHS_CPU" \
  "Bochs model must track the Poly state dirty bit"
assert_contains "0x3ca06800" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support AdvSIMD register-offset STR Q used by real glibc ld.so"
assert_contains "0x3ce06800" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support AdvSIMD register-offset LDR Q used by real glibc ld.so"
assert_contains "0x7ee08800" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support scalar CMGE D,#0 used by the C++ unwinder runtime"
assert_contains "0x6e20ac00" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support UMINP v.16b used by unmodified python3 startup"
assert_contains "0x2e204400" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support USHL v.2d used by unmodified python3 startup"
assert_contains "0x0e212800" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support XTN v.2s used by unmodified python3 startup"
assert_contains "0x9b200000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support SMADDL used by unmodified python3 startup"
assert_contains "0x9b208000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support SMSUBL used by unmodified python3 startup"
assert_contains "0x1e654000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support FRINTM D used by unmodified python3 startup"
assert_contains "0x1e64c000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support FRINTP D used by unmodified python3 startup"
assert_contains "0x69400000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support LDPSW used by unmodified python3 startup"
assert_contains "op && cmode == 14" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support scalar MOVI D used by unmodified python3 startup"
assert_contains "0x9eae0000" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support FMOV X,V.d[1] used by unmodified python3 startup"
assert_contains "0x1e600400" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support FCCMP D used by unmodified python3 startup"
assert_contains "write_virtual_byte\\(BX_SEG_REG_DS, addr, \\(Bit8u\\) value\\)" "$BOCHS_CPU" \
  "Bochs AArch64 raw frontend must support scalar FP STR B used by unmodified python3 startup"
assert_contains "ip link set lo up" "$BOOT_SCRIPT" \
  "focused Python epoll boot fixture must enable guest loopback before TCP loopback connect"
assert_contains "chmod 1777 /tmp" "$BOOT_SCRIPT" \
  "boot initramfs must provide a writable /tmp for the non-root polyexec proof"
assert_contains "chdir\\(\"/tmp\"\\)" "$POLYEXEC_NONROOT_RUNNER_SRC" \
  "non-root polyexec runner must use writable scratch space after dropping privileges"
assert_contains "state_dirty_i" "$RTL_INTERRUPT_BOUNDARY" \
  "RTL interrupt boundary must consume the Poly state dirty bit"
assert_contains "spill_full_state_o" "$RTL_INTERRUPT_BOUNDARY" \
  "RTL interrupt boundary must expose full-state spill decisions"
assert_contains "spill_header_only_o" "$RTL_INTERRUPT_BOUNDARY" \
  "RTL interrupt boundary must expose header-only spill decisions"
assert_contains "poly_state_dirty_q" "$RTL_FRONTEND_CORE" \
  "RTL frontend core must maintain an internal Poly state dirty bit"
assert_not_contains "BX_POLY_X86_CTRL_PRESTORE" "$BOCHS_CPU" \
  "x86 control path must not retain the deprecated PRESTORE opcode"
assert_not_contains "BX_POLY_X86_CTRL_SPILL_PTR_SET" "$BOCHS_CPU" \
  "Bochs must not implement the retired raw spill pointer control"
assert_contains "BX_POLY_X86_CTRL_EVENT_PTR_SET" "$BOCHS_CPU" \
  "Bochs must implement the v2 event-frame control"
assert_contains "frame[[:space:]]*==[[:space:]]*0 && bytes[[:space:]]*==[[:space:]]*0" "$BOCHS_CPU" \
  "Bochs must allow PSET_EVENT_PTR(0, 0) to unregister v2 event frames"
assert_not_contains "BX_POLY_X86_CTRL_SPILL_DESC_SET" "$BOCHS_CPU" \
  "Bochs must not implement the retired v2 spill descriptor control"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_DUMP_STATE" "$BOCHS_CPU" \
  "Bochs must implement the v2 debug-note export control"
assert_contains "export_poly_v2_debug_note" "$BOCHS_CPU" \
  "Bochs must export the v2 debug-note blob through a dedicated helper"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_MEM_PROBE_RANGE" "$BOCHS_CPU" \
  "Bochs must implement the v2 memory-probe control"
assert_contains "probe_poly_v2_memory_range" "$BOCHS_CPU" \
  "Bochs must probe v2 memory ranges through a dedicated helper"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_DERIVE_STATE" "$BOCHS_CPU" \
  "Bochs must implement the v2 state-derivation control"
assert_contains "derive_poly_v2_state" "$BOCHS_CPU" \
  "Bochs must derive child frontend state through a dedicated helper"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_COMPLETE_EVENT" "$BOCHS_CPU" \
  "Bochs must implement the v2 event-completion control"
assert_contains "complete_poly_v2_event" "$BOCHS_CPU" \
  "Bochs must complete frontend events through a dedicated helper"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_FENCE" "$BOCHS_CPU" \
  "Bochs must implement the v2 shared-memory fence control"
assert_contains "op[[:space:]]*==[[:space:]]*BX_POLY_X86_CTRL_MONITOR_ENTRY_SET" "$BOCHS_CPU" \
  "Bochs must implement the v2 monitor-entry stack/frame contract"
assert_contains "BX_POLY_CPUID_V2_IMPLEMENTED_FEATURES" "$BOCHS_CPU" \
  "Bochs v2 CPUID must advertise only implemented v2 features"
assert_not_contains "BX_POLY_CPUID_V2_FEATURES" "$BOCHS_CPU" \
  "Bochs v2 CPUID must not advertise every draft v2 feature as implemented"
assert_contains "POLY_CPUID_V2_IMPLEMENTED_FEATURES" "$ROOT_DIR/tools/include/polycpuid.h" \
  "public CPUID contract must expose an implemented v2 feature mask"
assert_contains "regs\\.ebx[[:space:]]*=[[:space:]]*POLY_CPUID_V2_IMPLEMENTED_FEATURES" "$ROOT_DIR/tools/include/polycpuid.h" \
  "public v2 CPUID expectation must use the implemented feature mask"
assert_not_contains "POLY_CPUID_V2_IMPLEMENTED_FEATURES = .*1U << 1([^0-9]|$)" "$ROOT_DIR/tools/include/polycpuid.h" \
  "public v2 CPUID must not advertise the deprecated spill descriptor as a production feature"
assert_contains "POLY_CPUID_V2_REQUIRED_FEATURES = \\(1U << 0\\)" "$ROOT_DIR/tools/include/polycpuid.h" \
  "public v2 CPUID must require only canonical event frames"
assert_not_contains "BX_POLY_CPUID_V2_IMPLEMENTED_FEATURES = .*1U << 1([^0-9]|$)" "$BOCHS_CPU" \
  "Bochs v2 CPUID must not advertise the deprecated spill descriptor as a production feature"
assert_contains "BX_POLY_CPUID_V2_REQUIRED_FEATURES = \\(1U << 0\\)" "$BOCHS_CPU" \
  "Bochs v2 CPUID must require only canonical event frames"
assert_contains "poly_cpuid_expected_v2_leaf" "$POLYEXEC" \
  "userspace monitor must validate the v2 CPUID discovery leaf"
assert_contains "poly_cpuid_expected_v2_leaf" "$NATIVECHECK" \
  "nativecheck must validate the v2 CPUID discovery leaf"
assert_contains "run_poly_v2_debug_note_probe" "$NATIVECHECK" \
  "nativecheck must exercise PDUMP_STATE before the debug-note bit is advertised"
assert_contains "run_poly_v2_mem_probe_range_probe" "$NATIVECHECK" \
  "nativecheck must exercise PMEM_PROBE_RANGE before the memory-probe bit is advertised"
assert_contains "run_poly_v2_derive_state_probe" "$NATIVECHECK" \
  "nativecheck must exercise PDERIVE_STATE before the derive bit is advertised"
assert_contains "run_poly_v2_complete_event_probe" "$NATIVECHECK" \
  "nativecheck must retain a PCOMPLETE_EVENT probe for when the completion bit is advertised"
assert_contains "run_poly_v2_fence_probe" "$NATIVECHECK" \
  "nativecheck must exercise PFENCE when the shared-memory fence bit is advertised"
assert_contains "run_poly_v2_monitor_entry_probe" "$NATIVECHECK" \
  "nativecheck must exercise the monitor-entry contract when the bit is advertised"
assert_contains "poly_seccomp_preflight_syscall" "$POLYEXEC" \
  "userspace monitor must evaluate guest seccomp policy before host syscall dispatch"
assert_contains "poly_seccomp_dispatch_control" "$POLYEXEC" \
  "userspace monitor must intercept guest seccomp filter installation"
assert_contains "aarch64-process-seccomp-policy-real\\.elf" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must run an AArch64 seccomp policy fixture"
assert_contains "POLY_SECCOMP_POLICY_OK" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must require the seccomp policy fixture marker"
assert_contains "polyexec_process_io_uring_real\\.c" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must build a real io_uring shared-ring fixture"
assert_contains "aarch64-process-io-uring-real\\.elf=42" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must run the AArch64 io_uring shared-ring fixture"
assert_contains "riscv-process-io-uring-real\\.elf=42" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must run the RISC-V io_uring shared-ring fixture"
assert_contains "POLY_IO_URING_NOP_OK" "$ROOT_DIR/scripts/boot.sh" \
  "boot syscall coverage must require the io_uring shared-ring marker"
assert_contains "RUN_POLY_ALPINE_FIO_IO_URING" "$ROOT_DIR/scripts/boot.sh" \
  "boot coverage must expose an opt-in fio io_uring workload"
assert_contains "fio --name=poly-fio-uring" "$ROOT_DIR/scripts/boot.sh" \
  "fio workload must explicitly run through the io_uring engine"
assert_contains "POLY_ALPINE_FIO_IO_URING_OK" "$ROOT_DIR/scripts/boot.sh" \
  "boot coverage must require the fio io_uring marker"
assert_contains "boot-poly-alpine-fio-io-uring" "$ROOT_DIR/Makefile" \
  "Makefile must expose a dedicated fio io_uring boot target"
assert_contains "BX_POLY_V2_DERIVE_FLAG_ACTIVATE_DST" "$BOCHS_CPU" \
  "PDERIVE_STATE must expose an activation flag for resume imports"
assert_contains "poly_ud: derived v2 activation enter" "$BOCHS_CPU" \
  "PDERIVE_STATE ACTIVATE_DST must atomically enter the imported raw frontend"
assert_contains "RIP[[:space:]]*=[[:space:]]*saved_rip" "$BOCHS_CPU" \
  "PDERIVE_STATE ACTIVATE_DST must jump directly to the imported foreign PC"
assert_contains "bx_poly_derive_resume_target_valid[[:space:]]*=[[:space:]]*false" "$BOCHS_CPU" \
  "PDERIVE_STATE ACTIVATE_DST must not leave a racy pending PENTER resume"
assert_contains "sigaction\\(SIGSEGV" "$POLYEXEC" \
  "userspace monitor must install a SIGSEGV handler for Poly fault translation"
assert_contains "poly_write_fatal_debug_note" "$POLYEXEC" \
  "userspace monitor must synthesize a v2 debug-note artifact on fatal Poly faults"
assert_contains "PT_NOTE" "$POLYEXEC" \
  "fatal Poly debug artifacts must be wrapped in an ELF note container"
assert_contains "NT_SIGINFO" "$POLYEXEC" \
  "fatal Poly ELF cores must include signal information for debuggers"
assert_contains "NT_FILE" "$POLYEXEC" \
  "fatal Poly ELF cores must include mapped-file metadata for debuggers"
assert_contains "PT_LOAD" "$POLYEXEC" \
  "fatal Poly ELF cores must include loadable memory segments for debuggers"
assert_contains "POLYEXEC_FATAL_DEBUG_NOTE_DIR" "$POLYEXEC" \
  "fatal Poly debug-note output must be opt-in through a runtime directory"
assert_contains "poly_abi_descriptor_signature_kind" "$POLYEXEC" \
  "userspace monitor must use the v2 software-owned ABI descriptor API"
assert_not_contains "poly_abi_legacy_bridge" "$POLYEXEC" \
  "userspace monitor must not depend on the removed legacy ABI bridge API"
assert_file_exists "$ROOT_DIR/tools/runtime/abi/poly_abi_descriptor.c" \
  "v2 ABI descriptor decoder source must exist"
assert_file_not_exists "$ROOT_DIR/tools/runtime/abi/poly_abi_legacy_bridge.c" \
  "legacy ABI bridge decoder source must stay removed"
assert_not_contains "POLY_OP_SPILL_PTR_SET" "$POLYEXEC" \
  "userspace monitor must not use legacy raw spill pointer setup"
assert_not_contains "POLY_OP_SPILL_PTR_SET" "$POLYBINFMT_EXEC" \
  "binfmt helper must not use legacy raw spill pointer setup"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYBINFMT_EXEC" \
  "binfmt helper must clear the v2 canonical event frame registration"
assert_not_contains "POLY_OP_SPILL_DESC_SET" "$POLYBINFMT_EXEC" \
  "binfmt helper must not use the retired v2 spill descriptor registration"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET" "$POLYBINFMT_EXEC" \
  "binfmt helper must not use legacy monitor-packet control"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYEXEC" \
  "userspace monitor must register a v2 canonical event frame"
assert_not_contains "POLY_OP_SPILL_DESC_SET|populate_poly_v2_spill_descriptor|poly_auto_spill_state" "$POLYEXEC" \
  "userspace monitor must not retain retired v2 spill descriptor plumbing"
assert_contains "poly_v2_event_to_runtime_packet" "$POLYEXEC" \
  "userspace monitor must consume the v2 event frame as the default trap dispatch payload"
assert_contains "polyapp_event_frame_to_packet" "$POLYAPP" \
  "raw app harness must consume v2 event frames for trap-vector dispatch"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYAPP" \
  "raw app harness must register a v2 canonical event frame"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|polyapp_monitor_packet" "$POLYAPP" \
  "raw app harness must not depend on legacy monitor-packet publication"
assert_contains "polybench_event_frame_to_packet" "$POLYBENCH" \
  "polybench trap-vector dispatch must consume v2 event frames"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYBENCH" \
  "polybench must register a v2 canonical event frame"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|polybench_monitor_packet" "$POLYBENCH" \
  "polybench must not depend on legacy monitor-packet publication"
assert_contains "polyprobe_event_frame_to_packet" "$POLYPROBE" \
  "polyprobe trap-vector dispatch must consume v2 event frames"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYPROBE" \
  "polyprobe must register a v2 canonical event frame"
assert_contains "POLY_PROBE_EVENT_FRAMES_OK" "$POLYPROBE" \
  "polyprobe must report v2 event-frame trap-vector coverage"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|POLY_OP_MONITOR_PACKET_GET|monitor_packet" "$POLYPROBE" \
  "polyprobe must not depend on legacy monitor-packet controls"
assert_contains "POLY_PROBE_EVENT_FRAMES_OK" "$BOOT_SCRIPT" \
  "polyprobe boot assertions must wait for v2 event-frame coverage"
assert_contains "polythread_event_frame_to_packet" "$POLYTHREAD" \
  "polythread trap-vector dispatch must consume v2 event frames"
assert_contains "POLY_OP_EVENT_PTR_SET" "$POLYTHREAD" \
  "polythread must register a v2 canonical event frame"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|polythread_monitor_packet" "$POLYTHREAD" \
  "polythread must not depend on legacy monitor-packet publication"
assert_contains "refresh_poly_trap_event_frame" "$POLYEXEC" \
  "userspace monitor must register v2 event frames even without auto-spill"
assert_not_contains "POLYEXEC_AUTO_SPILL|polyexec_use_auto_spill" "$POLYEXEC" \
  "userspace monitor must not retain the deprecated auto-spill runtime flag"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|POLY_OP_MONITOR_PACKET_GET|poly_monitor_packet|read_poly_monitor_packet" "$POLYEXEC" \
  "userspace monitor must not retain the legacy monitor-packet trap dispatch fallback"
assert_not_contains "POLY_OP_MONITOR_PACKET_SET|POLY_OP_MONITOR_PACKET_GET|poly_monitor_packet_(set|get)" "$NATIVECHECK" \
  "nativecheck must not exercise retired monitor-packet control opcodes"
assert_not_contains "poly_auto_spill_resume_info|poly_auto_spill_resume_trampoline" "$POLYEXEC" \
  "userspace monitor must not retain the retired auto-spill resume trampoline"
assert_not_contains "POLY_OP_PRESTORE" "$POLYEXEC" \
  "userspace monitor must not use the deprecated PRESTORE opcode"
assert_contains "POLY_OP_DERIVE_STATE" "$POLYEXEC" \
  "userspace monitor must use PDERIVE_STATE for OS-neutral state derivation"
assert_contains "POLY_OP_MEM_PROBE_RANGE" "$POLYEXEC" \
  "userspace monitor must use v2 memory probing for guest range validation"
assert_contains "POLY_CPUID_V2_FEATURE_EVENT_COMPLETE" "$POLYEXEC" \
  "userspace monitor must gate unproven v2 event completion on CPUID"
assert_contains "!aarch64_rt_sigreturn" "$POLYEXEC" \
  "rt_sigreturn imports must bypass PCOMPLETE_EVENT and resume only through PDERIVE_STATE"
assert_contains "state->trap_restore.x86_gpr\\[15\\] = monitor_sp" "$POLYEXEC" \
  "rt_sigreturn imports must preserve the native monitor stack separately from the guest SP"
assert_contains "frame.saved_trap_restore.x86_gpr\\[15\\] != 0" "$POLYEXEC" \
  "rt_sigreturn imports must derive the monitor stack from the saved trap frame contract"
assert_not_contains "native_return_sp" "$POLYEXEC" \
  "rt_sigreturn imports must not treat the guest signal SP as a native return stack"
assert_not_contains "frame.signum == SIGCHLD[[:space:]]*\\)[[:space:]]*poly_clear_native_return_state" "$POLYEXEC" \
  "rt_sigreturn imports must not special-case SIGCHLD by erasing saved native-return metadata"
assert_contains "poly_clear_foreign_return_transition_state\\(state\\)" "$POLYEXEC" \
  "redirected rt_sigreturn imports must preserve native-return metadata while clearing foreign transition frames"
assert_contains "restore_entry_stack_for_import_state" "$POLYEXEC" \
  "rt_sigreturn imports must not overwrite the imported guest SP with the monitor stack"
assert_contains "POLY_CPUID_V2_FEATURE_SHARED_MEMORY_FENCE" "$POLYEXEC" \
  "userspace monitor must gate io_uring/shared-ring ordering on PFENCE CPUID"
assert_contains "POLY_CPUID_V2_FEATURE_MONITOR_ENTRY_FRAME" "$POLYEXEC" \
  "userspace monitor must use the v2 monitor-entry stack/frame contract"
assert_not_contains "poly_prefault_(executable|writable)_mappings|poly_prefault_(executable|writable)_mapping_line" "$POLYEXEC" \
  "userspace monitor must not retain broad /proc/self/maps prefault walkers"
assert_contains "POLYEXEC_SIGNAL: signo=0x" "$POLYEXEC" \
  "userspace monitor must report fatal signal diagnostics without auto-spill"
assert_contains "POLYEXEC_FATAL_DEBUG_NOTE" "$POLYEXEC" \
  "userspace monitor must emit fatal debug-note diagnostics without auto-spill"
assert_contains "selftest-pagefault" "$POLYEXEC" \
  "userspace monitor must expose a deliberate Poly page-fault self-test"
assert_contains "POLY_FATAL_DEBUG_NOTE_ELF_OK" "$BOOT_SCRIPT" \
  "page-fault boot coverage must verify the fatal v2 debug-note ELF artifact"
assert_contains "polyexec_process_crash_real\\.c" "$BOOT_SCRIPT" \
  "boot coverage must build real crashing process fixtures for core validation"
assert_contains "RUN_POLY_GDB_CORE_VALIDATION" "$BOOT_SCRIPT" \
  "boot coverage must expose opt-in GDB core validation"
assert_contains "gdb-multiarch" "$BOOT_SCRIPT" \
  "host-side core validation must use gdb-multiarch"
assert_contains "POLY_GDB_CORE_VALIDATION_HOST_OK" "$BOOT_SCRIPT" \
  "boot coverage must require host-side GDB core validation"
assert_contains "poly_crash_leaf" "$BOOT_SCRIPT" \
  "host-side core validation must require a symbolized guest crash frame"
assert_contains "info proc mappings" "$BOOT_SCRIPT" \
  "host-side core validation must require mapped executable visibility"
assert_contains "NT_SIGINFO" "$BOOT_SCRIPT" \
  "host-side core validation must require signal-info notes"
assert_contains "NT_FILE" "$BOOT_SCRIPT" \
  "host-side core validation must require mapped-file notes"
assert_contains "boot-poly-gdb-core-validation" "$ROOT_DIR/Makefile" \
  "Makefile must expose a dedicated GDB core validation boot target"
assert_not_contains "POLYEXEC_AUTO_SPILL_STATUS|poly_auto_spill_count_status" "$POLYEXEC" \
  "userspace monitor must not retain retired auto-spill profiling counters"
assert_contains "PT_INTERP" "$POLYEXEC" \
  "userspace monitor must parse ELF interpreter metadata for dynamic process binaries"
assert_contains "interp=%s" "$POLYEXEC" \
  "userspace monitor must report process-mode ELF interpreter metadata"
assert_contains "AT_BASE, at_base" "$POLYEXEC" \
  "userspace monitor must pass the foreign interpreter load bias in auxv"
assert_contains "resolve_process_interpreter_path" "$POLYEXEC" \
  "userspace monitor must resolve copied foreign PT_INTERP loaders"
assert_contains "POLYEXEC_INTERP_LOAD" "$POLYEXEC" \
  "userspace monitor must report real foreign interpreter handoff"
assert_contains "poly_process_uses_real_interpreter" "$POLYEXEC" \
  "process mode must select real ld.so handoff through an explicit helper"
assert_contains "dynamic-libc" "$POLYEXEC" \
  "real ld.so handoff must be limited to the explicit dynamic smoke invocation"
assert_contains "poly_prefault_range" "$POLYEXEC" \
  "real ld.so mmap/mprotect translation must prefault mapped pages for raw Poly memory access"
if awk '
    /^static void poly_prefault_range\(/ { in_func = 1 }
    in_func && /polyexec_use_auto_spill/ { found = 1 }
    in_func && /^}/ { exit found ? 0 : 1 }
    END { if (!in_func) exit 1 }
  ' "$POLYEXEC"; then
  fail "raw Poly prefaulting must not depend on deprecated auto-spill being enabled"
fi
assert_contains "fstat\\(\\(int\\) arg4" "$POLYEXEC" \
  "file-backed mmap prefaulting must avoid SIGBUS beyond the mapped file bytes"
assert_contains "aarch64-real-python3\\.elf" "$POLYEXEC" \
  "process mode must allow unmodified AArch64 python3 to use its real PT_INTERP loader"
assert_contains "riscv-real-python3\\.elf" "$POLYEXEC" \
  "process mode must allow unmodified RISC-V python3 to use its real PT_INTERP loader"
assert_contains "aarch64-process-exception-real\\.elf" "$POLYEXEC" \
  "process mode must allow the AArch64 C++ exception fixture to use real ld.so"
assert_contains "aarch64-process-setjmp-real\\.elf" "$POLYEXEC" \
  "process mode must allow the AArch64 setjmp fixture to use real ld.so"
assert_contains "aarch64-process-vdso-time-real\\.elf" "$POLYEXEC" \
  "process mode must allow the AArch64 vDSO time fixture to use real ld.so"
assert_contains "riscv-process-exception-real\\.elf" "$POLYEXEC" \
  "process mode must allow the RISC-V C++ exception fixture to use real ld.so"
assert_contains "riscv-process-setjmp-real\\.elf" "$POLYEXEC" \
  "process mode must allow the RISC-V setjmp fixture to use real ld.so"
assert_contains "POLYEXEC_MONITOR_UID" "$POLYEXEC" \
  "userspace monitor must expose a UID diagnostic for non-root proof runs"
assert_contains "SYS_epoll_pwait" "$POLYEXEC" \
  "userspace monitor syscall proxy must support epoll waits for daemon-style I/O"
assert_contains "SYS_rt_sigpending" "$POLYEXEC" \
  "userspace monitor syscall proxy must expose pending signal masks for foreign signal edge tests"
assert_contains "AT_SYSINFO_EHDR" "$POLYEXEC" \
  "process mode must publish a foreign vDSO base through auxv"
assert_contains "map_process_aarch64_vdso" "$POLYEXEC" \
  "process mode must map the custom AArch64 vDSO blob"
assert_contains "POLYEXEC_VDSO_MAP: arch=aarch64" "$POLYEXEC" \
  "process mode must emit a diagnostic when mapping the AArch64 vDSO"
assert_contains "SYS_socket" "$POLYEXEC" \
  "userspace monitor syscall proxy must support sockets for daemon-style I/O"
assert_contains "SYS_getdents64" "$POLYEXEC" \
  "userspace monitor syscall proxy must support directory enumeration for unmodified python3 imports"
assert_contains "memcpy\\(\\(void \\*\\) \\(uintptr_t\\) arg1, entries" "$POLYEXEC" \
  "userspace monitor getdents64 proxy must copy native dirent bytes back to the foreign buffer"
assert_contains "POLY_AARCH64_O_DIRECTORY" "$POLYEXEC" \
  "userspace monitor openat proxy must know AArch64 directory-open flag layout"
assert_contains "poly_translate_open_flags" "$POLYEXEC" \
  "userspace monitor openat proxy must translate foreign open flags before host syscalls"
assert_contains "--threads" "$POLYEXEC" \
  "userspace monitor must expose an in-process pthread stress mode"
assert_contains "pthread_create" "$POLYEXEC" \
  "userspace monitor thread stress must spawn pthreads inside one address space"
assert_not_contains "duplicate auto-spill buffer|spill_buffer" "$POLYEXEC" \
  "userspace monitor thread stress must not depend on retired auto-spill buffers"
assert_contains "compiler_args\\+=\\(-pthread\\)" "$BOOT_SCRIPT" \
  "boot image must link polyexec with pthread support"
assert_contains "RUN_POLY_EXEC_FOCUSED" "$BOOT_SCRIPT" \
  "boot image must expose a focused polyexec proof mode for thread and interpreter stress cases"
assert_contains "POLY_EXEC_FOCUSED_OK" "$BOOT_SCRIPT" \
  "boot validation must gate focused polyexec proof completion"
assert_contains "poly_entry" "$POLYEXEC_THREAD_PREEMPT_STRESS_SRC" \
  "thread preemption stress fixture must expose a returning Poly entry point"
assert_contains "ret" "$POLYEXEC_THREAD_PREEMPT_STRESS_SRC" \
  "thread preemption stress fixture must return through the monitor instead of exiting"
assert_contains "aarch64-thread-preempt-stress-real\\.so#poly_entry=42" "$BOOT_SCRIPT" \
  "boot image must run the AArch64 pthread stress fixture"
assert_contains "riscv-thread-preempt-stress-real\\.so#poly_entry=42" "$BOOT_SCRIPT" \
  "boot image must run the RISC-V pthread stress fixture"
assert_contains "POLYEXEC_THREADS_OK: threads=4 path=/usr/lib/polyapps/aarch64-thread-preempt-stress-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 pthread stress result"
assert_contains "POLYEXEC_THREADS_OK: threads=4 path=/usr/lib/polyapps/riscv-thread-preempt-stress-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V pthread stress result"
assert_contains "--atomic-threads" "$POLYEXEC" \
  "userspace monitor must expose an SMP shared-counter pthread stress mode"
assert_contains "SYS_sched_setaffinity" "$POLYEXEC" \
  "SMP stress mode must churn host thread affinity to force cross-core migration"
assert_contains "POLYEXEC_ATOMIC_THREADS_OK" "$POLYEXEC" \
  "SMP stress mode must report shared-counter atomic success"
assert_contains "POLYEXEC_AFFINITY_CHURN_OK" "$POLYEXEC" \
  "SMP stress mode must report affinity pin/unpin churn"
assert_contains "RUN_POLY_SMP_STRESS" "$BOOT_SCRIPT" \
  "boot image must expose an SMP polyexec stress proof mode"
assert_contains "BOCHS_CPU_COUNT" "$BOOT_SCRIPT" \
  "boot image must make Bochs CPU count configurable"
assert_contains "kernel_smp_flags=\"noapic nolapic acpi=off\"" "$BOOT_SCRIPT" \
  "non-SMP boot mode must keep the legacy APIC-disabling flags"
assert_contains 'kernel_smp_flags="lapic possible_cpus=\$BOCHS_CPU_COUNT nr_cpus=\$BOCHS_CPU_COUNT"' "$BOOT_SCRIPT" \
  "SMP boot mode must enable APIC startup and advertise all requested CPUs"
assert_contains 'cpu: count=\$BOCHS_CPU_COUNT, ips=\$BOCHS_IPS, poly_enabled=\$POLY_ENABLED' "$BOOT_SCRIPT" \
  "bochsrc generation must request SMP CPUs while preserving Poly enablement"
assert_contains "grep -c '\\^processor\\[\\[:space:\\]\\]\\*:' /proc/cpuinfo" "$BOOT_SCRIPT" \
  "boot guest must derive the Linux-visible CPU count from procfs"
assert_contains 'POLY_SMP_NPROC: count=\\\$cpu_count' "$BOOT_SCRIPT" \
  "boot guest must report the Linux-visible CPU count"
assert_contains "aarch64-smp-atomic-real\\.so#poly_entry=42" "$BOOT_SCRIPT" \
  "boot image must run the AArch64 SMP atomic fixture"
assert_contains "riscv-smp-atomic-real\\.so#poly_entry=42" "$BOOT_SCRIPT" \
  "boot image must run the RISC-V SMP atomic fixture"
assert_contains 'POLYEXEC_ATOMIC_THREADS_OK: threads=\$POLY_SMP_THREADS iterations=\$POLY_SMP_ATOMIC_ITERATIONS counter=\$expected_counter path=/usr/lib/polyapps/aarch64-smp-atomic-real' "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 SMP atomic result"
assert_contains 'POLYEXEC_ATOMIC_THREADS_OK: threads=\$POLY_SMP_THREADS iterations=\$POLY_SMP_ATOMIC_ITERATIONS counter=\$expected_counter path=/usr/lib/polyapps/riscv-smp-atomic-real' "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V SMP atomic result"
assert_contains 'POLYEXEC_AFFINITY_CHURN_OK: cpus=\$BOCHS_CPU_COUNT migrations=\[1-9\]\[0-9\]\* threads=\$POLY_SMP_THREADS path=/usr/lib/polyapps/aarch64-smp-atomic-real' "$BOOT_SCRIPT" \
  "boot validation must gate AArch64 cross-core affinity churn"
assert_contains 'POLYEXEC_AFFINITY_CHURN_OK: cpus=\$BOCHS_CPU_COUNT migrations=\[1-9\]\[0-9\]\* threads=\$POLY_SMP_THREADS path=/usr/lib/polyapps/riscv-smp-atomic-real' "$BOOT_SCRIPT" \
  "boot validation must gate RISC-V cross-core affinity churn"
assert_not_contains "POLY_ALPINE_POLYEXEC_AUTO_SPILL|POLYEXEC_AUTO_SPILL_STATUS" "$BOOT_SCRIPT" \
  "boot validation must not expose the retired auto-spill runtime path"
assert_contains "ldxr" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "AArch64 SMP fixture must use native load-linked"
assert_contains "stxr" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "AArch64 SMP fixture must use native store-conditional"
assert_contains "lr\\.d" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "RISC-V SMP fixture must use native load-reserved"
assert_contains "sc\\.d" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "RISC-V SMP fixture must use native store-conditional"
assert_contains "dmb ish" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "AArch64 SMP fixture must execute a memory barrier"
assert_contains "fence rw,rw" "$POLYEXEC_SMP_ATOMIC_SRC" \
  "RISC-V SMP fixture must execute a memory fence"
assert_contains "RUN_POLY_FPU_TORTURE" "$BOOT_SCRIPT" \
  "boot image must expose a dedicated FPU torture proof mode"
assert_contains "aarch64-fpu-torture-real\\.so#poly_entry=6147" "$BOOT_SCRIPT" \
  "boot image must run the AArch64 FPU torture fixture"
assert_contains "riscv-fpu-torture-real\\.so#poly_entry=6147" "$BOOT_SCRIPT" \
  "boot image must run the RISC-V FPU torture fixture"
assert_contains "POLY_FPU_TORTURE_OK" "$BOOT_SCRIPT" \
  "boot validation must gate FPU torture completion"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=6147 path=/usr/lib/polyapps/aarch64-fpu-torture-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 FPU torture result"
assert_contains "POLYEXEC_RESULT: arch=riscv value=6147 path=/usr/lib/polyapps/riscv-fpu-torture-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V FPU torture result"
assert_contains "bx_poly_riscv_canonicalize_fp64_nan" "$BOCHS_CPU" \
  "RISC-V FPU decoder must canonicalize NaN results for guest semantics"
assert_contains "msr fpcr" "$POLYEXEC_FPU_TORTURE_SRC" \
  "AArch64 FPU torture fixture must write FPCR rounding mode"
assert_contains "mrs %x0, fpsr" "$POLYEXEC_FPU_TORTURE_SRC" \
  "AArch64 FPU torture fixture must read FPSR status flags"
assert_contains "csrw fcsr" "$POLYEXEC_FPU_TORTURE_SRC" \
  "RISC-V FPU torture fixture must write fcsr/frm state"
assert_contains "csrr %0, fflags" "$POLYEXEC_FPU_TORTURE_SRC" \
  "RISC-V FPU torture fixture must read fflags status"
assert_contains "qnan_payload" "$POLYEXEC_FPU_TORTURE_SRC" \
  "FPU torture fixture must check quiet-NaN payload propagation"
assert_contains "snan_payload" "$POLYEXEC_FPU_TORTURE_SRC" \
  "FPU torture fixture must check signaling-NaN invalid exceptions"
assert_contains "is_subnormal64" "$POLYEXEC_FPU_TORTURE_SRC" \
  "FPU torture fixture must check subnormal/underflow results"
assert_contains "fpsr_ixc" "$POLYEXEC_FPU_TORTURE_SRC" \
  "AArch64 FPU torture fixture must check inexact FPSR status"
assert_contains "fflag_nx" "$POLYEXEC_FPU_TORTURE_SRC" \
  "RISC-V FPU torture fixture must check inexact fflags status"
assert_contains "0x3ff0000000000001" "$POLYEXEC_FPU_TORTURE_SRC" \
  "FPU torture fixture must check rounding-mode-sensitive results"
assert_contains "RUN_POLY_JIT_SELFTEST" "$BOOT_SCRIPT" \
  "boot image must expose a dedicated JIT/self-modifying-code proof mode"
assert_contains "aarch64-jit-selfmod-real\\.elf=42" "$BOOT_SCRIPT" \
  "boot image must run the AArch64 JIT self-modifying-code fixture"
assert_contains "POLY_JIT_SELFTEST_OK" "$BOOT_SCRIPT" \
  "boot validation must gate JIT self-test completion"
assert_contains "POLY_JIT_SELF_MOD_OK: arch=aarch64 first=13 second=29 wx=1 icache=1" "$BOOT_SCRIPT" \
  "boot validation must require the AArch64 JIT self-modifying-code success marker"
assert_contains "dc cvau" "$POLYEXEC_JIT_SELFMOD_SRC" \
  "AArch64 JIT fixture must clean data cache before executing generated code"
assert_contains "ic ivau" "$POLYEXEC_JIT_SELFMOD_SRC" \
  "AArch64 JIT fixture must invalidate instruction cache for generated code"
assert_contains "protect_page\\(page, PROT_READ \\| PROT_EXEC\\)" "$POLYEXEC_JIT_SELFMOD_SRC" \
  "AArch64 JIT fixture must flip generated code to executable without write permission"
assert_contains "aarch64-process-dynamic-libc-real\\.elf" "$BOOT_SCRIPT" \
  "boot image must build and run an AArch64 dynamically linked process fixture"
assert_contains "riscv-process-dynamic-libc-real\\.elf" "$BOOT_SCRIPT" \
  "boot image must build and run a RISC-V dynamically linked process fixture"
assert_contains "aarch64-real-ls\\.elf" "$BOOT_SCRIPT" \
  "boot image must copy and run an unmodified AArch64 dynamically linked real-world binary"
assert_contains "/bin/ls" "$BOOT_SCRIPT" \
  "unmodified dynamic binary proof must use the host AArch64 coreutils ls image"
assert_contains "/usr/bin/python3\\.12" "$BOOT_SCRIPT" \
  "boot image must copy an unmodified AArch64 python3 binary for daemon-style I/O proof"
assert_contains "POLY_RISCV_PYTHON_PACKAGES" "$BOOT_SCRIPT" \
  "boot image must define the riscv64 Python 3.12 package closure"
assert_contains "stage_riscv_python_runtime" "$BOOT_SCRIPT" \
  "boot image must stage a riscv64 Python 3.12 runtime"
assert_contains "riscv-real-python3\\.elf" "$BOOT_SCRIPT" \
  "boot image must copy an unmodified RISC-V python3 binary for daemon-style I/O proof"
assert_contains "riscv-python-root" "$BOOT_SCRIPT" \
  "focused RISC-V Python process run must use an architecture-private Python home"
assert_contains 'download \$POLY_RISCV_PYTHON_PACKAGES' "$BOOT_SCRIPT" \
  "RISC-V Python staging must fetch the exact configured riscv64 package closure without host installation"
assert_contains "polyexec_python_epoll_server\\.py" "$BOOT_SCRIPT" \
  "boot image must include the Python epoll web-server smoke script"
assert_contains "PYTHONHOME=/usr" "$BOOT_SCRIPT" \
  "focused Python process run must set a deterministic Python home inside the initramfs"
assert_contains "-S /usr/lib/polyapps/polyexec_python_epoll_server\\.py" "$BOOT_SCRIPT" \
  "focused Python process run must skip site imports to keep the initramfs payload minimal"
assert_contains "aarch64-process-exception-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the AArch64 C++ exception fixture"
assert_contains "aarch64-process-setjmp-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the AArch64 setjmp/longjmp fixture"
assert_contains "aarch64-process-signal-mask-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the AArch64 signal mask edge fixture"
assert_contains "aarch64-process-signal-handler-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the AArch64 signal handler/rt_sigreturn fixture"
assert_contains "POLYEXEC_PROTECT_RUNTIME_SIGNALS=1 /usr/bin/polyexec --process" "$BOOT_SCRIPT" \
  "signal handler fixture must opt into protected runtime signal delivery"
assert_contains "case 190: \\*x86_number = SYS_semget" "$POLYEXEC" \
  "generic Linux syscall 190 must translate to semget for PostgreSQL-style SysV IPC"
assert_contains "case 192: \\*x86_number = SYS_semtimedop" "$POLYEXEC" \
  "generic Linux syscall 192 must translate to semtimedop for semaphore waits"
assert_contains "case 194: \\*x86_number = SYS_shmget" "$POLYEXEC" \
  "generic Linux syscall 194 must translate to shmget for shared-memory workloads"
assert_contains "case 196: \\*x86_number = SYS_shmat" "$POLYEXEC" \
  "generic Linux syscall 196 must translate to shmat for shared-memory attaches"
assert_contains "#define POLY_AARCH64_O_DIRECTORY 040000ULL" "$POLYEXEC" \
  "AArch64 O_DIRECTORY must translate directory opens correctly"
assert_contains "#define POLY_AARCH64_O_NOFOLLOW 0100000ULL" "$POLYEXEC" \
  "AArch64 O_NOFOLLOW must not be confused with O_LARGEFILE"
assert_contains "#define POLY_AARCH64_O_DIRECT 0200000ULL" "$POLYEXEC" \
  "AArch64 O_DIRECT must not be confused with O_DIRECTORY"
assert_contains "#define POLY_AARCH64_O_LARGEFILE 0400000ULL" "$POLYEXEC" \
  "AArch64 O_LARGEFILE must be stripped before host openat"
assert_contains "POLY_SYSV_IPC_OK: shm=1 sem=1 fcntl=1" "$POLYEXEC_PROCESS_SYSCALL_SRC" \
  "process syscall fixture must prove SysV shm/semaphore and fcntl coverage"
assert_contains "POLY_SYSV_IPC_OK: shm=1 sem=1 fcntl=1" "$BOOT_SCRIPT" \
  "boot validation must gate the SysV IPC process syscall marker"
assert_contains "aarch64-process-vdso-time-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the AArch64 vDSO time fixture"
assert_contains "aarch64-polyexec-vdso\\.so" "$BOOT_SCRIPT" \
  "focused boot must build and stage the custom AArch64 vDSO"
assert_contains "polyexec_aarch64_vdso\\.ld" "$BOOT_SCRIPT" \
  "focused boot must link the AArch64 vDSO with the fixed single-load layout"
assert_contains "POLYEXEC_VDSO_MAP: arch=aarch64" "$BOOT_SCRIPT" \
  "focused boot validation must gate the AArch64 vDSO auxv mapping"
assert_contains "riscv-process-signal-mask-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the RISC-V signal mask edge fixture"
assert_contains "riscv-process-signal-handler-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the RISC-V signal handler/rt_sigreturn fixture"
assert_contains "riscv-process-exception-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the RISC-V C++ exception fixture"
assert_contains "riscv-process-setjmp-real\\.elf=42" "$BOOT_SCRIPT" \
  "focused boot must run the RISC-V setjmp/longjmp fixture"
assert_contains "processdeps/riscv64/libm\\.so\\.6" "$BOOT_SCRIPT" \
  "focused boot must stage RISC-V libm for the libstdc++ unwinder dependency chain"
assert_contains "lib/riscv64-linux-gnu/libm\\.so\\.6" "$BOOT_SCRIPT" \
  "focused boot must expose RISC-V libm on the foreign loader default library path"
assert_contains "polyexec-nonroot" "$BOOT_SCRIPT" \
  "focused boot must run the monitor through the non-root privilege-drop wrapper"
assert_contains "POLYEXEC_PROCESS_EXIT" "$POLYEXEC" \
  "userspace monitor must report real dynamic process exit codes"
assert_contains "processdeps/aarch64/libc\\.so\\.6" "$BOOT_SCRIPT" \
  "boot image must include AArch64 dynamic libc for process-mode DT_NEEDED loading"
assert_contains "processdeps/riscv64/libc\\.so\\.6" "$BOOT_SCRIPT" \
  "boot image must include RISC-V dynamic libc for process-mode DT_NEEDED loading"
assert_contains "POLYEXEC_INTERP_LOAD: arch=aarch64.*aarch64-process-dynamic-libc-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 real ld.so handoff"
assert_contains "POLYEXEC_INTERP_LOAD: arch=riscv.*riscv-process-dynamic-libc-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V real ld.so handoff"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-dynamic-libc-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 dynamic libc process result"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=0 process=1 path=/usr/lib/polyapps/aarch64-real-ls" "$BOOT_SCRIPT" \
  "boot validation must gate the unmodified AArch64 ls dynamic process result"
assert_contains "POLYEXEC_INTERP_LOAD: arch=aarch64.*aarch64-real-python3" "$BOOT_SCRIPT" \
  "boot validation must gate the unmodified AArch64 python3 real ld.so handoff"
assert_contains "POLY_PYTHON_EPOLL_OK: selector=EpollSelector" "$BOOT_SCRIPT" \
  "boot validation must gate the Python epoll web-server success marker"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-real-python3" "$BOOT_SCRIPT" \
  "boot validation must gate the unmodified AArch64 python3 process result"
assert_contains "POLYEXEC_INTERP_LOAD: arch=riscv.*riscv-real-python3" "$BOOT_SCRIPT" \
  "boot validation must gate the unmodified RISC-V python3 real ld.so handoff"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-real-python3" "$BOOT_SCRIPT" \
  "boot validation must gate the unmodified RISC-V python3 process result"
assert_contains "grep -c \"POLY_PYTHON_EPOLL_OK: selector=EpollSelector\"" "$BOOT_SCRIPT" \
  "boot validation must require epoll success markers from both AArch64 and RISC-V Python runs"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-exception-real" "$BOOT_SCRIPT" \
  "boot validation must gate the C++ exception process result"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-setjmp-real" "$BOOT_SCRIPT" \
  "boot validation must gate the setjmp/longjmp process result"
assert_contains "POLY_SIGNAL_MASK_EDGE_OK iterations=8" "$BOOT_SCRIPT" \
  "boot validation must gate the signal mask queue/drain success marker"
assert_contains "POLY_SIGNAL_HANDLER_OK signum=10 count=1 x19=restored" "$BOOT_SCRIPT" \
  "boot validation must gate the signal handler/rt_sigreturn success marker"
assert_contains "POLY_SIGNAL_HANDLER_MASK_OK blocked=1 delivered_after_unblock=1" "$BOOT_SCRIPT" \
  "boot validation must gate masked virtual signal delivery"
assert_contains 'grep -c "POLY_SIGNAL_HANDLER_OK signum=10 count=1 x19=restored"' "$BOOT_SCRIPT" \
  "boot validation must require signal handler success markers from both AArch64 and RISC-V"
assert_contains 'grep -c "POLY_SIGNAL_HANDLER_MASK_OK blocked=1 delivered_after_unblock=1"' "$BOOT_SCRIPT" \
  "boot validation must require masked handler delivery markers from both AArch64 and RISC-V"
assert_contains "poly_rt_sigreturn_restorer" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "signal handler fixture must provide a guest rt_sigreturn restorer"
assert_contains "POLY_SYS_RT_SIGPROCMASK" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "signal handler fixture must verify guest signal masks"
assert_contains "POLY_SYS_RT_SIGACTION" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "signal handler fixture must install the handler through rt_sigaction"
assert_contains "POLY_SYS_KILL" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "signal handler fixture must raise a protected runtime signal"
assert_contains "#elif defined\\(__riscv\\)" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "signal handler fixture must compile for RISC-V as well as AArch64"
assert_contains "li a7, 139" "$POLYEXEC_PROCESS_SIGNAL_HANDLER_SRC" \
  "RISC-V signal handler fixture must invoke rt_sigreturn through ecall 139"
assert_contains "POLY_VDSO_TIME_OK iterations=64" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 vDSO time fixture marker"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-vdso-time-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 vDSO time fixture result"
assert_contains "POLYEXEC_EVENTS: count=\\(\\[1-9\\]\\|\\[1-4\\]\\[0-9\\]\\).*aarch64-process-vdso-time-real" "$BOOT_SCRIPT" \
  "boot validation must prove the AArch64 vDSO time loop avoids one event per call"
assert_contains "POLYEXEC_RESULT: arch=aarch64 value=42 process=1 path=/usr/lib/polyapps/aarch64-process-signal-mask-real" "$BOOT_SCRIPT" \
  "boot validation must gate the AArch64 signal mask edge result"
assert_contains "POLY_NONROOT_EXEC: uid=65534 euid=65534 gid=65534 egid=65534 command=/usr/bin/polyexec" "$BOOT_SCRIPT" \
  "boot validation must prove polyexec was launched after dropping root privileges"
assert_contains "POLYEXEC_MONITOR_UID: uid=65534 euid=65534 gid=65534 egid=65534" "$BOOT_SCRIPT" \
  "boot validation must prove the monitor itself observed a non-root uid"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-dynamic-libc-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V dynamic libc process result"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-exception-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V C++ exception process result"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-setjmp-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V setjmp/longjmp process result"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-signal-mask-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V signal mask edge result"
assert_contains "POLYEXEC_RESULT: arch=riscv value=42 process=1 path=/usr/lib/polyapps/riscv-process-signal-handler-real" "$BOOT_SCRIPT" \
  "boot validation must gate the RISC-V signal handler/rt_sigreturn result"
assert_contains "throw std::runtime_error" "$POLYEXEC_PROCESS_EXCEPTION_SRC" \
  "C++ exception fixture must throw through the foreign ABI unwinder"
assert_contains "catch \\(const std::runtime_error" "$POLYEXEC_PROCESS_EXCEPTION_SRC" \
  "C++ exception fixture must catch through the foreign ABI unwinder"
assert_contains "setjmp" "$POLYEXEC_PROCESS_SETJMP_SRC" \
  "setjmp fixture must save foreign ABI stack/register state"
assert_contains "longjmp" "$POLYEXEC_PROCESS_SETJMP_SRC" \
  "setjmp fixture must restore foreign ABI stack/register state"
assert_contains "POLY_SYS_RT_SIGPROCMASK" "$POLYEXEC_PROCESS_SIGNAL_MASK_SRC" \
  "signal mask edge fixture must use the guest rt_sigprocmask syscall"
assert_contains "POLY_SYS_RT_SIGPENDING" "$POLYEXEC_PROCESS_SIGNAL_MASK_SRC" \
  "signal mask edge fixture must assert pending signal queue state"
assert_contains "POLY_SYS_SIGNALFD4" "$POLYEXEC_PROCESS_SIGNAL_MASK_SRC" \
  "signal mask edge fixture must drain queued blocked signals explicitly"
assert_contains "for \\(int iteration = 0; iteration < 8; iteration\\+\\+\\)" "$POLYEXEC_PROCESS_SIGNAL_MASK_SRC" \
  "signal mask edge fixture must heavily mask and unmask rather than doing a one-shot smoke"
assert_contains "cntvct_el0" "$POLYEXEC_AARCH64_VDSO_SRC" \
  "AArch64 vDSO must read the user-mode virtual counter"
assert_contains "cntfrq_el0" "$POLYEXEC_AARCH64_VDSO_SRC" \
  "AArch64 vDSO must read the user-mode counter frequency"
assert_contains "__kernel_clock_gettime" "$POLYEXEC_AARCH64_VDSO_SRC" \
  "AArch64 vDSO must export the clock_gettime entry point"
assert_contains "__clock_gettime" "$POLYEXEC_AARCH64_VDSO_SRC" \
  "AArch64 vDSO must export the glibc AArch64 clock_gettime alias"
assert_contains "__kernel_gettimeofday" "$POLYEXEC_AARCH64_VDSO_SRC" \
  "AArch64 vDSO must export the gettimeofday entry point"
assert_contains "LINUX_2\\.6\\.39" "$POLYEXEC_AARCH64_VDSO_MAP" \
  "AArch64 vDSO symbols must use the Linux AArch64 vDSO version namespace"
assert_contains "PT_LOAD FILEHDR PHDRS" "$POLYEXEC_AARCH64_VDSO_LD" \
  "AArch64 vDSO linker script must keep ELF headers inside the first load segment"
assert_contains "PT_DYNAMIC FLAGS\\(4\\)" "$POLYEXEC_AARCH64_VDSO_LD" \
  "AArch64 vDSO linker script must expose a readonly PT_DYNAMIC segment"
assert_contains "clock_gettime\\(CLOCK_MONOTONIC" "$POLYEXEC_PROCESS_VDSO_TIME_SRC" \
  "AArch64 vDSO time fixture must exercise monotonic clock_gettime"
assert_contains "iteration < 64" "$POLYEXEC_PROCESS_VDSO_TIME_SRC" \
  "AArch64 vDSO time fixture must make enough calls to catch syscall-trap fallback"
assert_contains "selectors\\.DefaultSelector" "$POLYEXEC_PYTHON_EPOLL_SERVER_SRC" \
  "Python daemon fixture must use the default selector abstraction"
assert_contains "EpollSelector" "$POLYEXEC_PYTHON_EPOLL_SERVER_SRC" \
  "Python daemon fixture must require Linux epoll rather than a weaker selector"
assert_contains "listen\\(1\\)" "$POLYEXEC_PYTHON_EPOLL_SERVER_SRC" \
  "Python daemon fixture must create a listening socket"
assert_contains "setuid\\(65534\\)" "$POLYEXEC_NONROOT_RUNNER_SRC" \
  "non-root wrapper must drop the monitor to uid 65534 before exec"
assert_contains "execvp\\(argv\\[1\\]" "$POLYEXEC_NONROOT_RUNNER_SRC" \
  "non-root wrapper must exec the monitor after dropping privileges"
assert_contains "fmov d31" "$POLYEXEC_PREEMPT_STRESS_SRC" \
  "preemption stress fixture must keep the full AArch64 FP bank live"
assert_contains "\\.irp r,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31" "$POLYEXEC_PREEMPT_STRESS_SRC" \
  "preemption stress fixture must keep the full RISC-V FP bank live"
assert_contains "fmv\\.d\\.x f\\\\\\\\r" "$POLYEXEC_PREEMPT_STRESS_SRC" \
  "preemption stress fixture must write RISC-V FP registers"
assert_contains "POLYEXEC_PREEMPT_STRESS_REAL_SRC" "$BOOT_SCRIPT" \
  "boot image must build the preemption stress fixture"
assert_not_contains "RUN_POLY_PREEMPT_STRESS|POLY_PREEMPT_STRESS_OK" "$BOOT_SCRIPT" \
  "boot script must not retain the retired auto-spill preemption stress section"
assert_contains "CPL[[:space:]]*!=[[:space:]]*3" "$RESTORE_FUNC" \
  "raw interrupt restore must only run on return to userspace"
assert_contains "bx_poly_interrupted_raw_valid" "$RESTORE_FUNC" \
  "raw interrupt restore must require an armed interrupted foreign state"
assert_contains "bx_poly_is_raw_mode\\(bx_poly_interrupted_raw_mode\\)" "$RESTORE_FUNC" \
  "raw interrupt restore must require a recorded raw foreign mode"
assert_contains "bx_poly_interrupted_raw_rip[[:space:]]*!=[[:space:]]*RIP" "$RESTORE_FUNC" \
  "raw interrupt restore must only resume when IRET/SYSRET reaches the recorded RIP"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*bx_poly_interrupted_raw_mode" "$RESTORE_FUNC" \
  "raw interrupt restore must switch back to the recorded foreign frontend"
assert_contains "bx_poly_interrupted_raw_valid[[:space:]]*=[[:space:]]*false" "$RESTORE_FUNC" \
  "raw interrupt restore must consume the interrupted foreign state"
assert_contains "bx_poly_commit_reg_state" "$RESTORE_FUNC" \
  "raw interrupt restore must commit the keyed synthetic bank"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$RESTORE_FUNC" \
  "raw interrupt restore must split the current x86 trace before raw fetch resumes"
assert_contains "poly_interrupt_enter\\(vector, type, error_code\\)" "$BOCHS_EXCEPTION" \
  "x86 interrupt delivery must invoke raw foreign interrupt capture"
assert_contains "poly_iret_return_to_user\\(\\)" "$IRET64_FUNC" \
  "IRET64 return must invoke raw foreign frontend restore"
assert_contains "poly_sysret_return_to_user\\(\\)" "$SYSRET_FUNC" \
  "SYSRET return must invoke raw foreign frontend restore"
assert_contains "poly_sysexit_return_to_user\\(\\)" "$SYSEXIT_FUNC" \
  "SYSEXIT return must invoke raw foreign frontend restore"

CROSS_A64_FUNC="$TMP_DIR/execute_poly_raw_aarch64.cc"
CROSS_RV_FUNC="$TMP_DIR/execute_poly_raw_riscv.cc"
CROSS_ENTER_FUNC="$TMP_DIR/enter_poly_cross_call.cc"
CROSS_RETURN_FUNC="$TMP_DIR/return_poly_cross_call.cc"
extract_function "execute_poly_raw_aarch64" "$CROSS_A64_FUNC"
extract_function "execute_poly_raw_riscv" "$CROSS_RV_FUNC"
extract_function "enter_poly_cross_call" "$CROSS_ENTER_FUNC"
extract_function "return_poly_cross_call" "$CROSS_RETURN_FUNC"
assert_contains "BX_POLY_AARCH64_CTRL_SWITCH_MODE" "$CROSS_A64_FUNC" \
  "AArch64 raw decoder must recognize generic frontend switch"
assert_contains "BX_POLY_AARCH64_CTRL_CALL_MODE" "$CROSS_A64_FUNC" \
  "AArch64 raw decoder must recognize generic frontend call"
assert_contains "bx_poly_frontend_id_to_mode" "$CROSS_A64_FUNC" \
  "AArch64 generic transitions must resolve frontend IDs without routing through x86"
assert_contains "read_poly_aarch64_reg\\(16" "$CROSS_A64_FUNC" \
  "AArch64 generic transition must take the target from x16"
assert_contains "read_poly_aarch64_reg\\(17" "$CROSS_A64_FUNC" \
  "AArch64 generic transition must take the frontend ID from x17"
assert_contains "read_poly_aarch64_reg\\(18" "$CROSS_A64_FUNC" \
  "AArch64 generic call must take the return PC from x18"
assert_contains "BX_POLY_MODE_RAW_AARCH64" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must record AArch64 as the caller mode"
assert_contains "enter_poly_cross_call\\(BX_POLY_MODE_RAW_AARCH64,[[:space:]]*target_mode" "$CROSS_A64_FUNC" \
  "AArch64 cross-call gate must use the resolved callee frontend mode"
assert_contains "BX_POLY_RISCV_CTRL_SWITCH_MODE" "$CROSS_RV_FUNC" \
  "RISC-V raw decoder must recognize generic frontend switch"
assert_contains "BX_POLY_RISCV_CTRL_CALL_MODE" "$CROSS_RV_FUNC" \
  "RISC-V raw decoder must recognize generic frontend call"
assert_contains "bx_poly_frontend_id_to_mode" "$CROSS_RV_FUNC" \
  "RISC-V generic transitions must resolve frontend IDs without routing through x86"
assert_contains "read_poly_riscv_reg\\(5" "$CROSS_RV_FUNC" \
  "RISC-V generic transition must take the target from x5/t0"
assert_contains "read_poly_riscv_reg\\(6" "$CROSS_RV_FUNC" \
  "RISC-V generic transition must take the frontend ID from x6/t1"
assert_contains "read_poly_riscv_reg\\(7" "$CROSS_RV_FUNC" \
  "RISC-V generic call must take the return PC from x7/t2"
assert_contains "BX_POLY_MODE_RAW_RISCV" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must record RISC-V as the caller mode"
assert_contains "enter_poly_cross_call\\(BX_POLY_MODE_RAW_RISCV,[[:space:]]*target_mode" "$CROSS_RV_FUNC" \
  "RISC-V cross-call gate must use the resolved callee frontend mode"
assert_contains "write_poly_aarch64_reg\\(30,[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE\\)" "$CROSS_ENTER_FUNC" \
  "cross-call entry must use the native AArch64 link register return cookie"
assert_contains "write_poly_riscv_reg\\(1,[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE\\)" "$CROSS_ENTER_FUNC" \
  "cross-call entry must use the native RISC-V ra return cookie"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*callee_mode" "$CROSS_ENTER_FUNC" \
  "cross-call entry must switch directly to the callee frontend"
assert_contains "RIP[[:space:]]*=[[:space:]]*target_rip" "$CROSS_ENTER_FUNC" \
  "cross-call entry must branch directly to the foreign target"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$CROSS_ENTER_FUNC" \
  "cross-call entry must split the current trace before the callee frontend runs"
assert_not_contains "BX_POLY_MODE_X86|return_poly_abi_call|deliver_poly_architectural_trap|handle_poly_foreign_syscall" \
  "$CROSS_ENTER_FUNC" \
  "cross-call entry must not route native foreign-to-foreign calls through x86 policy"
assert_contains "target_rip[[:space:]]*!=[[:space:]]*\\(bx_address\\)[[:space:]]*BX_POLY_CROSS_RETURN_COOKIE" "$CROSS_RETURN_FUNC" \
  "cross-call return must require the native return-cookie target"
assert_contains "bx_poly_current_mode[[:space:]]*=[[:space:]]*frame\\.caller_mode" "$CROSS_RETURN_FUNC" \
  "cross-call return must restore the caller frontend directly"
assert_contains "RIP[[:space:]]*=[[:space:]]*frame\\.return_rip" "$CROSS_RETURN_FUNC" \
  "cross-call return must resume the caller's native return PC"
assert_contains "BX_ASYNC_EVENT_STOP_TRACE" "$CROSS_RETURN_FUNC" \
  "cross-call return must split the trace before caller frontend resumes"
assert_not_contains "BX_POLY_MODE_X86|return_poly_abi_call|deliver_poly_architectural_trap|handle_poly_foreign_syscall" \
  "$CROSS_RETURN_FUNC" \
  "cross-call return must not route native foreign-to-foreign returns through x86 policy"
assert_contains "0xd5032f1f" "$POLYPROBE" \
  "polyprobe must exercise AArch64 generic frontend switch"
assert_contains "0x1000700b" "$POLYPROBE" \
  "polyprobe must exercise RISC-V generic frontend switch"
assert_contains "0xd5032f3f" "$POLYPROBE" \
  "polyprobe must exercise AArch64 generic frontend call"
assert_contains "0x1200700b" "$POLYPROBE" \
  "polyprobe must exercise RISC-V generic frontend call"
assert_contains "aarch64-to-riscv" "$POLYBENCH" \
  "polybench must cover AArch64-to-RISC-V mixed execution"
assert_contains "riscv-to-aarch64" "$POLYBENCH" \
  "polybench must cover RISC-V-to-AArch64 mixed execution"
assert_contains "POLYBENCH_CROSS_CALL_RESULT" "$POLYBENCH" \
  "polybench must cover native cross-frontend call/return"
assert_contains "POLYBENCH_TRAP_DENSITY_RESULT" "$POLYBENCH" \
  "polybench must report monitor traps per million foreign instructions"
assert_contains "traps_per_million" "$BOOT_SCRIPT" \
  "boot validation must gate the trap-density performance metric"
assert_contains "POLY_ALPINE_POSTGRES_INITDB_TIMEOUT" "$BOOT_SCRIPT" \
  "PostgreSQL smoke must expose an initdb diagnostic timeout"
assert_contains "POLY_ALPINE_POSTGRES_INITDB_TIMEOUT" "$BOOT_SCRIPT" \
  "PostgreSQL smoke must dump state when initdb stalls"
assert_contains "POLY_ALPINE_POSTGRES_INITDB_WATCHDOG_START" "$BOOT_SCRIPT" \
  "PostgreSQL smoke must report when the initdb watchdog is active"
assert_contains "POLYEXEC_TRACE_POSTGRES_SYSCALLS" "$BOOT_SCRIPT" \
  "PostgreSQL smoke must support targeted postgres syscall tracing"

if grep -R -I -n -E "BXPN_POLY_COMPAT_TRAPS|poly_compat_traps|compat_traps" "$BOCHS_DIR" \
    > "$TMP_DIR/compat-uses"; then
  cat "$TMP_DIR/compat-uses" >&2
  fail "deprecated compat trap knob must not exist in the Bochs prototype"
fi
assert_not_contains "POLY_COMPAT_TRAPS|boot-poly-compat|boot-poly-full-compat" "$ROOT_DIR/Makefile" \
  "root make targets must not expose the removed compat trap knob"
assert_not_contains "POLY_COMPAT_TRAPS|poly_compat_traps" "$ROOT_DIR/scripts/boot.sh" \
  "boot configuration must not emit the removed compat trap knob"
assert_not_contains "poly_cpuid_expected_feature_mask_for_compat" "$ROOT_DIR/tools/include/polycpuid.h" \
  "CPUID checks must not carry compat-trap feature variants"
assert_not_contains "POLY_CPUID_FEATURE_COMPAT_TRAPS" "$ROOT_DIR/tools/include/polycpuid.h" \
  "CPUID ABI must not retain a named compatibility-trap feature bit"
assert_not_contains "libcall_(expected|number_expected|id)" "$POLYAPP" \
  "polyapp manifests must use neutral break-trap keys, not legacy libcall aliases"

echo "poly architecture contract OK"
