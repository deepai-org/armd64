namespace {

class PolyVirtualBase {
public:
  virtual unsigned long eval(unsigned long value) const
  {
    return value + 1;
  }
};

class PolyVirtualDerived : public PolyVirtualBase {
public:
  explicit PolyVirtualDerived(unsigned long seed) : bias_(seed + 30) {}

  unsigned long eval(unsigned long value) const override
  {
    return value + bias_;
  }

private:
  unsigned long bias_;
};

PolyVirtualDerived global_dep_object(7);

__attribute__((noinline))
unsigned long call_virtual(const PolyVirtualBase *object, unsigned long value)
{
  return object->eval(value);
}

} // namespace

extern "C" __attribute__((visibility("default")))
unsigned long poly_cxx_virtual_dep(unsigned long seed, unsigned long value)
{
  PolyVirtualDerived local_object(seed);

  return call_virtual(&local_object, value) +
    call_virtual(&global_dep_object, seed);
}
