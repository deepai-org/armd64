.text
.align 4

.global __kernel_clock_gettime
.global __clock_gettime
.type __kernel_clock_gettime, %function
.type __clock_gettime, %function
__clock_gettime:
__kernel_clock_gettime:
  cbz x1, 9f
  cmp w0, #1
  b.eq 1f
  cmp w0, #4
  b.eq 1f
  mov x8, #113
  svc #0
  ret
1:
  mrs x2, cntvct_el0
  mrs x3, cntfrq_el0
  cbz x3, 9f
  udiv x4, x2, x3
  mul x5, x4, x3
  sub x5, x2, x5
  movz x6, #0xca00
  movk x6, #0x3b9a, lsl #16
  mul x5, x5, x6
  udiv x5, x5, x3
  str x4, [x1]
  str x5, [x1, #8]
  mov w0, #0
  ret
9:
  mov w0, #-22
  ret
.size __kernel_clock_gettime, . - __kernel_clock_gettime
.size __clock_gettime, . - __clock_gettime

.global __kernel_clock_getres
.global __clock_getres
.type __kernel_clock_getres, %function
.type __clock_getres, %function
__clock_getres:
__kernel_clock_getres:
  cbz x1, 2f
  cmp w0, #1
  b.eq 1f
  cmp w0, #4
  b.eq 1f
  mov x8, #114
  svc #0
  ret
1:
  mrs x2, cntfrq_el0
  cbz x2, 9f
  movz x3, #0xca00
  movk x3, #0x3b9a, lsl #16
  udiv x3, x3, x2
  str xzr, [x1]
  str x3, [x1, #8]
2:
  mov w0, #0
  ret
9:
  mov w0, #-22
  ret
.size __kernel_clock_getres, . - __kernel_clock_getres
.size __clock_getres, . - __clock_getres

.global __kernel_gettimeofday
.type __kernel_gettimeofday, %function
__kernel_gettimeofday:
  mov x8, #169
  svc #0
  ret
.size __kernel_gettimeofday, . - __kernel_gettimeofday

.global __kernel_rt_sigreturn
.type __kernel_rt_sigreturn, %function
__kernel_rt_sigreturn:
  mov x8, #139
  svc #0
  ret
.size __kernel_rt_sigreturn, . - __kernel_rt_sigreturn
