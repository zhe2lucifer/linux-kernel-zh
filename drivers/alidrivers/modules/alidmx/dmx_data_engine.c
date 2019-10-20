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


#include <linux/version.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/sched.h>
#include <linux/sched/rt.h>
#else
#include <linux/sched.h>
#endif
#include <linux/kthread.h>
#include <linux/time.h>

#include <asm/div64.h>

#include <ali_cache.h>

#include "dmx_stack.h"
#include "dmx_dbg.h"

#define CONFIG_DVB_ALI_M36_DMX_CACHE
#define M_DEMUX_DATA_ENGINE_CACHE_BUF_LEVEL_MAX 85
#define M_DEMUX_DATA_ENGINE_CACHE_RETRACE_BUF_LEVEL_DROP 5

#define M_DEMUX_FULSH_ALL

struct dmx_data_engine_module ali_dmx_data_engine_module;
struct Ali_DmxKernGlobalStatInfo g_stat_info;

static struct timeval start, end;
static __u32 interval;
static __u32 retrace_count;
static __u32 retrace_drop_count;
static __u32 normal_count;
static __u32 retrace_overflow_count;



#ifdef DMX_USE_CACHE_BUF_ADDR
__s32 dmx_hw_buf_invalidate
(
    __u32  hw_buf_base,
    __u32  rd_idx,
    __u32  wr_idx,
    __u32  buf_cnt
)
{
    __u32 inv_len;

    /* Invalidate cach in HW buffer.
     */
    if (rd_idx > wr_idx)
    {
        /* rd -> end
        */
        inv_len = (buf_cnt - rd_idx) * 188;
    
        if (0 != inv_len)
        {
            __CACHE_LINEALIGN_INV_ALI(hw_buf_base + (rd_idx * 188), inv_len);
        }

        /* start -> wr
        */  
        inv_len = wr_idx * 188;
    
        if (0 != inv_len)
        {
            __CACHE_LINEALIGN_INV_ALI(hw_buf_base, inv_len);
        }
    }
    else
    {
        /* rd -> wr
        */
        inv_len = (wr_idx - rd_idx) * 188;
    
        if (0 != inv_len)
        {
            __CACHE_LINEALIGN_INV_ALI(hw_buf_base + (rd_idx * 188), inv_len);
        }
    }

    return(0);
}
#endif


static int timeval_subtract (struct timeval *result, struct timeval *x,struct timeval *y)  
{  
	/* Perform the carry for the later subtraction by updating y. */  
	if (x->tv_usec < y->tv_usec) {  
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;  
		y->tv_usec -= 1000000 * nsec;  
		y->tv_sec += nsec;  
	}  
	if (x->tv_usec - y->tv_usec > 1000000) {  
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;  
		y->tv_usec += 1000000 * nsec;  
		y->tv_sec -= nsec;  
	}  

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */  
	result->tv_sec = x->tv_sec - y->tv_sec;  
	result->tv_usec = x->tv_usec - y->tv_usec;  

	/* Return 1 if result is negative. */  
	return x->tv_sec < y->tv_sec;
}


static void i_dmx_data_engine_cache_bypass_enable(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	dmx_pid_flt_all_disable(src_hw_interface_id);

	engine->cache_tp_flt_id = dmx_pid_flt_register(src_hw_interface_id, DMX_EXCLUDE_1FFF_PID, NULL, 0);

	engine->cache_param.type = DMX_CACHE_TP;

	return;
}


static void i_dmx_data_engine_cache_bypass_disable(__u32 src_hw_interface_id, struct Ali_DmxCacheParam *p_dmx_cache_param)
{
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	dmx_pid_flt_unregister(engine->cache_tp_flt_id);

	engine->cache_param.type = p_dmx_cache_param->type;

	return;

}


static __s32 i_dmx_data_engine_cache_pid_exist(__u16 pid, __u16 *pid_list, __u32 pid_list_len)
{
	int i = 0;
	int index = -1;
	
	for (i = 0; i < pid_list_len; i++)
	{
		if(pid == pid_list[i])
		{
			index = i;
			break;
		}
	}

	return index;
}


static void i_dmx_data_engine_cache_add_pids(__u32 src_hw_interface_id, struct Ali_DmxCacheParam *p_dmx_cache_param)
{
	int i = 0;
	struct dmx_data_engine_output *engine = NULL;
	int index = -1;
	__s32 tmp_cache_pid_flt_id[DMX_CACHE_PID_LIST_MAX_LEN] = {0};

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	for (i = 0; i < p_dmx_cache_param->pid_list_len; i++)
	{
		index = i_dmx_data_engine_cache_pid_exist(p_dmx_cache_param->pid_list[i], engine->cache_param.pid_list, engine->cache_param.pid_list_len);
		if(index >= 0)
		{
			tmp_cache_pid_flt_id[i] = engine->cache_pid_flt_id[index];
			engine->cache_pid_flt_id[index] = -1;
			
			engine->cache_param.pid_list[index] = 0x1fff;
		}
		else
		{
			tmp_cache_pid_flt_id[i] = dmx_pid_flt_register(src_hw_interface_id, p_dmx_cache_param->pid_list[i] & 0x1fff, NULL, 0);
		}
	}

	engine->cache_param.type = p_dmx_cache_param->type;
	engine->cache_param.pid_list_len = p_dmx_cache_param->pid_list_len;
	memcpy(&(engine->cache_param.pid_list[0]), p_dmx_cache_param->pid_list, sizeof(__u16) * p_dmx_cache_param->pid_list_len);
	
	memcpy(engine->cache_pid_flt_id, tmp_cache_pid_flt_id, sizeof(__s32) * p_dmx_cache_param->pid_list_len);

	return;
}


static void i_dmx_data_engine_cache_del_pids(__u32 src_hw_interface_id, struct Ali_DmxCacheParam *p_dmx_cache_param)
{
	int i = 0;
	struct dmx_data_engine_output *engine = NULL;
	int index = -1;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((p_dmx_cache_param->type == DMX_NO_CACHE) || (p_dmx_cache_param->type == DMX_CACHE_TP))
	{
		for (i = 0; i < engine->cache_param.pid_list_len; i++)
		{
			dmx_pid_flt_unregister(engine->cache_pid_flt_id[i]);
			engine->cache_pid_flt_id[i] = -1;

			engine->cache_param.pid_list[i] = 0x1fff;
		}

		engine->cache_param.pid_list_len = 0;
		engine->cache_param.type = p_dmx_cache_param->type;
	}
	else if(p_dmx_cache_param->type == DMX_CACHE_PID)
	{
		for (i = 0; i < engine->cache_param.pid_list_len; i++)
		{
			index = i_dmx_data_engine_cache_pid_exist(engine->cache_param.pid_list[i], p_dmx_cache_param->pid_list, p_dmx_cache_param->pid_list_len);
			/* if pid not set in p_dmx_cache_param, we will delete it */
			if(index < 0)
			{
				dmx_pid_flt_unregister(engine->cache_pid_flt_id[i]);
				engine->cache_pid_flt_id[i] = -1;

				engine->cache_param.pid_list[i] = 0x1fff;
			}
		}

		engine->cache_param.type = p_dmx_cache_param->type;
	}

	return;
}


