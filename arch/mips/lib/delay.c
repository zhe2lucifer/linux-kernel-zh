/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 by Waldorf Electronics
 * Copyright (C) 1995 - 2000, 01, 03 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2007  Maciej W. Rozycki
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/smp.h>

#include <asm/compiler.h>
#include <asm/war.h>
#include <ali_soc.h>
void __delay(unsigned long loops)
{
	__asm__ __volatile__ (
	"	.set	noreorder				\n"
	"	.align	3					\n"
	"1:	bnez	%0, 1b					\n"
#if BITS_PER_LONG == 32
	"	subu	%0, 1					\n"
#else
	"	dsubu	%0, 1					\n"
#endif
	"	.set	reorder					\n"
	: "=r" (loops)
	: "0" (loops));
}
EXPORT_SYMBOL(__delay);

/*
 * Division by multiplication: you don't have to worry about
 * loss of precision.
 *
 * Use only for very small delays ( < 1 msec).	Should probably use a
 * lookup table, really, as the multiplications take much too long with
 * short delays.  This is a "reasonable" implementation, though (and the
 * first constant multiplications gets optimized away if the delay is
 * a constant)
 */
#if 1
#ifdef CONFIG_ALI_FPGA_TEST
#define SYS_CPU_CLOCK                   (27 * 1000 * 1000)
#else
#define SYS_CPU_CLOCK                   (ali_sys_ic_get_cpu_clock() * 1000 * 1000)
#endif
#define US_TICKS    (SYS_CPU_CLOCK / 2000000)
//#define STR(x) #x
#define read_32bit_cp0_register(source)	\
	({ int __res;								\
	        __asm__ __volatile__(					\
		".set\tpush\n\t"						\
		".set\treorder\n\t"						\
	        "mfc0\t%0,"STR(source)"\n\t"			\
		".set\tpop"							\
	        : "=r" (__res));                                        \
	        __res;})

void __udelay(unsigned long usecs)
{
	unsigned long tick, old_tick;
	old_tick=read_32bit_cp0_register($9);
	tick = usecs * US_TICKS;
	while ((read_32bit_cp0_register($9) - old_tick) < tick);
	
	return;
	
}
#endif

/*
void __udelay(unsigned long us)
{
	unsigned int lpj = raw_current_cpu_data.udelay_val;

	__delay((us * 0x000010c7ull * HZ * lpj) >> 32);
}*/

EXPORT_SYMBOL(__udelay);

void __ndelay(unsigned long ns)
{
	unsigned int lpj = raw_current_cpu_data.udelay_val;

	__delay((ns * 0x00000005ull * HZ * lpj) >> 32);
}
EXPORT_SYMBOL(__ndelay);
