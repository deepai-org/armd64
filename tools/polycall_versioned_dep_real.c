unsigned long poly_versioned_add_v1(unsigned long a, unsigned long b)
{
  return a + b + 1000;
}

unsigned long poly_versioned_add_v2(unsigned long a, unsigned long b)
{
  return a + b + 2000;
}

__asm__(".symver poly_versioned_add_v1, poly_versioned_add@POLY_1.0");
__asm__(".symver poly_versioned_add_v2, poly_versioned_add@@POLY_2.0");