__u32 i_dmx_data_engine_cache_rd_get(__u32 src_hw_interface_id)
{
	__u32 rd = 0;
	struct dmx_data_engine_output *engine = NULL;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if(engine->cache_param.type == DMX_NO_CACHE)
	{
		rd = dmx_hw_buf_rd_get(src_hw_interface_id);
	}
	else if((engine->cache_param.type == DMX_CACHE_PID) || (engine->cache_param.type == DMX_CACHE_TP))
	{
		if(engine->cache_retrace_status == DMX_RETRACE_STOP)
		{
			rd = engine->buf_swrd;
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_SET)
		{
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		{
			rd = engine->buf_retrace_swrd;
		}		
	}
	else
	{
		rd = dmx_hw_buf_rd_get(src_hw_interface_id);
	}

	return rd;
}


__u32 i_dmx_data_engine_cache_rd_set(__u32 src_hw_interface_id, __u32 rd)
{
	struct dmx_data_engine_output *engine = NULL;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if(engine->cache_param.type == DMX_NO_CACHE)
	{
		dmx_hw_buf_rd_set(src_hw_interface_id, rd);
	}
	else if((engine->cache_param.type == DMX_CACHE_PID) || (engine->cache_param.type == DMX_CACHE_TP))
	{
		if(engine->cache_retrace_status == DMX_RETRACE_STOP)
		{
			engine->buf_swrd = rd;
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_SET)
		{
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		{
			engine->buf_retrace_swrd = rd;
		}		
	}
	else
	{
		dmx_hw_buf_rd_set(src_hw_interface_id, rd);
	}

	return rd;
}

__u32 i_dmx_data_engine_cache_wr_get(__u32 src_hw_interface_id)
{
	__u32 wr = 0;
	struct dmx_data_engine_output *engine = NULL;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if(engine->cache_param.type == DMX_NO_CACHE)
	{
		wr = dmx_hw_buf_wr_get(src_hw_interface_id);
	}
	else if((engine->cache_param.type == DMX_CACHE_PID) || (engine->cache_param.type == DMX_CACHE_TP))
	{
		if(engine->cache_retrace_status == DMX_RETRACE_STOP)
		{
			wr = dmx_hw_buf_wr_get(src_hw_interface_id);
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_SET)
		{
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		{
			wr = engine->buf_swrd;
		}		
	}
	else
	{
		wr = dmx_hw_buf_wr_get(src_hw_interface_id);
	}

	return wr;
}


void i_dmx_data_engine_cache_adjust_bylevel(__u32 src_hw_interface_id)
{
	__u32 rd = 0;
	__u32 wr = 0;
	__s32 avail = 0;
	__u32 rdadjust = 0;
	struct dmx_data_engine_output *engine = NULL;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_status == DMX_CACHE_POSTINIT)
		&& ((engine->cache_retrace_status == DMX_RETRACE_STOP) || (engine->cache_retrace_status == DMX_RETRACE_POSTSTART)))
	{
		rd = dmx_hw_buf_rd_get(src_hw_interface_id);
		if(engine->cache_retrace_status == DMX_RETRACE_STOP)
			wr = dmx_hw_buf_wr_get(src_hw_interface_id);
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
			wr = engine->buf_retrace_swrd;

		avail = wr - rd;
		if (avail < 0)
			avail += engine->hw_pkt_buf_cnt;

		/* if buf level > M_DEMUX_DATA_ENGINE_CACHE_BUF_LEVEL_MAX%, we will adjust rd */
		if((engine->cache_retrace_status == DMX_RETRACE_STOP) 
			&& (avail > (engine->hw_pkt_buf_cnt * M_DEMUX_DATA_ENGINE_CACHE_BUF_LEVEL_MAX / 100)))
		{
			rdadjust = avail - (engine->hw_pkt_buf_cnt * M_DEMUX_DATA_ENGINE_CACHE_BUF_LEVEL_MAX / 100);
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		{
			if(avail > (engine->hw_pkt_buf_cnt * M_DEMUX_DATA_ENGINE_CACHE_RETRACE_BUF_LEVEL_DROP / 100))
				rdadjust = engine->hw_pkt_buf_cnt * M_DEMUX_DATA_ENGINE_CACHE_RETRACE_BUF_LEVEL_DROP / 100;
			else
				rdadjust = avail;
		}

		//printk(KERN_NOTICE "%s,%d,hw_pkt_buf_cnt:%u,rdadjust:%u\n", __FUNCTION__, __LINE__, engine->hw_pkt_buf_cnt, rdadjust);
		if(rdadjust)
		{
			while(rdadjust)
			{
				rdadjust--;
				rd++;
				if (rd > (engine->hw_pkt_buf_cnt - 1))
				{
					rd = 0;
				}
			}
			
			dmx_hw_buf_rd_set(src_hw_interface_id, rd);
		}

	}

	return;
}

__s32 i_dmx_data_engine_cache_checkvalid(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_status == DMX_CACHE_PREINIT))
	{	
		return 0;
	}

	return -1;
}


void i_dmx_data_engine_cache_valid(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_status == DMX_CACHE_PREINIT))
	{	
		engine->buf_swrd = dmx_hw_buf_rd_get(src_hw_interface_id);

		engine->cache_status = DMX_CACHE_POSTINIT;

		printk(KERN_NOTICE "%s,%d\n", __FUNCTION__, __LINE__);
	}

	return;
}


__s32 i_dmx_data_engine_cache_checkretrace(__u32 src_hw_interface_id)
{
	__s32 ret = -1;
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& ((engine->cache_retrace_status == DMX_RETRACE_SET) || (engine->cache_retrace_status == DMX_RETRACE_PRESTART)))
	{	
		ret = 0;
	}

	return ret;
}


__s32 i_dmx_data_engine_cache_retracewait(__u32 src_hw_interface_id)
{
	__s32 ret = -1;
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_retrace_status == DMX_RETRACE_SET))
	{
		ret = 0;
	}

	return ret;
}


void i_dmx_data_engine_cache_retracevalid(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;
	__s32 avail = 0;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_retrace_status == DMX_RETRACE_PRESTART))
	{	
		engine->buf_retrace_swrd = dmx_hw_buf_rd_get(src_hw_interface_id);
	
		engine->cache_retrace_status = DMX_RETRACE_POSTSTART;

		avail = engine->buf_swrd - engine->buf_retrace_swrd;
		if (avail < 0)
			avail += engine->hw_pkt_buf_cnt;

		DMX_PRINTK(KERN_NOTICE "%s,%d,retrace avail:%d\n", __FUNCTION__, __LINE__, avail);

		do_gettimeofday(&start);

		retrace_count = 0;
		retrace_drop_count = 0;
		normal_count = 0;
		retrace_overflow_count = 0;
		
	}

	return;
}


void i_dmx_data_engine_cache_retracefinish(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if((engine->cache_param.type != DMX_NO_CACHE)
		&& (engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		&& (engine->buf_retrace_swrd == engine->buf_swrd))
	{	
		engine->cache_retrace_status = DMX_RETRACE_STOP;

		do_gettimeofday(&end);

		interval = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);

		DMX_PRINTK(KERN_NOTICE "%s,%d,retrace time:%dms,hw:%d,count:%d,drop count:%d,retrace_overflow_count:%d\n", __FUNCTION__, __LINE__, 
							interval/1000, engine->hw_pkt_buf_cnt, retrace_count, retrace_drop_count, retrace_overflow_count);
	}

	return;
}


