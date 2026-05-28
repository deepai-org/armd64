struct poly_compact_u32_f32 {
  unsigned int i;
  float f;
};

struct poly_compact_f32_u32 {
  float f;
  unsigned int i;
};

union poly_float_bits {
  float f;
  unsigned int u;
};

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_cross_ifunc_compact_u32_f32 compact_u32_f32\\n\"\n"
  "   .ascii \"poly_cross_ifunc_compact_f32_u32 compact_f32_u32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

static struct poly_compact_u32_f32 poly_cross_ifunc_compact_impl(
    struct poly_compact_u32_f32 in, unsigned int scale)
{
  struct poly_compact_u32_f32 out;
  out.i = in.i + scale + 300;
  out.f = in.f + (float) scale + 75.0f;
  return out;
}

static void *poly_cross_ifunc_compact_resolver(void)
{
  return poly_cross_ifunc_compact_impl;
}

static struct poly_compact_f32_u32 poly_cross_ifunc_compact_rev_impl(
    struct poly_compact_f32_u32 in, unsigned int scale)
{
  struct poly_compact_f32_u32 out;
  out.f = in.f + (float) scale + 150.0f;
  out.i = in.i + scale + 500;
  return out;
}

static void *poly_cross_ifunc_compact_rev_resolver(void)
{
  return poly_cross_ifunc_compact_rev_impl;
}

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_cross_ifunc_compact_u32_f32(
    struct poly_compact_u32_f32, unsigned int)
    __attribute__((ifunc("poly_cross_ifunc_compact_resolver")));

__attribute__((visibility("default")))
struct poly_compact_f32_u32 poly_cross_ifunc_compact_f32_u32(
    struct poly_compact_f32_u32, unsigned int)
    __attribute__((ifunc("poly_cross_ifunc_compact_rev_resolver")));
