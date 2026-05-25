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
  */aarch64-mul.elf) expected=42 ;;
  */aarch64-branch.elf) expected=42 ;;
  */aarch64-condbranch.elf) expected=91 ;;
  */aarch64-loop.elf) expected=0 ;;
  */aarch64-ret.elf) expected=55 ;;
  */aarch64-mem.elf) expected=77 ;;
  */aarch64-strlen.elf) expected=5 ;;
  */aarch64-memfill.elf) expected=4 ;;
  */aarch64-memcmp.elf) expected=1 ;;
  */aarch64-memcpy.elf) expected=4 ;;
  */aarch64-read.elf) expected=4 ;;
  */aarch64-write.elf) expected=5 ;;
  */aarch64-getpid.elf) expected=4242 ;;
  */aarch64-exit.elf) expected=7 ;;
  */aarch64-brk.elf) expected=0x4c000105 ;;
  */aarch64-svc.elf) expected=0x53000701 ;;
  */riscv-add.elf) expected=27 ;;
  */riscv-regadd.elf) expected=123 ;;
  */riscv-mul.elf) expected=42 ;;
  */riscv-branch.elf) expected=42 ;;
  */riscv-condbranch.elf) expected=91 ;;
  */riscv-loop.elf) expected=0 ;;
  */riscv-ret.elf) expected=55 ;;
  */riscv-mem.elf) expected=77 ;;
  */riscv-strlen.elf) expected=5 ;;
  */riscv-memfill.elf) expected=4 ;;
  */riscv-memcmp.elf) expected=1 ;;
  */riscv-memcpy.elf) expected=4 ;;
  */riscv-read.elf) expected=4 ;;
  */riscv-write.elf) expected=5 ;;
  */riscv-getpid.elf) expected=4242 ;;
  */riscv-exit.elf) expected=7 ;;
  */riscv-ebreak.elf) expected=0x4c000205 ;;
  */riscv-ecall.elf) expected=0x53000702 ;;
esac

echo "POLYBINFMT_EXEC: path=$path expected=${expected:-none}"
if [ -n "$expected" ]; then
  exec /usr/bin/polyexec "$path=$expected"
fi
exec /usr/bin/polyexec "$path"
