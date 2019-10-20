/*
 * ali_interrupt.h
 *
 * Copyright (C) 2013 ALi, Inc.
 *
 * Author:
 *	Tony Zhang <tony.zhang@alitech.com>
 *
 */

#ifndef __ALI_SHM_H
#define __ALI_SHM_H

#if defined(CONFIG_ARM)
	#define ALI_VMEM_BASE   0x00000000
#else
	#define ALI_VMEM_BASE   0xA0000000
#endif

#define __VMEMALI(x)    (((unsigned long)x) | ALI_VMEM_BASE)

#if defined(CONFIG_ARM)
/*Main to SEE Address translation*/
#define __VMTSALI(x)    ((((unsigned long)x) & 0x1FFFFFFF) | 0xA0000000)

/*SEE to Main Address translation*/
#define __VSTMALI(x)    ((((unsigned long)x) & 0x1FFFFFFF) | 0xC0000000)
#else
/*Main to SEE Address translation*/
#define __VMTSALI(x)    ((unsigned long)x)

/*SEE to Main Address translation*/
#define __VSTMALI(x)    ((unsigned long)x)
#endif

#endif

