extern unsigned long poly_many_needed_value_1(void);
extern unsigned long poly_many_needed_value_2(void);
extern unsigned long poly_many_needed_value_3(void);
extern unsigned long poly_many_needed_value_4(void);
extern unsigned long poly_many_needed_value_5(void);
extern unsigned long poly_many_needed_value_6(void);
extern unsigned long poly_many_needed_value_7(void);
extern unsigned long poly_many_needed_value_8(void);
extern unsigned long poly_many_needed_value_9(void);

__attribute__((visibility("default")))
unsigned long poly_entry(unsigned long a0, unsigned long a1,
    unsigned long a2, unsigned long a3, unsigned long a4,
    unsigned long a5, unsigned long a6, unsigned long a7,
    unsigned long a8)
{
  return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 +
    poly_many_needed_value_1() + poly_many_needed_value_2() +
    poly_many_needed_value_3() + poly_many_needed_value_4() +
    poly_many_needed_value_5() + poly_many_needed_value_6() +
    poly_many_needed_value_7() + poly_many_needed_value_8() +
    poly_many_needed_value_9();
}