__s32 i_dmx_data_engine_cache_checkpid(__u32 src_hw_interface_id, __u16 pid)
{
	int i = 0;
	__s32 ret = -1;
	struct dmx_data_engine_output *engine = NULL;

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if(engine->cache_param.type != DMX_NO_CACHE)
	{
		if(engine->cache_retrace_status == DMX_RETRACE_STOP)
		{
			//printk(KERN_NOTICE "%s,%d,pid:%d\n", __FUNCTION__, __LINE__, pid);
			ret = 0;
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_SET)
		{
			ret = -1;
		}
		else if(engine->cache_retrace_status == DMX_RETRACE_POSTSTART)
		{
            if (engine->cache_param.type == DMX_CACHE_PID)
            {
                ret = 0;
                retrace_count++;
            }
            else
            {
    			for (i = 0; i < engine->cache_retrace_param.pid_list_len; i++)
    			{
    				if(engine->cache_retrace_param.pid_list[i] == pid)
    				{
    					ret = 0;
    					retrace_count++;
    					//printk(KERN_NOTICE "%s,%d,pid:%d\n", __FUNCTION__, __LINE__, pid);
    					break;
    				}
    			}
            }

			if(ret != 0)
			{
				//retrace_drop_count++;
			}
		}
	}
	else
	{
		/*
		for (i = 0; i < engine->cache_retrace_param.pid_list_len; i++)
		{
			if((engine->cache_retrace_param.pid_list[i] & 0x1fff) == pid)
			{
				printk(KERN_NOTICE "%s,%d,pid:%d\n", __FUNCTION__, __LINE__, pid);
				break;
			}
		}		
		*/

		//normal_count++;
		//if(normal_count%1000 == 0)
			//printk(KERN_NOTICE "%s,%d,noraml_countcount:%d\n", __FUNCTION__, __LINE__, normal_count);
		//printk(KERN_NOTICE "%s,%d,pid:%d\n", __FUNCTION__, __LINE__, pid);

		ret = 0;
	}
	
	return ret;
}


__s32 i_dmx_data_engine_bitrate_checkpid(__u32 src_hw_interface_id, __u16 pid)
{
	struct dmx_data_engine_output *engine = NULL;
	__u32 index = 0;
	__u32 pid_match = 0;
	
    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_BITRATE_ERROR);
    }

	if(pid == DMX_INVALID_PID)
	{
		return 0;
	}

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if(engine->m_bitrate_param.pid_list_length == 0)
	{
		engine->pkts_num_bitrate_pidlist++;
	}
	else
	{
		pid_match = 0;
		for(index = 0;index < engine->m_bitrate_param.pid_list_length;index++)
		{
			if(pid == engine->m_bitrate_param.pid_list[index])
			{
				pid_match = 1;
				break;
			}
		}				
		
		if(pid_match)
		{
			engine->pkts_num_bitrate_pidlist++;
		}
	}

	return 0;
}

__s32 i_dmx_data_engine_bitrate_calc(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;
	__u8 cnt = 0, valid_cnt = 0;
	__u32 bit_rate = 0;
	struct timeval current_time;
	struct timeval result;
	__u32 timedelta;
	
    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_BITRATE_ERROR);
    }

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	do_gettimeofday(&current_time);

	timeval_subtract(&result, &current_time, &engine->last_time);

	timedelta = result.tv_sec * 1000 + result.tv_usec/1000;

	/*
	printk(KERN_NOTICE "%s,%d,timedelta:%u, pkgnum:%u, %u\n", __FUNCTION__, __LINE__, timedelta, 
						engine->pkts_num_bitrate_pidlist,
						engine->last_pkts_num_bitrate_pidlist);
	*/

	if ((engine->bitrate_detect == 1) && (timedelta > 60))
	{
		bit_rate = (engine->pkts_num_bitrate_pidlist - engine->last_pkts_num_bitrate_pidlist) * 188 * 8  * 1000 / timedelta;
		
		for(cnt = 0; cnt < 7; cnt++)
		{
			engine->last_rate[cnt] = engine->last_rate[cnt + 1];

			if(engine->last_rate[cnt] != 0)
				valid_cnt++;
		}
		
		engine->last_rate[7] = bit_rate;
		if(engine->last_rate[7] != 0)
			valid_cnt++;

		if(valid_cnt != 0)
			engine->bitrate = (engine->last_rate[0] + engine->last_rate[1] + engine->last_rate[2] + engine->last_rate[3] +
								engine->last_rate[4] + engine->last_rate[5] + engine->last_rate[6] + engine->last_rate[7])/valid_cnt;


		engine->last_pkts_num_bitrate_pidlist = engine->pkts_num_bitrate_pidlist;
		do_gettimeofday(&engine->last_time);

		/*
		printk(KERN_NOTICE "%s,%d,bitrate:%u %u %u %u %u %u %u %u \n", __FUNCTION__, __LINE__,
							engine->last_rate[0],
							engine->last_rate[1],
							engine->last_rate[2],
							engine->last_rate[3],
							engine->last_rate[4],
							engine->last_rate[5],
							engine->last_rate[6],
							engine->last_rate[7]);
		*/
	}	

	return 0;
}


__s32 dmx_data_engine_cache_set(__u32 src_hw_interface_id, struct Ali_DmxCacheParam *p_dmx_cache_param)
{
	int i = 0;
    __s32 ret = 0;
    struct dmx_data_engine_output *engine = NULL;
	struct AliDmxCacheParam *p_dmx_cache_param_org = NULL;
	
	if(!p_dmx_cache_param)
	{
		printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
		return DMX_DATA_ENGINE_CACHE_ERROR;
	}

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }

	if(p_dmx_cache_param->type == DMX_CACHE_PID)
	{
        if (p_dmx_cache_param->pid_list_len == 0)
        {
            ret = dmx_data_engine_cache_hwbuf_reset(src_hw_interface_id);
        }

		for (i = 0; i < p_dmx_cache_param->pid_list_len; i++)
		{
			p_dmx_cache_param->pid_list[i] = p_dmx_cache_param->pid_list[i] & 0x1fff;
		}	
	}
    
    engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];
	p_dmx_cache_param_org = &engine->cache_param;

	if(p_dmx_cache_param_org->type == DMX_NO_CACHE)
	{
		if(p_dmx_cache_param->type == DMX_CACHE_PID)
		{
			i_dmx_data_engine_cache_add_pids(src_hw_interface_id, p_dmx_cache_param);

			engine->cache_status = DMX_CACHE_PREINIT;
		}
		else if(p_dmx_cache_param->type == DMX_CACHE_TP)
		{
			i_dmx_data_engine_cache_bypass_enable(src_hw_interface_id);

			engine->cache_status = DMX_CACHE_PREINIT;
		}
	}
	else if(p_dmx_cache_param_org->type == DMX_CACHE_PID)
	{
		if(p_dmx_cache_param->type == DMX_NO_CACHE)
		{
			i_dmx_data_engine_cache_del_pids(src_hw_interface_id, p_dmx_cache_param);
            ret = dmx_data_engine_cache_hwbuf_reset(src_hw_interface_id);

			engine->cache_status = DMX_CACHE_UNINIT;
		}
		else if(p_dmx_cache_param->type == DMX_CACHE_PID)
		{
			i_dmx_data_engine_cache_del_pids(src_hw_interface_id, p_dmx_cache_param);
			i_dmx_data_engine_cache_add_pids(src_hw_interface_id, p_dmx_cache_param);

			//engine->cache_status = DMX_CACHE_PREINIT;
		}
		else if(p_dmx_cache_param->type == DMX_CACHE_TP)
		{
			i_dmx_data_engine_cache_del_pids(src_hw_interface_id, p_dmx_cache_param);
			i_dmx_data_engine_cache_bypass_enable(src_hw_interface_id);

			//engine->cache_status = DMX_CACHE_PREINIT;
		}		
	}
	else if(p_dmx_cache_param_org->type == DMX_CACHE_TP)
	{
		if(p_dmx_cache_param->type == DMX_NO_CACHE)
		{
			i_dmx_data_engine_cache_bypass_disable(src_hw_interface_id, p_dmx_cache_param);
            ret = dmx_data_engine_cache_hwbuf_reset(src_hw_interface_id);

			engine->cache_status = DMX_CACHE_UNINIT;
		}
		else if(p_dmx_cache_param->type == DMX_CACHE_PID)
		{
			i_dmx_data_engine_cache_bypass_disable(src_hw_interface_id, p_dmx_cache_param);
			i_dmx_data_engine_cache_add_pids(src_hw_interface_id, p_dmx_cache_param);

			//engine->cache_status = DMX_CACHE_PREINIT;
		}
	}

	printk(KERN_NOTICE "%s,%d,cache type:%d\n", __FUNCTION__, __LINE__, p_dmx_cache_param_org->type);

	return 0;
}


