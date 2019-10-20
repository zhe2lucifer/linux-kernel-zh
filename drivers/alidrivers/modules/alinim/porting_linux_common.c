/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#include "porting_linux_header.h"

static int g_log_level = NIM_LOG_NULL;


UINT32 osal_get_tick(void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}


UINT32 nim_flag_read(NIM_FLAG_LOCK *flag_lock, UINT32 T1, UINT32 T2, UINT32 T3)
{
    UINT32 flag_ptn;

    read_lock(&flag_lock->flagid_rwlk);
    flag_ptn = flag_lock->flag_id;
    read_unlock(&flag_lock->flagid_rwlk);

    return flag_ptn;
}

UINT32 nim_flag_create(NIM_FLAG_LOCK *flag_lock)
{
    if (flag_lock->flag_id == OSAL_INVALID_ID)
    {
        write_lock(&flag_lock->flagid_rwlk);
        flag_lock->flag_id = 0;
        write_unlock(&flag_lock->flagid_rwlk);
    }
    return 0;
}

UINT32 nim_flag_set(NIM_FLAG_LOCK *flag_lock, UINT32 value)
{
    write_lock(&flag_lock->flagid_rwlk);
    flag_lock->flag_id |= value;
    write_unlock(&flag_lock->flagid_rwlk);
    return 0;
}

UINT32 nim_flag_clear(NIM_FLAG_LOCK *flag_lock, UINT32 value)
{
    write_lock(&flag_lock->flagid_rwlk);
    flag_lock->flag_id &= (~value);
    write_unlock(&flag_lock->flagid_rwlk);
    return 0;
}

UINT32 nim_flag_del(NIM_FLAG_LOCK *flag_lock)
{
    write_lock(&flag_lock->flagid_rwlk);
    flag_lock->flag_id = OSAL_INVALID_ID;
    write_unlock(&flag_lock->flagid_rwlk);
    return 0;
}

void nim_print_x(int log_level,const char *fmt, ...)
{
#ifdef CONFIG_PRINTK
	va_list args;
#endif

    if(log_level == g_log_level)
    {
    
#ifdef CONFIG_PRINTK
		va_start(args, fmt);
		vprintk_emit(0, -1, NULL, 0, fmt, args);
		va_end(args);
#endif

    }
	
}

void set_log_level(int log_level)
{
	if((log_level>=NIM_LOG_NULL) && (log_level<NIM_LOG_MAX))
	{
		g_log_level=log_level;
	}	
	printk("[%s]line=%d,log_level=%d\n", __FUNCTION__, __LINE__,g_log_level);
}
