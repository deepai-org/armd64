#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
usage: run_poly_preemption_stress.sh --payload ELF --expected VALUE [--jobs N] [--iterations N] [--polyexec PATH] [--log-dir DIR]

Runs multiple polyexec instances in parallel and verifies that each instance:
  - exits successfully,
  - reports the expected POLYEXEC_RESULT value,
  - reports a nonzero POLYEXEC_AUTO_SPILL_STATUS count.

Run this inside the Linux guest on a Poly emulator build with auto-spill enabled.
USAGE
}

payload=""
expected=""
jobs="${POLY_STRESS_JOBS:-4}"
iterations="${POLY_STRESS_ITER:-1}"
polyexec="${POLYEXEC:-/usr/bin/polyexec}"
log_dir=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --payload)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      payload="$2"
      shift 2
      ;;
    --expected)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      expected="$2"
      shift 2
      ;;
    --jobs)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      jobs="$2"
      shift 2
      ;;
    --iterations)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      iterations="$2"
      shift 2
      ;;
    --polyexec)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      polyexec="$2"
      shift 2
      ;;
    --log-dir)
      [[ $# -ge 2 ]] || { usage; exit 2; }
      log_dir="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [[ -z "$payload" || -z "$expected" ]]; then
  usage
  exit 2
fi
if ! [[ "$jobs" =~ ^[1-9][0-9]*$ && "$iterations" =~ ^[1-9][0-9]*$ ]]; then
  echo "POLY_STRESS_FAIL: --jobs and --iterations must be positive integers" >&2
  exit 2
fi
if [[ ! -x "$polyexec" ]]; then
  echo "POLY_STRESS_FAIL: polyexec is not executable: $polyexec" >&2
  exit 2
fi

cleanup_log_dir=0
if [[ -z "$log_dir" ]]; then
  log_dir="$(mktemp -d)"
  cleanup_log_dir=1
else
  mkdir -p "$log_dir"
fi
trap 'if [[ "$cleanup_log_dir" -eq 1 ]]; then rm -rf "$log_dir"; fi' EXIT

pids=()
for job in $(seq 1 "$jobs"); do
  log="$log_dir/poly-preempt-$job.log"
  (
    for iter in $(seq 1 "$iterations"); do
      POLYEXEC_AUTO_SPILL=1 "$polyexec" "$payload=$expected"
    done
  ) >"$log" 2>&1 &
  pids+=("$!")
done

failed=0
for pid in "${pids[@]}"; do
  if ! wait "$pid"; then
    failed=1
  fi
done

for job in $(seq 1 "$jobs"); do
  log="$log_dir/poly-preempt-$job.log"
  if ! grep -Eq "POLYEXEC_RESULT: .* value=$expected( |$)" "$log"; then
    echo "POLY_STRESS_FAIL: missing expected result in $log" >&2
    failed=1
  fi
  if ! grep -Eq "POLYEXEC_AUTO_SPILL_STATUS: count=[1-9][0-9]* " "$log"; then
    echo "POLY_STRESS_FAIL: missing nonzero auto-spill count in $log" >&2
    failed=1
  fi
done

if [[ "$failed" -ne 0 ]]; then
  echo "POLY_STRESS_FAIL: logs retained in $log_dir" >&2
  trap - EXIT
  exit 1
fi

echo "POLY_STRESS_OK: jobs=$jobs iterations=$iterations payload=$payload expected=$expected"