__s32 dmx_data_engine_cache_hwbuf_reset(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];
	
	engine->cache_hwbuf_flush = true;

	dmx_hw_buf_rd_set(engine->src_hw_interface_id, dmx_hw_buf_wr_get(engine->src_hw_interface_id));

	printk(KERN_NOTICE "%s,%d\n", __FUNCTION__, __LINE__);

	return 0;
}


enum Ali_DmxCacheType dmx_data_engine_cache_typeget(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	return engine->cache_param.type;
}


__s32 dmx_data_engine_cache_retrace_set(__u32 src_hw_interface_id, struct Ali_DmxCacheRetraceParam *p_cache_retrace_param)
{
	int i = 0;
	struct dmx_data_engine_output *engine = NULL;

	if(!p_cache_retrace_param)
	{
		printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
		return DMX_DATA_ENGINE_CACHE_ERROR;
	}

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }

	for (i = 0; i < p_cache_retrace_param->pid_list_len; i++)
	{
		p_cache_retrace_param->pid_list[i] = p_cache_retrace_param->pid_list[i] & 0x1fff;
		DMX_PRINTK(KERN_NOTICE "%d\n", p_cache_retrace_param->pid_list[i]);
	}	

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	memcpy(&engine->cache_retrace_param, p_cache_retrace_param, sizeof(struct Ali_DmxCacheRetraceParam));

	engine->cache_retrace_status = DMX_RETRACE_SET;

	DMX_PRINTK(KERN_NOTICE "%s,%d\n", __FUNCTION__, __LINE__);	

	return 0;
}


__s32 dmx_data_engine_cache_retrace_start(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }


	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	engine->cache_retrace_status = DMX_RETRACE_PRESTART;

	DMX_PRINTK(KERN_NOTICE "%s,%d\n", __FUNCTION__, __LINE__);

	return 0;
}


enum Ali_DmxCacheType dmx_data_engine_retrace_statusget(__u32 src_hw_interface_id)
{
	struct dmx_data_engine_output *engine = NULL;

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_CACHE_ERROR);
    }

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	return engine->cache_retrace_status;
}


__s32 dmx_data_engine_set_bitrateparam(__u32 src_hw_interface_id, AliDmxBitrateParam_S *p_bitrate_param)
{	
	struct dmx_data_engine_output *engine = NULL;
	int index = 0;
	int param_same = 0;

    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
    	printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
        return(DMX_DATA_ENGINE_BITRATE_ERROR);
    }

	if(!p_bitrate_param)
	{
		printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
		return DMX_DATA_ENGINE_BITRATE_ERROR;
	}	

	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	/* param same judge(simple judge) */
	if(p_bitrate_param->pid_list_length == engine->m_bitrate_param.pid_list_length)
	{
		for(index = 0;index < engine->m_bitrate_param.pid_list_length;index++)
		{
			if(p_bitrate_param->pid_list[index] != engine->m_bitrate_param.pid_list[index])
				break;
		}

		if(index == engine->m_bitrate_param.pid_list_length)
			param_same = 1;
	}

	memcpy(&(engine->m_bitrate_param), p_bitrate_param, sizeof(AliDmxBitrateParam_S));

	printk(KERN_NOTICE "bitrate length %d\n", engine->m_bitrate_param.pid_list_length);

	printk(KERN_NOTICE "pids:\n");

	for(index = 0;index < engine->m_bitrate_param.pid_list_length;index++)
	{
		printk(KERN_NOTICE "%d:%d\n", index, engine->m_bitrate_param.pid_list[index]);
	}				

	/*reset the parameter when change bitrate parameter*/
	if(p_bitrate_param->pid_list_length > 0)
	{
		engine->bitrate_detect = 1;

		if(!param_same)
		{
			engine->pkts_num_bitrate_pidlist = 0;	
			engine->last_pkts_num_bitrate_pidlist = 0;

			do_gettimeofday(&engine->last_time);
			
			engine->last_rate[0] = engine->last_rate[1] = engine->last_rate[2] = engine->last_rate[3] = 0;
			engine->last_rate[4] = engine->last_rate[5] = engine->last_rate[6] = engine->last_rate[7] = 0;

			engine->bitrate = 0;
		}		
	}
	else
	{
		engine->bitrate_detect = 0;
	}
	
	return 0;
}


__u32 dmx_data_engine_bitrate_valid_count(__u32 src_hw_interface_id)
{
	__s32 i = 0;
	__u32 valid_count = 0;
	
	struct dmx_data_engine_output *engine = NULL;
	
	if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
	{
		printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
		return(DMX_DATA_ENGINE_BITRATE_ERROR);
	}
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	
	for(i = 0; i < 8; i++)
	{
		if(engine->last_rate[i] != 0)
			valid_count++;
	}

	DMX_PRINTK(KERN_NOTICE "%s,%d,%d\n", __FUNCTION__, __LINE__, valid_count);

	return valid_count;
}


__u32 dmx_data_engine_get_bitrate(__u32 src_hw_interface_id)
{
	__u32 bitrate = 0;
	//__s32 time_out = 24;

	struct dmx_data_engine_output *engine = NULL;
	
	if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
	{
		printk(KERN_ERR "%s,%d\n", __FUNCTION__, __LINE__);
		return(DMX_DATA_ENGINE_BITRATE_ERROR);
	}
	
	engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	if (engine->bitrate_detect == 1) 
	{
		/* make sure the first time can get the bitrate, max 1s */
		/*
		while (dmx_data_engine_bitrate_valid_count(src_hw_interface_id) <= 6)
		{
			msleep(20);
			if (--time_out == 0)
			{
				break;
			}
		}
		*/
		bitrate = engine->bitrate;

		/*
		printk(KERN_NOTICE "%s,%d,bitrate:%u %u %u %u %u %u %u %u \n", __FUNCTION__, __LINE__,
							engine->last_rate[0],
							engine->last_rate[1],
							engine->last_rate[2],
							engine->last_rate[3],
							engine->last_rate[4],
							engine->last_rate[5],
							engine->last_rate[6],
							engine->last_rate[7]);	
		*/
	}	
	
	return bitrate;
}

