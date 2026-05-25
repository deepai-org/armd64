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
  */aarch64-openat.elf) expected=3 ;;
  */aarch64-openat-lseek.elf) expected=7 ;;
  */aarch64-openat-read.elf) expected=4 ;;
  */aarch64-openat-read-close.elf) expected=0 ;;
  */aarch64-clock-gettime.elf) expected=0 ;;
  */aarch64-getpgid.elf) expected=4242 ;;
  */aarch64-getsid.elf) expected=4242 ;;
  */aarch64-getrusage.elf) expected=321 ;;
  */aarch64-getcpu.elf) expected=12 ;;
  */aarch64-gettimeofday.elf) expected=246 ;;
  */aarch64-sysinfo.elf) expected=98765 ;;
  */aarch64-mmap.elf) expected=0 ;;
  */aarch64-mmap-store.elf) expected=77 ;;
  */aarch64-getpid.elf) expected=4242 ;;
  */aarch64-getppid.elf) expected=4241 ;;
  */aarch64-getuid.elf) expected=1000 ;;
  */aarch64-geteuid.elf) expected=1000 ;;
  */aarch64-getgid.elf) expected=1000 ;;
  */aarch64-getegid.elf) expected=1000 ;;
  */aarch64-gettid.elf) expected=4243 ;;
  */aarch64-getcwd.elf) expected=6 ;;
  */aarch64-uname.elf) expected=0 ;;
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
  */riscv-openat.elf) expected=3 ;;
  */riscv-openat-lseek.elf) expected=7 ;;
  */riscv-openat-read.elf) expected=4 ;;
  */riscv-openat-read-close.elf) expected=0 ;;
  */riscv-clock-gettime.elf) expected=0 ;;
  */riscv-getpgid.elf) expected=4242 ;;
  */riscv-getsid.elf) expected=4242 ;;
  */riscv-getrusage.elf) expected=321 ;;
  */riscv-getcpu.elf) expected=12 ;;
  */riscv-gettimeofday.elf) expected=246 ;;
  */riscv-sysinfo.elf) expected=98765 ;;
  */riscv-mmap.elf) expected=0 ;;
  */riscv-mmap-store.elf) expected=77 ;;
  */riscv-getpid.elf) expected=4242 ;;
  */riscv-getppid.elf) expected=4241 ;;
  */riscv-getuid.elf) expected=1000 ;;
  */riscv-geteuid.elf) expected=1000 ;;
  */riscv-getgid.elf) expected=1000 ;;
  */riscv-getegid.elf) expected=1000 ;;
  */riscv-gettid.elf) expected=4243 ;;
  */riscv-getcwd.elf) expected=6 ;;
  */riscv-uname.elf) expected=0 ;;
  */riscv-exit.elf) expected=7 ;;
  */riscv-ebreak.elf) expected=0x4c000205 ;;
  */riscv-ecall.elf) expected=0x53000702 ;;
esac

echo "POLYBINFMT_EXEC: path=$path expected=${expected:-none}"
if [ -n "$expected" ]; then
  exec /usr/bin/polyexec "$path=$expected"
fi
exec /usr/bin/polyexec "$path"
