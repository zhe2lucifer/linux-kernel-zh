/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced security memory check function implementations.
 *
 *  Author:
 *	Zhao Owen <owen.zhao@alitech.com>
 *
 *  Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *      as published by the Free Software Foundation.
 */

#include "aliasix_memchk.h"

//just let compile go through.
extern int ali_trig_ram_mon(__u32 start_addr,__u32 end_addr, __u32 interval, \
								__u32 sha_mode, int DisableOrEnable);

/*
 * System information variables of memory
 */
#define ALIASIX_MEMCHK_INTERVAL   5
extern int _stext, _etext;
extern int __end_rodata, __start_rodata;

#ifdef CONFIG_MEMORY_PROTECTION_ALIASIX
/*
 * aliasix_memchk_set_protect - set mem protect to kernel
 *
 * return
 */
void aliasix_memchk_set_protect(void)
{
    u_long u32_memchk_start = __pa(&_stext);
    u_long u32_memchk_end = __pa(&__end_rodata);

    ALIASIX_MEMCHK("Info, memchk start 0x%x end 0x%x\n", \
                   u32_memchk_start, u32_memchk_end);
    u32_memchk_start &= 0x0FFFFFFF;
    u32_memchk_start |= 0xA0000000;
    u32_memchk_end &= 0x0FFFFFFF;
    u32_memchk_end |= 0xA0000000;
    ALIASIX_MEMCHK("Info, memchk start 0x%x end 0x%x\n", \
                   u32_memchk_start, u32_memchk_end);
    ali_trig_ram_mon(u32_memchk_start, u32_memchk_end, \
                     ALIASIX_MEMCHK_INTERVAL, SHA_SHA_256, TRUE);
    
    return;
}
#endif

