#include <linux/init.h>
#include <linux/module.h>
#include <linux/smp.h>

#define POLY_XCR0_BIT 20
#define POLY_XCR0_MASK (1ULL << POLY_XCR0_BIT)

static inline unsigned long long poly_xgetbv(unsigned int index)
{
	unsigned int eax;
	unsigned int edx;

	asm volatile("xgetbv"
		     : "=a" (eax), "=d" (edx)
		     : "c" (index)
		     : "memory");
	return ((unsigned long long) edx << 32) | eax;
}

static inline void poly_xsetbv(unsigned int index, unsigned long long value)
{
	unsigned int eax = (unsigned int) value;
	unsigned int edx = (unsigned int) (value >> 32);

	asm volatile("xsetbv"
		     :
		     : "c" (index), "a" (eax), "d" (edx)
		     : "memory");
}

static void poly_enable_xcr0_cpu(void *unused)
{
	unsigned long long xcr0 = poly_xgetbv(0);

	(void) unused;
	if ((xcr0 & POLY_XCR0_MASK) == 0)
		poly_xsetbv(0, xcr0 | POLY_XCR0_MASK);
}

static int __init poly_xcr0_init(void)
{
	on_each_cpu(poly_enable_xcr0_cpu, NULL, 1);
	pr_info("poly_xcr0: requested XCR0 bit %u on all online CPUs\n",
		POLY_XCR0_BIT);
	return 0;
}

static void __exit poly_xcr0_exit(void)
{
	pr_info("poly_xcr0: unloaded\n");
}

module_init(poly_xcr0_init);
module_exit(poly_xcr0_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Prototype Poly XCR0 enablement module");
