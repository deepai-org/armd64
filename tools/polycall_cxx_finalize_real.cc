extern "C" {
void *__dso_handle = &__dso_handle;
int __cxa_finalize(void *);
}

static unsigned long finalize_state;

namespace {

class PolyFinalizeProbe {
public:
  PolyFinalizeProbe(unsigned long init_value, unsigned long fini_value)
      : fini_value_(fini_value)
  {
    finalize_state += init_value;
  }

  ~PolyFinalizeProbe()
  {
    finalize_state += fini_value_;
  }

private:
  unsigned long fini_value_;
};

PolyFinalizeProbe global_finalize_probe(400, 3000);

} // namespace

extern "C" __attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  finalize_state += a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  __cxa_finalize(__dso_handle);
  return finalize_state;
}
