/*
 * x86 FPU bug checks:
 */
#include <asm/fpu/internal.h>

/*
 * Boot time CPU/FPU FDIV bug detection code:
 */

#ifndef CONFIG_L4
static double __initdata x = 4195835.0;
static double __initdata y = 3145727.0;
#endif /* L4 */

/*
 * This used to check for exceptions..
 * However, it turns out that to support that,
 * the XMM trap handlers basically had to
 * be buggy. So let's have a correct XMM trap
 * handler, and forget about printing out
 * some status at boot.
 *
 * We should really only care about bugs here
 * anyway. Not features.
 */
static void __init check_fpu(void)
{
#ifndef CONFIG_L4
	u32 cr0_saved;
	s32 fdiv_bug;

	/* We might have CR0::TS set already, clear it: */
	cr0_saved = read_cr0();
	write_cr0(cr0_saved & ~X86_CR0_TS);

	kernel_fpu_begin();

	/*
	 * trap_init() enabled FXSR and company _before_ testing for FP
	 * problems here.
	 *
	 * Test for the divl bug: http://en.wikipedia.org/wiki/Fdiv_bug
	 */
	__asm__("fninit\n\t"
		"fldl %1\n\t"
		"fdivl %2\n\t"
		"fmull %2\n\t"
		"fldl %1\n\t"
		"fsubp %%st,%%st(1)\n\t"
		"fistpl %0\n\t"
		"fwait\n\t"
		"fninit"
		: "=m" (*&fdiv_bug)
		: "m" (*&x), "m" (*&y));

	kernel_fpu_end();

	write_cr0(cr0_saved);

	if (fdiv_bug) {
		set_cpu_bug(&boot_cpu_data, X86_BUG_FDIV);
		pr_warn("Hmm, FPU with FDIV bug\n");
	}
#endif /* L4 */
}

void __init fpu__init_check_bugs(void)
{
	/*
	 * kernel_fpu_begin/end() in check_fpu() relies on the patched
	 * alternative instructions.
	 */
	if (cpu_has_fpu)
		check_fpu();
}
