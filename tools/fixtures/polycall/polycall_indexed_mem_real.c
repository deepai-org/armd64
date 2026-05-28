static volatile unsigned long table[4] = { 11, 22, 33, 44 };

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  unsigned long load_index = (a0 + a8) & 3;
  unsigned long store_index = (a1 + 1) & 3;
  unsigned long unused_args = (a3 + a4 + a5 + a6 + a7) & 0;

  table[store_index] = a2 + 5;
  return table[load_index] + table[store_index] + unused_args;
}
