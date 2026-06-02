static volatile unsigned char poly_large_image_bss[1024 * 1024 + 8192]
    __attribute__((used, aligned(4096)));

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  poly_large_image_bss[0] += (unsigned char) a0;
  poly_large_image_bss[4095] += (unsigned char) a1;
  poly_large_image_bss[1024 * 1024] += (unsigned char) a2;
  poly_large_image_bss[1024 * 1024 + 8191] += (unsigned char) a3;
  return (unsigned long) poly_large_image_bss[0] +
    (unsigned long) poly_large_image_bss[4095] +
    (unsigned long) poly_large_image_bss[1024 * 1024] +
    (unsigned long) poly_large_image_bss[1024 * 1024 + 8191] +
    a4 + a5 + a6 + a7 + a8;
}
