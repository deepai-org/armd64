#include <stdexcept>

static int throw_from_leaf(int value) {
  if (value == 7)
    throw std::runtime_error("poly exception path");
  return value;
}

static int catch_runtime_exception(void) {
  try {
    return throw_from_leaf(7);
  } catch (const std::runtime_error &error) {
    return error.what()[0] == 'p' ? 31 : 3;
  }
}

int main(void) {
  try {
    throw catch_runtime_exception() + 11;
  } catch (int value) {
    return value == 42 ? 42 : 4;
  } catch (...) {
    return 5;
  }
}
