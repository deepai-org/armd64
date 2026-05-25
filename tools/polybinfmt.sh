#!/bin/busybox sh
set -eu

path="${1:-}"
if [ -z "$path" ]; then
  echo "POLYBINFMT_FAIL: missing foreign ELF path" >&2
  exit 2
fi

expected=""
case "$path" in
  */aarch64-add.elf) expected=132 ;;
  */aarch64-regadd.elf) expected=123 ;;
  */aarch64-brk.elf) expected=0x4c000105 ;;
  */aarch64-svc.elf) expected=0x53000701 ;;
  */riscv-add.elf) expected=27 ;;
  */riscv-regadd.elf) expected=123 ;;
  */riscv-ebreak.elf) expected=0x4c000205 ;;
  */riscv-ecall.elf) expected=0x53000702 ;;
esac

echo "POLYBINFMT_EXEC: path=$path expected=${expected:-none}"
if [ -n "$expected" ]; then
  exec /usr/bin/polyexec "$path=$expected"
fi
exec /usr/bin/polyexec "$path"
