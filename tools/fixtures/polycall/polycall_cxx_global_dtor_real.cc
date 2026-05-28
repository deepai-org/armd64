extern "C" {
void *__dso_handle = &__dso_handle;
}

static unsigned long global_state;

namespace {

class PolyGlobalDtorProbe {
public:
  PolyGlobalDtorProbe(unsigned long init_value, unsigned long fini_value)
      : fini_value_(fini_value)
  {
    global_state += init_value;
  }

  ~PolyGlobalDtorProbe()
  {
    global_state += fini_value_;
  }

private:
  unsigned long fini_value_;
};

PolyGlobalDtorProbe global_probe(300, 2000);

} // namespace

extern "C" __attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  global_state += a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  return global_state;
}

extern "C" __attribute__((visibility("default")))
unsigned long poly_fini_result(void)
{
  return global_state;
}