/*****************************************************************************/
/**
*
* Retrieve TS packets from demux hw buffer, dispatch them to upper layer.
*
* @param
* - param1: Not used.
*
* - param2: Not used
*
* @note
*
* @return
* - NONE
*
******************************************************************************/
__s32 dmx_data_engine_output_task_from_kern
(
    void *param
)
{
    __s32                          ret;
    __u32                          hw_buf_rd;
    __u32                          hw_buf_wr;
    __u32                          hw_buf_rd_real;
    __u32                          hw_buf_wr_real;	
    __u32                          next_hw_buf_wr;
    __u32                          hw_buf_end_idx;
    __u32                          hw_buf_start_addr;
    __u8                          *ts_addr;
    struct dmx_data_engine_output *engine;

    engine = (struct dmx_data_engine_output *)param;

    hw_buf_start_addr = dmx_hw_buf_start_addr_get(engine->src_hw_interface_id);
    
    hw_buf_end_idx = dmx_hw_buf_end_get(engine->src_hw_interface_id);

	engine->hw_pkt_buf_cnt = hw_buf_end_idx + 1;

    for(;;)
    {
        /* Check dmx buffer every DMX_DATA_ENG_OUTPUT_INTERVAL miliseconds.
         * This should be enough to meet all time reqirments.
         */
        msleep(DMX_DATA_ENG_OUTPUT_INTERVAL);

        ret = dmx_mutex_output_lock(engine->src_hw_interface_id);

        if (0 != ret)
        {
            printk("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
        
            return(ret);
        }

#ifdef CONFIG_DVB_ALI_M36_DMX_CACHE
		if(i_dmx_data_engine_cache_retracewait(engine->src_hw_interface_id) == 0)
		{
			dmx_mutex_output_unlock(engine->src_hw_interface_id);
			continue;
		}

		i_dmx_data_engine_cache_adjust_bylevel(engine->src_hw_interface_id);

		i_dmx_data_engine_cache_valid(engine->src_hw_interface_id);

		i_dmx_data_engine_cache_retracevalid(engine->src_hw_interface_id);

		i_dmx_data_engine_cache_retracefinish(engine->src_hw_interface_id);
#endif

        /* Loop to process TS packets, copy the TS packet payload to 
         * upper layer buffers.
         */
        hw_buf_rd = i_dmx_data_engine_cache_rd_get(engine->src_hw_interface_id);

        hw_buf_wr = i_dmx_data_engine_cache_wr_get(engine->src_hw_interface_id);

#ifdef CONFIG_DVB_ALI_M36_DMX_CACHE
		hw_buf_rd_real = dmx_hw_buf_rd_get(engine->src_hw_interface_id);
		
		hw_buf_wr_real = dmx_hw_buf_wr_get(engine->src_hw_interface_id);
#else
		hw_buf_rd_real = hw_buf_rd;
		hw_buf_wr_real = hw_buf_wr;
#endif
		dmx_mutex_output_unlock(engine->src_hw_interface_id);

        /* Check if buffer is empty.
        */
        if (hw_buf_rd_real == hw_buf_wr_real)
        {
            g_stat_info.DmxBufEmptyCnt++;

            continue;
        }

        if (DMX_DATA_ENGINE_SRC_REAL_HW == engine->src_type)
        {
            /* Check if dmx hw buffer is overflow, clean hw buffer if it is.
            */
            next_hw_buf_wr = hw_buf_wr_real + 1;
        
            if (next_hw_buf_wr > hw_buf_end_idx)
            {
                next_hw_buf_wr = 0;
            }
            
            if (hw_buf_rd_real == next_hw_buf_wr)
            {
                g_stat_info.OverlapCnt++;
                DMX_PRINTK("hw dmx:%d, hw buf overflow,rd:%d,wr:%d.\n",
							engine->src_hw_interface_id, hw_buf_rd_real, hw_buf_wr_real);
#ifdef CONFIG_DVB_ALI_M36_DMX_CACHE
				dmx_hw_buf_rd_set(engine->src_hw_interface_id, hw_buf_wr_real);
#else		
				dmx_mutex_output_lock(engine->src_hw_interface_id);

				if(dmx_data_engine_cache_typeget(engine->src_hw_interface_id) == DMX_NO_CACHE)
				{
					dmx_hw_buf_rd_set(engine->src_hw_interface_id, hw_buf_wr_real);
				}
				else
				{
					i_dmx_data_engine_cache_adjust_bylevel(engine->src_hw_interface_id);
				}

				dmx_mutex_output_unlock(engine->src_hw_interface_id);
#endif		
                continue;
            }


#ifdef CONFIG_ARM
#else
#ifdef DMX_USE_CACHE_BUF_ADDR

			if(engine->src_hw_interface_id != ALI_SEETOMAIN_BUF_HWIF_ID)
			{
#ifdef M_DEMUX_FULSH_ALL
                #ifndef CONFIG_DVB_ALI_M36_DMX_CACHE
                __CACHE_LINEALIGN_INV_ALI(hw_buf_start_addr, engine->hw_pkt_buf_cnt * 188);
                #endif
#else
                dmx_hw_buf_invalidate(hw_buf_start_addr, hw_buf_rd_real, hw_buf_wr_real,
                                      engine->hw_pkt_buf_cnt);
#endif
			}
#endif
#endif
        }

		/*
		if (hw_buf_rd == hw_buf_wr)
			printk("r w:%d %d\n", hw_buf_rd, hw_buf_wr);
		*/	

        ts_addr = (__u8 *)(hw_buf_start_addr + (hw_buf_rd * 188));

        /* Lock entire DMX until all the data contained in HW buffer has been
         * parsed for safty reason.
         */
        ret = dmx_mutex_output_lock(engine->src_hw_interface_id);

        if (0 != ret)
        {
            printk("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
        
            return(ret);
        }  	

        /* Parse TS packets from EXTERNAL TO MAIN buffer.
        */		
        while(hw_buf_rd != hw_buf_wr)
        {
#ifdef CONFIG_DVB_ALI_M36_DMX_CACHE        
			if((i_dmx_data_engine_cache_checkvalid(engine->src_hw_interface_id) == 0)
				|| (i_dmx_data_engine_cache_checkretrace(engine->src_hw_interface_id) == 0))
			{
				break;
			}
            #ifdef CONFIG_ARM
            #else
                #ifdef CONFIG_DVB_ALI_M36_DMX_CACHE
            __CACHE_LINEALIGN_INV_ALI((__u32)ts_addr, 188);
                #endif
            #endif

			if(i_dmx_data_engine_cache_checkpid(engine->src_hw_interface_id, ((ts_addr[1] & 0x1F) << 8) | ts_addr[2]) == 0)
#endif				
			{
	            g_stat_info.TotalTsInCnt++;

	            ret = dmx_pid_flt_parse(engine->src_hw_interface_id, ts_addr);

	            /* Caution: 
	             * No ts packet handling proccess is allowed to return
	             * DMX_ERR_BUF_BUSY except dmx_see_buf_wr_ts(), because in PVR case,
	             * SEE may need to hold dmx by no longer retrieving data from 
	             * main2see buffer.
	             */
	            if (DMX_ERR_BUF_BUSY == ret)
	            {
	                //printk("%s,%d,hw dmx:%d,rd:%d,wr:%d.\n",__FUNCTION__, __LINE__,
	                       //engine->src_hw_interface_id, hw_buf_rd, hw_buf_wr);
					//retrace_overflow_count++;
	                g_stat_info.PlayBusyCnt++;

	                break;
	            }
	            else if (DMX_ERR_OK != ret)
	            {
	                g_stat_info.NobodyCareCnt++;
	            }

	            /* Statistics.
	            */
	            engine->ts_in_cnt++;
			}

			i_dmx_data_engine_bitrate_checkpid(engine->src_hw_interface_id, ((ts_addr[1] & 0x1F) << 8) | ts_addr[2]);
			
            hw_buf_rd++;

            if (hw_buf_rd > hw_buf_end_idx)
            {
                hw_buf_rd = 0;

                ts_addr = (__u8 *)(hw_buf_start_addr);
            }
            else
            {
                ts_addr += 188;
            }

            /* Move hardware rd pointer. 
             */
            i_dmx_data_engine_cache_rd_set(engine->src_hw_interface_id, hw_buf_rd);
        }

		i_dmx_data_engine_bitrate_calc(engine->src_hw_interface_id);
        
        dmx_mutex_output_unlock(engine->src_hw_interface_id);
    }

    return(-__LINE__);
}





/*****************************************************************************/
/**
*
* Retrieve TS packets from demux hw buffer, dispatch them to upper layer.
*
* @param
* - param1: Not used.
*
* - param2: Not used
*
* @note
*
* @return
* - NONE
*
******************************************************************************/
__s32 dmx_data_engine_locate_ts_pkt
(
    struct dmx_data_engine_output *engine
)
{
    __s32  round;
    __u32  chk_pkt_len;
    __u8  *rd_addr;
    __u8  *wr_addr;
    __u8  *buf_start_addr;
    __u8  *buf_end_addr;
    __u8  *sync_find_ptr_1;
    __u8  *sync_find_ptr_2; 
    __u8  *sync_find_ptr_3;
    __u32  rd_idx;
    __u32  wr_idx;
    __u32  buf_end_idx; 
	__u32 data_size;
	
    /* If pakcet already synced, do nothing.
    */
    if (0 != engine->pkt_synced)
    {
        return(0);
    }

    rd_idx = dmx_hw_buf_rd_get(engine->src_hw_interface_id);

    wr_idx = dmx_hw_buf_wr_get(engine->src_hw_interface_id);

    /* If buffer is empty, do nothing.
    */
    if (rd_idx == wr_idx)
    {
        return(0);
    }

    sync_find_ptr_1 = NULL;
    sync_find_ptr_2 = NULL; 
    sync_find_ptr_3 = NULL;

    buf_start_addr = (__u8 *)dmx_hw_buf_start_addr_get(engine->src_hw_interface_id);

    rd_addr = buf_start_addr + rd_idx;

    wr_addr = buf_start_addr + wr_idx;  

    buf_end_idx = dmx_hw_buf_end_get(engine->src_hw_interface_id);
    
    buf_end_addr = buf_start_addr + buf_end_idx;

    sync_find_ptr_1 = rd_addr;

	if(wr_idx > rd_idx)
	{
		data_size = wr_idx - rd_idx;
	}
	else
	{
		data_size = buf_end_idx - rd_idx + wr_idx;
	}

	/*data is not enough*/
	if(data_size < (204 * 3))
    {
        return(0);
    }		

    for (round = 0; round < 2; round++)
    {
        if (0 == round)
        {
            printk("%s,%d\n", __FUNCTION__, __LINE__);
            
            chk_pkt_len = 188;
        }
        else if (1 == round)
        {
            printk("%s,%d\n", __FUNCTION__, __LINE__);
            
            chk_pkt_len = 204;
        }
        else
        {
            chk_pkt_len = 0;
            
            printk("%s,%d\n", __FUNCTION__, __LINE__);
			
            return(0);
        }

        sync_find_ptr_1 = rd_addr;

        if (rd_addr > wr_addr)
        {
            for (;;)
            {
                sync_find_ptr_2 = sync_find_ptr_1 + chk_pkt_len;
                
                if (sync_find_ptr_2 >= buf_end_addr)
                {
                    sync_find_ptr_2 = sync_find_ptr_2 - buf_end_addr + buf_start_addr;             
                }

				if((sync_find_ptr_1 >= buf_start_addr) && (sync_find_ptr_1 < wr_addr))
				{
                    if (sync_find_ptr_2 >= wr_addr)
                    {
                        printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x\n",
                                __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                                (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr);
                        break;
                    }					
				}

                sync_find_ptr_3 = sync_find_ptr_2 + chk_pkt_len;
    
                if (sync_find_ptr_3 >= buf_end_addr)
                {
                    sync_find_ptr_3 = sync_find_ptr_3 - buf_end_addr + buf_start_addr;                 
                }

				if((sync_find_ptr_1 >= buf_start_addr) && (sync_find_ptr_1 < wr_addr))
				{
                    if (sync_find_ptr_3 >= wr_addr)
                    {
                        printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x\n",
                                __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                                (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr);
        
                        break;
                    }					
				}
	
                if ((0x47 == *sync_find_ptr_1) && (0x47 == *sync_find_ptr_2) &&
                    (0x47 == *sync_find_ptr_3))
                {
                    printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x,chk_pkt_len:%d\n",
                            __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                            (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr, chk_pkt_len);
    
                    /* Drop thrash data before the first found TS packet.
                    */
                    dmx_hw_buf_rd_set(engine->src_hw_interface_id, (__u32)(sync_find_ptr_1 - buf_start_addr));
    
                    /* Now we are in sync.
                    */
                    engine->pkt_synced = 1;
    
                    engine->pkt_len = chk_pkt_len;
                    
                    return(1);
                }       

                sync_find_ptr_1++;
            
                if (sync_find_ptr_1 >= buf_end_addr)
                {
                    sync_find_ptr_1 = buf_start_addr;
                }

            }
        }
        else
        {
            for (;;)
            {
                sync_find_ptr_2 = sync_find_ptr_1 + chk_pkt_len;
    
                if (sync_find_ptr_2 >= wr_addr)
                {
                    printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x\n",
                            __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                            (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr);
                    break;
                }               

                sync_find_ptr_3 = sync_find_ptr_2 + chk_pkt_len;
    
                if (sync_find_ptr_3 >= wr_addr)
                {
                    printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x\n",
                            __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                            (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr);
    
                    break;
                }                   
    
                if ((0x47 == *sync_find_ptr_1) && (0x47 == *sync_find_ptr_2) &&
                    (0x47 == *sync_find_ptr_3))
                {
                    printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x,chk_pkt_len:%d\n",
                            __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
                            (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr, chk_pkt_len);
    
                    /* Drop thrash data before the first found TS packet.
                    */
                    dmx_hw_buf_rd_set(engine->src_hw_interface_id, (__u32)(sync_find_ptr_1 - buf_start_addr));
    
                    /* Now we are in sync.
                    */
                    engine->pkt_synced = 1;
    
                    engine->pkt_len = chk_pkt_len;
                    
                    return(1);
                }       

                sync_find_ptr_1++;
            
                if (sync_find_ptr_1 >= buf_end_addr)
                {
                    sync_find_ptr_1 = buf_start_addr;
                } 
			
            }
        }
    }

    printk("%s,%d,sync_find_ptr_1:%x,sync_find_ptr_2:%x,sync_find_ptr_3:%x,rd_addr:%x,wr_addr:%x,chk_pkt_len:%d\n",
            __FUNCTION__, __LINE__, (__u32)sync_find_ptr_1, (__u32)sync_find_ptr_2,
            (__u32)sync_find_ptr_3, (__u32)rd_addr, (__u32)wr_addr, chk_pkt_len);

    /* Drop thrash data if no packet found.
    */
    dmx_hw_buf_rd_set(engine->src_hw_interface_id, (__u32)(sync_find_ptr_1 - buf_start_addr));

    return(0);
}









__s32 dmx_data_engine_send_ts_pkt
(
    struct dmx_data_engine_output *engine,
    __u8 *pkt_addr
)
{
    __s32 ret;
    
    /* 0xAA is used for ALI PVR, nasty.
    */
    if ((0x47 != pkt_addr[0]) && (0xAA != pkt_addr[0]))
    {
        engine->pkt_synced = 0;

        printk("%s,%d,rd_addr:%x,rd_addr[0]:%02x %02x %02x %02x %02x\n",
               __FUNCTION__, __LINE__, (__u32)pkt_addr, pkt_addr[0],
               pkt_addr[1], pkt_addr[2], pkt_addr[3], pkt_addr[4]);
        
        return(DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT);
    }
    
    ret = dmx_pid_flt_parse(engine->src_hw_interface_id, pkt_addr);

    return(ret);
}





__s32 dmx_data_engine_output_task_from_usr
(
    void *param
)
{
    __s32                          ret;
    __u8                          *rd_addr;
    __u8                          *wr_addr;
    __u8                          *end_addr;    
    __u8                          *next_rd_addr;
    __u8                          *buf_start_addr;
    __u32                          rd_idx;
    __u32                          wr_idx;
    __u32                          end_idx;
    struct dmx_data_engine_output *engine;
    __u32                          data_len;
    __u32                          runback_need_len;

    engine = (struct dmx_data_engine_output *)param;

    buf_start_addr = (__u8 *)dmx_hw_buf_start_addr_get(engine->src_hw_interface_id);

    end_idx = dmx_hw_buf_end_get(engine->src_hw_interface_id);

    end_addr = buf_start_addr + end_idx;

    //printk("%s,%d,buf_start_addr:%x,end_addr:%x\n",
           //__FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr);  	
	
    for(;;)
    {
        while (0 == engine->pkt_synced)
        {
            //printk("%s,%d\n", __FUNCTION__, __LINE__);
            
            msleep_interruptible(DMX_DATA_ENG_OUTPUT_INTERVAL);

			ret = dmx_mutex_output_lock(engine->src_hw_interface_id);
			
			if (0 != ret)
			{
				printk("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
			
				continue;
			}  
						
            dmx_data_engine_locate_ts_pkt(engine);

			dmx_mutex_output_unlock(engine->src_hw_interface_id);
        }

	    //printk("%s,%d\n", __FUNCTION__, __LINE__);

        /* Lock entire DMX until all the data contained in HW buffer has been
         * parsed for safe.
         */
        ret = dmx_mutex_output_lock(engine->src_hw_interface_id);
		
        if (0 != ret)
        {
            //printk("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
        
            return(ret);
        }  

        rd_idx = dmx_hw_buf_rd_get(engine->src_hw_interface_id);
        
        wr_idx = dmx_hw_buf_wr_get(engine->src_hw_interface_id);

        rd_addr = buf_start_addr + rd_idx;

        wr_addr = buf_start_addr + wr_idx;

        //printk("%s,%d\n", __FUNCTION__, __LINE__);

        if(rd_addr == wr_addr)
        {           
            goto NEXT_LOOP;
        }

        /* rd ==> end then rd = 0
        */
        if (rd_addr > wr_addr)
        {  
            /* Process run-back pakcet which occupies the end and start of ring
             * buffer.
             */
            if(engine->runback_pkt_buf_wr > 0)
            {
                data_len = wr_addr - end_addr;
                
                runback_need_len = engine->pkt_len - engine->runback_pkt_buf_wr;
                
                if (data_len < runback_need_len)
                {
                    memcpy(engine->runback_pkt_buf + engine->runback_pkt_buf_wr,
                           rd_addr, data_len);
                
                    engine->runback_pkt_buf_wr += data_len;
                
                    rd_addr = buf_start_addr;
                
                    goto NEXT_LOOP;
                }
                /* Send runback pakcet.
                */
                else
                {
                    memcpy(engine->runback_pkt_buf + engine->runback_pkt_buf_wr,
                           rd_addr, runback_need_len);
                    
                    ret = dmx_data_engine_send_ts_pkt(engine, 
                                                      engine->runback_pkt_buf);

                    if (DMX_ERR_BUF_BUSY == ret)
                    {
                        goto NEXT_LOOP;
                    }
                    
                    /* If the TS pakcet in runback buffer is bad, drop this packet.
                     * Doing this may loss at most 188 bytes which should not be dropped, but
                     * the code may be greatly simplified.
                     * In the case of bad TS pakcet, dropping 188 bytes should be affordable.
                    */
                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);  
                    }                   
                
                    engine->runback_pkt_buf_wr = 0;
                    
                    rd_addr += runback_need_len;
                }
            }

            /* Process normal pakcet.
             */
            next_rd_addr = rd_addr + engine->pkt_len;

            for (;;)
            {
                /* send pakcet in the ring buffer.
                 */
                if (next_rd_addr < end_addr)
                {
                    ret = dmx_data_engine_send_ts_pkt(engine, rd_addr);

                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);  
                    }                   

                    if ((DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret) || 
                        (DMX_ERR_BUF_BUSY == ret))
                    {
                        goto NEXT_LOOP;
                    }
                }
                /* pakcet just complete at boundary, send all pakcet and wait
                 * for next rund.
                 */ 
                else if (next_rd_addr == end_addr)
                {
                    ret = dmx_data_engine_send_ts_pkt(engine, rd_addr);

                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);  
                    }
                    
                    if ((DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret) || 
                        (DMX_ERR_BUF_BUSY == ret))
                    {
                        goto NEXT_LOOP;
                    }

                    break;
                }
                /* we have a run-back pakcet which occupies the end and the 
                 * start of the ring buffer, copy it a temporay buffer for 
                 * later sending it.
                 * At most one TS packet in runback_pkt_buf.
                */
                else 
                {
                    engine->runback_pkt_buf_wr = end_addr - rd_addr;
                    
                    memcpy(engine->runback_pkt_buf, rd_addr, 
                           engine->runback_pkt_buf_wr);

                    break;
                }
                
                rd_addr = next_rd_addr;

                next_rd_addr += engine->pkt_len;
            }
            
            rd_addr = buf_start_addr;
        }

        /* rd ==> wr
        */
        if (rd_addr < wr_addr)
        {
            /* Process run-back pakcet which occupies the end and start of ring
             * buffer.
             */
            if(engine->runback_pkt_buf_wr > 0)
            {
                data_len = wr_addr - rd_addr;
                
                runback_need_len = engine->pkt_len - engine->runback_pkt_buf_wr;
                
                if (data_len < runback_need_len)
                {
                    memcpy(engine->runback_pkt_buf + engine->runback_pkt_buf_wr,
                           rd_addr, data_len);
                
                    engine->runback_pkt_buf_wr += data_len;
                
                    rd_addr = wr_addr;
                
                    goto NEXT_LOOP;
                }
                /* Send runback pakcet.
                */
                else
                {
                    memcpy(engine->runback_pkt_buf + engine->runback_pkt_buf_wr,
                           rd_addr, runback_need_len);
                    
                    ret = dmx_data_engine_send_ts_pkt(engine, 
                                                      engine->runback_pkt_buf);

                    if (DMX_ERR_BUF_BUSY == ret)
                    {
                        goto NEXT_LOOP;
                    }
                    
                    /* If the TS pakcet in runback buffer is bad, drop this packet.
                     * Doing this may loss at most 188 bytes which should not be dropped, but
                     * the code may be greatly simplified.
                     * In the case of bad TS pakcet, dropping 188 bytes should be affordable.
                    */
                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);   
                    }

                    engine->runback_pkt_buf_wr = 0;
                    
                    rd_addr += runback_need_len;
                }
            }

            /* Process normal pakcet.
             */     
            next_rd_addr = rd_addr + engine->pkt_len;

            for (;;)
            {
                /* send pakcet in the ring buffer.
                */
                if (next_rd_addr < wr_addr)
                {
                    ret = dmx_data_engine_send_ts_pkt(engine, rd_addr);

                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);  
                    }
                    
                    if ((DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret) || 
                        (DMX_ERR_BUF_BUSY == ret))
                    {
                        goto NEXT_LOOP;
                    }

                    //printk("%s,%d,rd_addr:%x\n", __FUNCTION__, __LINE__, rd_addr);
                }
                /* pakcet just complete at boundary, send all pakcet and wait
                 * for next rund.
                 */             
                else if (next_rd_addr == wr_addr)
                {
                    ret = dmx_data_engine_send_ts_pkt(engine, rd_addr);

                    if (DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret)
                    {
                        printk("%s,%d,buf_start_addr:%x,end_addr:%x,rd_addr:%x,wr_addr:%x\n",
                               __FUNCTION__, __LINE__, (__u32)buf_start_addr, (__u32)end_addr,
                               (__u32)rd_addr, (__u32)wr_addr);  
                    }
                    
                    if ((DMX_ERR_DATA_ENGINE_NEED_RESYNC_PKT == ret) || 
                        (DMX_ERR_BUF_BUSY == ret))
                    {
                        goto NEXT_LOOP;
                    }

                    rd_addr = wr_addr;
                    
                    //printk("%s,%d,rd_addr:%x\n", __FUNCTION__, __LINE__, rd_addr);

                    break;
                }
                /* Pakcet not completely written by user, wait for next round
                 * for pakcet to be completely written by user.
                 */
                else 
                {                   
                    break;
                }
                
                rd_addr = next_rd_addr;

                next_rd_addr += engine->pkt_len;
            }
        }

