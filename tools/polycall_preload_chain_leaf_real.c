static unsigned long poly_preload_chain_leaf_state;

__attribute__((visibility("default")))
void poly_preload_chain_leaf_init(void)
{
  poly_preload_chain_leaf_state = 3900;
}

__attribute__((visibility("default")))
void poly_preload_chain_leaf_fini(void)
{
  poly_preload_chain_leaf_state += 1000;
}

__attribute__((visibility("default")))
unsigned long poly_preload_add(unsigned long a, unsigned long b)
{
  return a + b + poly_preload_chain_leaf_state;
}

__attribute__((visibility("default")))
unsigned long poly_needed_fini_result(void)
{
  return poly_preload_chain_leaf_state;
}
