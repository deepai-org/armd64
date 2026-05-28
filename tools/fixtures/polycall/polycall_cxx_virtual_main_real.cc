extern "C" unsigned long poly_cxx_virtual_dep(unsigned long, unsigned long);

namespace {

class PolyVirtualBase {
public:
  virtual unsigned long eval(unsigned long value) const
  {
    return value + 2;
  }
};

class PolyVirtualDerived : public PolyVirtualBase {
public:
  explicit PolyVirtualDerived(unsigned long seed) : bias_(seed + 11) {}

  unsigned long eval(unsigned long value) const override
  {
    return value + bias_;
  }

private:
  unsigned long bias_;
};

PolyVirtualDerived global_main_object(4);

__attribute__((noinline))
unsigned long call_virtual(const PolyVirtualBase *object, unsigned long value)
{
  return object->eval(value);
}

} // namespace

extern "C" __attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long result = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
  PolyVirtualDerived local_object(a2);

  result += poly_cxx_virtual_dep(a0, a1);
  result += poly_cxx_virtual_dep(a7, a8);
  result += call_virtual(&local_object, a3);
  result += call_virtual(&global_main_object, a4);
  return result;
}