NEXT_LOOP:
    
        //printk("%s,%d,(__u32)(new_rd_addr - buf_start_addr):%x\n", __FUNCTION__, __LINE__,(__u32)(new_rd_addr - buf_start_addr));

        dmx_hw_buf_rd_set(engine->src_hw_interface_id, (__u32)(rd_addr - buf_start_addr));

        dmx_mutex_output_unlock(engine->src_hw_interface_id);

        //printk("%s,%d\n", __FUNCTION__, __LINE__);

        /* Check dmx buffer every DMX_DATA_ENG_OUTPUT_INTERVAL miliseconds.
         * DMX_DATA_ENG_OUTPUT_INTERVAL should be enough to meet all time
         * reqirments.
         */
        msleep_interruptible(DMX_DATA_ENG_OUTPUT_INTERVAL);  
    }

    return(-__LINE__);
}



__u32 dmx_data_engine_output_ts_in_cnt_get
(
    __u32 src_hw_interface_id
)
{
    return(ali_dmx_data_engine_module.engine_output[src_hw_interface_id].ts_in_cnt);
}


static __s32 dmx_data_engine_thread_run
(
    __s32 (*engine_thread)(void *engine_thread_param),    
    void *engine_thread_param,    
    void *engine_thread_name
)
{
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};

	struct task_struct *p;

	p = kthread_create(engine_thread, engine_thread_param, engine_thread_name);

	if (IS_ERR(p))
	{
		return(PTR_ERR(p));	
	}

	sched_setscheduler(p, SCHED_RR, &param);

	wake_up_process(p);
	
	return(0);
}


