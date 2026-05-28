extern "C" {
void *__dso_handle = &__dso_handle;
}

static unsigned long dep_state;

namespace {

class PolyDepDtorProbe {
public:
  PolyDepDtorProbe(unsigned long init_value, unsigned long fini_value)
      : fini_value_(fini_value)
  {
    dep_state += init_value;
  }

  ~PolyDepDtorProbe()
  {
    dep_state += fini_value_;
  }

private:
  unsigned long fini_value_;
};

PolyDepDtorProbe global_dep_probe(400, 3000);

} // namespace

extern "C" __attribute__((visibility("default")))
unsigned long poly_cxx_dep_dtor_touch(unsigned long a, unsigned long b)
{
  dep_state += a + b;
  return dep_state;
}

extern "C" __attribute__((visibility("default")))
unsigned long poly_needed_fini_result(void)
{
  return dep_state;
}
