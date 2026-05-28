struct poly_compact_u32_f32 {
  unsigned int i;
  float f;
};

struct poly_compact_f32_u32 {
  float f;
  unsigned int i;
};

__asm__(
  ".section .note.polyabi,\"a\",%note\n"
  ".balign 4\n"
  ".long 8\n"
  ".long 2f-1f\n"
  ".long 1\n"
  ".asciz \"POLYABI\"\n"
  ".balign 4\n"
  "1: .ascii \"poly_cross_compact_u32_f32 compact_u32_f32\\n\"\n"
  "   .ascii \"poly_cross_compact_f32_u32 compact_f32_u32\\n\"\n"
  "2:\n"
  ".balign 4\n"
  ".previous\n");

__attribute__((visibility("default")))
struct poly_compact_u32_f32 poly_cross_compact_u32_f32(
    struct poly_compact_u32_f32 in, unsigned int scale)
{
  struct poly_compact_u32_f32 out;
  out.i = in.i + scale + 100;
  out.f = in.f + (float) scale + 25.0f;
  return out;
}

__attribute__((visibility("default")))
struct poly_compact_f32_u32 poly_cross_compact_f32_u32(
    struct poly_compact_f32_u32 in, unsigned int scale)
{
  struct poly_compact_f32_u32 out;
  out.f = in.f + (float) scale + 50.0f;
  out.i = in.i + scale + 200;
  return out;
}
