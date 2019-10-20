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
#ifndef _ASM_DELAY_H
#define _ASM_DELAY_H

#include <linux/param.h>

//#include <asm/mach-ali/unified_bsp_board_config.h>
#ifdef CONFIG_ALI_FPGA_TEST
//#define SYS_CPU_CLOCK                   (27 * 1000 * 1000)
#define US_TICKS    (27000000 / 2000000)
#else
#define SYS_CPU_CLOCK                   (ali_sys_ic_get_cpu_clock() * 1000 * 1000)
#define US_TICKS    (SYS_CPU_CLOCK / 2000000)
#endif
#define read_32bit_cp0_register(source)	\
	({ int __res;								\
	        __asm__ __volatile__(					\
		".set\tpush\n\t"						\
		".set\treorder\n\t"						\
	        "mfc0\t%0,"STR(source)"\n\t"			\
		".set\tpop"							\
	        : "=r" (__res));                                        \
	        __res;})

void __delay(unsigned int loops);
void __ndelay(unsigned long ns);
void __udelay(unsigned long usecs);

#define ndelay(ns) __ndelay(ns)
#define udelay(us) __udelay(us)
#define ali_udelay(usecs) __udelay((usecs))

/* make sure "usecs *= ..." in udelay do not overflow. */
#if HZ >= 1000
#define MAX_UDELAY_MS	1
#elif HZ <= 200
#define MAX_UDELAY_MS	5
#else
#define MAX_UDELAY_MS	(1000 / HZ)
#endif

#endif /* _ASM_DELAY_H */
