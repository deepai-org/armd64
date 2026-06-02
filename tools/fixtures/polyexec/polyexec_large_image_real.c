static volatile unsigned char poly_large_image_bss[1024 * 1024 + 8192]
    __attribute__((used, aligned(4096)));

static unsigned long __attribute__((used, noinline))
poly_large_image_check(void) {
  poly_large_image_bss[0] = 7;
  poly_large_image_bss[4095] = 11;
  poly_large_image_bss[1024 * 1024] = 13;
  poly_large_image_bss[1024 * 1024 + 8191] = 11;

  return (unsigned long) poly_large_image_bss[0] +
    (unsigned long) poly_large_image_bss[4095] +
    (unsigned long) poly_large_image_bss[1024 * 1024] +
    (unsigned long) poly_large_image_bss[1024 * 1024 + 8191];
}

#if defined(__aarch64__)
__asm__(
  ".global _start\n"
  ".type _start, %function\n"
  "_start:\n"
  "bl poly_large_image_check\n"
  "mov x8, #93\n"
  "svc #0\n");
#elif defined(__riscv)
__asm__(
  ".global _start\n"
  ".type _start, @function\n"
  "_start:\n"
  "call poly_large_image_check\n"
  "li a7, 93\n"
  "ecall\n");
#else
#error unsupported architecture
#endif