__s32 dmx_data_engine_module_init_kern
(
    __u32                     src_hw_interface_id,
    __u8                     *engine_name,
    enum DMX_DATA_ENGINE_SRC  src_type
)
{
    struct dmx_data_engine_output *engine;
    int i = 0;
	
    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
        return(DMX_ERR_DATA_ENGINE_INIT_FAIL);
    }
    
    engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];
    
    if (engine->state != DMX_DATA_ENGINE_TASK_HW_STATE_IDLE)
    {
        return(DMX_ERR_DATA_ENGINE_INIT_FAIL);
    }

    engine->src_type = src_type;

    engine->pkt_synced = 0;

    engine->pkt_len = 0;

    engine->src_hw_interface_id = src_hw_interface_id;

    engine->state = DMX_DATA_ENGINE_TASK_HW_STATE_RUN;

	/* demux cache member init */
	engine->hw_pkt_buf_cnt = 0;
	
	engine->cache_param.type = DMX_NO_CACHE;
	engine->cache_param.pid_list_len = 0;
	for (i = 0; i < DMX_CACHE_PID_LIST_MAX_LEN; i++)
	{
		engine->cache_param.pid_list[i] = 0x1fff;
	}
	engine->cache_status = DMX_CACHE_UNINIT;
	engine->cache_tp_flt_id = -1;
	for (i = 0; i < DMX_CACHE_PID_LIST_MAX_LEN; i++)
	{
		engine->cache_pid_flt_id[i] = -1;
	}

	engine->cache_hwbuf_flush = false;
	
	engine->cache_retrace_param.pid_list_len = 0;
	for (i = 0; i < DMX_CACHE_PID_LIST_MAX_LEN; i++)
	{
		engine->cache_retrace_param.pid_list[i] = 0x1fff;
	}	
	engine->cache_retrace_status = DMX_RETRACE_STOP;

	engine->buf_swrd = 0;
	engine->buf_retrace_swrd = 0;

	/* demux bitrate */
	engine->m_bitrate_param.pid_list_length = 0;
	for(i = 0; i < DMX_BITRATE_PID_LIST_MAX_LEN; i++)
	{
		engine->m_bitrate_param.pid_list[i] = 0x1fff;
	}
	
	engine->bitrate_detect = 0;
	
	engine->pkts_num_bitrate_pidlist = 0;
	engine->last_pkts_num_bitrate_pidlist = 0;

	engine->last_time.tv_sec = 0;
	engine->last_time.tv_usec = 0;

	engine->last_rate[0] = engine->last_rate[1] = engine->last_rate[2] = engine->last_rate[3] = 0;
	engine->last_rate[4] = engine->last_rate[5] = engine->last_rate[6] = engine->last_rate[7] = 0;	

	engine->bitrate = 0;

    dmx_data_engine_thread_run(dmx_data_engine_output_task_from_kern, engine, engine_name);

    return(0);
}


__s32 dmx_data_engine_module_init_usr
(
    __u32                     src_hw_interface_id,
    __u8                     *engine_name,
    enum DMX_DATA_ENGINE_SRC  src_type
)
{
    struct dmx_data_engine_output *engine;
    
    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
        return(DMX_ERR_DATA_ENGINE_INIT_FAIL);
    }
    
    engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];
    
    if (engine->state != DMX_DATA_ENGINE_TASK_HW_STATE_IDLE)
    {
        return(DMX_ERR_DATA_ENGINE_INIT_FAIL);
    }

    engine->src_type = src_type;

    engine->pkt_synced = 0;

    engine->pkt_len = 0;

    engine->src_hw_interface_id = src_hw_interface_id;

    engine->runback_pkt_buf_len = DMX_DATA_ENG_RUNBACK_BUF_LEN;

    engine->runback_pkt_buf_wr = 0;

    engine->runback_pkt_buf = vmalloc(engine->runback_pkt_buf_len);

    if (NULL == engine->runback_pkt_buf)
    {
        panic("%s,%d\n", __FUNCTION__, __LINE__);
    }

    engine->state = DMX_DATA_ENGINE_TASK_HW_STATE_RUN;

    dmx_data_engine_thread_run(dmx_data_engine_output_task_from_usr, engine, engine_name);

    return(0);
}


enum DMX_DATA_ENGINE_TASK_HW_STATE dmx_data_engine_module_get_state(__u32 src_hw_interface_id)
{
    struct dmx_data_engine_output *engine;
    
    if (src_hw_interface_id > DMX_LINUX_OUTPUT_DEV_CNT)
    {
        return(DMX_ERR_DATA_ENGINE_INIT_FAIL);
    }
    
    engine = &ali_dmx_data_engine_module.engine_output[src_hw_interface_id];

	return(engine->state);
}


