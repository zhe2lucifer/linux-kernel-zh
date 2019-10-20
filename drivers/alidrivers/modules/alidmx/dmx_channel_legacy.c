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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/sched.h>
#include <linux/sched/rt.h>
#else
#include <linux/sched.h>
#endif
#include <linux/kthread.h>
#include <asm/io.h>
//#include <ali_interrupt.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sched.h> 
#include <linux/mm.h>
#include <linux/time.h>

//#include <ali_dmx_common.h>

#include <linux/slab.h>

#include "dmx_stack.h"
#include "dmx_see_interface.h"
#include "dmx_dbg.h"

#include <linux/ali_dsc.h>
#include <linux/ali_pvr.h>
#ifdef CONFIG_ALI_SEC
#include <linux/ali_sec.h>
#endif
#include <ali_cache.h>

#if 1

#if 0
#define DMX_CH_LEGA_DBG printk
#else
#define DMX_CH_LEGA_DBG(...)
#endif


struct dmx_channel_module_legacy ali_dmx_channel_module_legacy;

extern struct dmx_ts_flt_module ali_dmx_ts_flt_module;
extern struct dmx_pcr_flt_module ali_dmx_pcr_flt_module;
extern struct dmx_sec_flt_module ali_dmx_sec_flt_module;
extern struct dmx_pes_flt_module ali_dmx_pes_flt_module;
extern struct dmx_see_device ali_dmx_see_dev[2];
extern struct Ali_DmxKernGlobalStatInfo g_stat_info;


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

__s32 dmx_channel_get_pkt_len
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                     ret;
    __u32                     len;
    struct dmx_channel_param *usr_para;

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    /* ch startus validation, return success in all state.
    */
    if (ch->state == DMX_CHANNEL_STATE_IDLE)
    {
        return(0);
    }

    ret = 0;

    usr_para = &ch->usr_param;

    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            len = dmx_data_buf_first_pkt_len(&ch->data_buf);
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
        {
            len = dmx_data_buf_first_pkt_len(&ch->data_buf);
        }
        break;        

        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            len = dmx_data_buf_total_len(&ch->data_buf);
        }
        break;

        default:
        {
            ret = -EINVAL;
        }
        break;
    }

    if (0 == ret)
    {
        if (len > 0)
        {
            //DMX_CH_LEGA_DBG("len 2:%d\n", len);
    
            ret = copy_to_user((void __user *)arg, &len, _IOC_SIZE(cmd));
        
            if (0 != ret)
            {
                ret = -ENOTTY;
            }
        }
        else
        {
            ret = -EAGAIN;
        }
    }

    return(ret);
}



__s32 dmx_bitrate_get
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)

{
    __s32 ret;
    __u32 bitrate;
	unsigned int cpy_flag;

	if(dmx_data_engine_bitrate_valid_count(dev->src_hw_interface_id) <= 6)
	{
		bitrate = 0;
		
		cpy_flag = copy_to_user((void __user *)arg, &bitrate, _IOC_SIZE(cmd));
		
		ret = -EBUSY;
	}
	else
	{
		/* Temparary, need to be implemented by upper layer.
		*/
	    bitrate = dmx_data_engine_get_bitrate(dev->src_hw_interface_id);

		//printk(KERN_NOTICE "%s,%d,bitrate:%u\n", __FUNCTION__, __LINE__, bitrate);			

	    ret = copy_to_user((void __user *)arg, &bitrate, _IOC_SIZE(cmd));

	    if (0 != ret)
	    {
	        ret = -ENOTTY;
	    }   
	}

    return(ret);
}





__s32 dmx_hw_buf_free_len
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __u32 pkt_cnt;
    __s32 ret;
    __u32 rd_idx;
    __u32 wr_idx;
    __u32 end_idx;

    ret = 0;

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    /* ch startus validation, return success in all state.
    */
    if (ch->state == DMX_CHANNEL_STATE_IDLE)
    {
        return(0);
    }

    wr_idx = dmx_hw_buf_wr_get(dev->src_hw_interface_id);

    rd_idx = dmx_hw_buf_rd_get(dev->src_hw_interface_id);

    if (wr_idx >= rd_idx)
    {
        end_idx = dmx_hw_buf_end_get(dev->src_hw_interface_id);
        
        pkt_cnt = end_idx - wr_idx + rd_idx - 1;
    }
    else
    {
        pkt_cnt = rd_idx - wr_idx - 1;
    }

    ret = copy_to_user((void __user *)arg, &pkt_cnt, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        ret = -ENOTTY;
    }    
    
    DMX_CH_LEGA_DBG("%s,%d,pkt_cnt:%d\n", __FUNCTION__, __LINE__, pkt_cnt);

    return(ret);
}


/* Note: Called in inerrupt context, so this function may not sleep.
*/
__s32 dmx_channel_pcr_wr
(
    __u32 pcr,
    __u32 param
)
{
    dmx_see_set_pcr((struct dmx_see_device *)param, pcr);

    return(0);
}


__s32 dmx_channel_kern_glb_cfg
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch
)
{
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* stream state validation. 
     */
    if ((DMX_CHANNEL_STATE_CFG != ch->state) &&
        (DMX_CHANNEL_STATE_STOP != ch->state))
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

	memset(&g_stat_info, 0, sizeof(struct Ali_DmxKernGlobalStatInfo));

    return(0);
}



__s32 dmx_channel_kern_glb_start
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{    
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* channel state validation. 
     */
    if (ch->state != DMX_CHANNEL_STATE_STOP)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_RUN;

    return(0);
}



__s32 dmx_channel_kern_glb_stop
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    return(0);
}


__s32 dmx_channel_kern_glb_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32 ret;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s(), L[%d] \n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

#if 0	
    if (stream->type != DMX_STREAM_TYPE_KERN_GLB)
    {
        return(-EPERM);
    }
#endif


    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
    	DMX_API_DBG("%s(), L[%d], state[%d]\n", __FUNCTION__, __LINE__, ch->state);
        
        return(-EPERM);
    }

	ret = copy_to_user((void __user *)arg, &g_stat_info, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    
    return(0);
}


__s32 dmx_channel_kern_glb_realtime_set
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* stream state validation. 
     */
    if (DMX_CHANNEL_STATE_IDLE == ch->state)
    {
        return(-EPERM);
    }

	g_stat_info.RealTimePrintEn = arg;

    return(0);
}


__s32 dmx_channel_see_glb_cfg
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
	//volatile struct Ali_DmxSeeGlobalStatInfo *p_StatInfo;

	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* stream state validation. 
     */
    if ((DMX_CHANNEL_STATE_CFG != ch->state) &&
        (DMX_CHANNEL_STATE_STOP != ch->state))
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    /* TODO: replaced by RPC.
	*/
	#if 0
	p_StatInfo = ali_dmx_see_dev[0].see_buf_init.GlobalStatInfo;
    
	memset((void *)p_StatInfo, 0, sizeof(struct Ali_DmxSeeGlobalStatInfo));
    #endif
	
    return(0);
}



__s32 dmx_channel_see_glb_start
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{    
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* Channel state validation. 
     */
    if (ch->state != DMX_CHANNEL_STATE_STOP)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_RUN;

    return(0);
}



__s32 dmx_channel_see_glb_stop
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    return(0);
}



__s32 dmx_channel_see_glb_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    //__s32 ret;
	//volatile struct Ali_DmxSeeGlobalStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }
	
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    /* TODO: replaced by RPC.
	*/
	#if 0
	p_StatInfo = ali_dmx_see_dev[0].see_buf_init.GlobalStatInfo;
			
	ret = copy_to_user((void __user *)arg, (const void *)p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    #endif
    
    return(0);
}


__s32 dmx_channel_see_glb_realtime_set
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
	if (NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    /* stream state validation. 
     */
    if (DMX_CHANNEL_STATE_IDLE == ch->state)
    {
        return(-EPERM);
    }

    /* TODO: replaced by RPC.
	*/
	#if 0
	ali_dmx_see_dev[0].see_buf_init.statistics->RealTimePrintEn = arg;
    #endif

    return(0);
}

__s32 dmx_channel_hw_reg_cfg
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
    /* channel state validation. 
     */
    if (ch->state != DMX_CHANNEL_STATE_CFG &&
		ch->state != DMX_CHANNEL_STATE_STOP)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    return(0);
}




__s32 dmx_channel_hw_reg_start
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{    	

    /* channel state validation. 
     */
    if (ch->state != DMX_CHANNEL_STATE_STOP)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_RUN;

    return(0);
}



__s32 dmx_channel_hw_reg_stop
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);


    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    return(0);
}



__s32 dmx_channel_hw_reg_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32 ret;
	__u32 HwRegTable[18][5];
	__u32 i, j, k, DmxBaseAddr;
	extern __u32 dmx_hw_id2base_m37(__u32);
	extern __u32 AliRegGet32(__u32);


    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

	DmxBaseAddr = dmx_hw_id2base_m37(0);
	
	memset(HwRegTable, 0, sizeof(HwRegTable));

	for (i = 0, k = 0; i < 0x370; i += 16)
	{
		if (i > 0x64 && i < 0x300)
		{
			if (i != 0xB0 && i != 0xC0 && i != 0x140 && i != 0x1C0)
			{
				continue;
			}
		}

		if (!(i & 0xf))
		{
			HwRegTable[k][0] = DmxBaseAddr + i;
		}

		for (j = 0; j < 4; j++)
		{
			HwRegTable[k][j + 1] = AliRegGet32(HwRegTable[k][0] + j * 4);
		}

		k++;
	}

    ret = copy_to_user((void __user *)arg, &HwRegTable, sizeof(HwRegTable));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);
    
    return(0);
}

static unsigned char temp_ts_pkt[188] = {0};
__s32 dmx_channel_ts_block_wr
(
    struct dmx_ts_pkt_inf *pkt_inf,
    __u32                  param
)
{
	__s32					 byte_cnt;
	__s32					 dsc_byte_cnt, tmp_byte_cnt;
	struct ali_dmx_data_buf *dest_buf;
	struct dmx_channel		*ch;
	__s32					 ret;
	__s32					 cur_data_len;
	__s32					 deencrypt_pkt_cnt;
	PVR_REC_VIDEO_PARAM video_param;
	PVR_RECORD_HEADER_PKT *tmp_pkt = (PVR_RECORD_HEADER_PKT *)temp_ts_pkt;
	static struct dmx_ts_pkt_inf pre_pkt_inf;
	
	if(pkt_inf->continuity == DMX_TS_PKT_CONTINU_DUPLICATE && pre_pkt_inf.pkt_addr == pkt_inf->pkt_addr)
	{
		//printk(KERN_ERR "TS_PKG DUP [%x %d] \n",pkt_inf->pid,pkt_inf->conti_cnt);
		return 0;
	}
	memcpy(&pre_pkt_inf,pkt_inf,sizeof(struct dmx_ts_pkt_inf));
	
	byte_cnt = 0;

	ch = (struct dmx_channel *)param;
	
	dest_buf = &(ch->data_buf_orig);

	/* Write to interchange buffer.
	*/
	byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
									DMX_DATA_SRC_TYPE_KERN);

	cur_data_len = dmx_data_buf_total_len(dest_buf);

	/*	Read out TS packet from interchange buffer, de-encrypt it, write back to normal buffer,
	 *	wait to be read out to userspace.
	*/
	if (cur_data_len >= (ali_dmx_channel_module_legacy.m_dmx_record_blocksize))
	{
		video_param.ifm_offset = INVALID_IFRM_OFFSET;
		video_param.pid = ch->usr_param.video_pid;
		video_param.type = ch->usr_param.video_type;
		/* Wrap to multiple of 64 pakcets to meet DSC requirement.
		 * DMX_DEENCRYPT_BUF_LEN must be multiple of 64 TS pakcets.
		 */
		deencrypt_pkt_cnt = (cur_data_len / 188);
		
		dsc_byte_cnt = dmx_data_buf_rd((void *)(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
										&(ch->data_buf_orig), cur_data_len, DMX_DATA_CPY_DEST_KERN); 

		//DMX_CH_LEGA_DBG("dmx_de_enc_input_buf:%x,dmx_de_enc_output_buf:%x\n", ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
				//ali_dmx_channel_module_legacy.dmx_de_enc_output_buf);

		/* De-encrypt ts packets.
		*/
		#ifdef CONFIG_ALI_PVR_RPC
		if(ch->detail.ts_ch.enc_para != NULL)
		{
			ret = ali_pvr_block_de_encrypt(ch->detail.ts_ch.enc_para, (UINT8 *)ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
								(UINT8 *)ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, deencrypt_pkt_cnt, &video_param);

			if (ret != 0)
			{
				DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

				return(-EFAULT);
			}
			if (INVALID_IFRM_OFFSET != video_param.ifm_offset)
			{
				DMX_CH_LEGA_DBG("DMX offset: 0x%.4x\n", video_param.ifm_offset);
			}
		}
		else
		{
			video_param.ifm_offset = DETECT_IFRM_BY_USER;
		}
		#endif

		/* Write back De-encrypted ts packets to normal buffer to be read out by userspace. 
		*/
		dest_buf = &(ch->data_buf);

		tmp_pkt->sync = PVR_RECORD_BLOCK_HEADER_PKT_SYNC;
		tmp_pkt->ifm_offset = video_param.ifm_offset;
		tmp_pkt->pkt_num = cur_data_len / 188;
		tmp_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)tmp_pkt, 188, DMX_DATA_SRC_TYPE_KERN);

		if(ch->detail.ts_ch.enc_para != NULL)		
		{
			dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)ali_dmx_channel_module_legacy.dmx_de_enc_output_buf,
									cur_data_len, DMX_DATA_SRC_TYPE_KERN);			
		}
		else
		{
			dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
												cur_data_len, DMX_DATA_SRC_TYPE_KERN);
		}
		
		if ((dsc_byte_cnt < 0) || (tmp_byte_cnt < 0))
		{		
			/* Must be buffer overflow, flush all data of this stream 
			* to free memory.
			*/
			dmx_data_buf_flush_all_pkt(dest_buf);		
		}		

		ret = dmx_data_buf_wr_pkt_end(dest_buf);
		if (0 == ret)
		{
		}
		wake_up_interruptible(&(dest_buf->rd_wq));
	}

	return(byte_cnt);
}

__s32 dmx_channel_ts_wr
(
    struct dmx_ts_pkt_inf *pkt_inf,
    __u32                  param
)
{
    __s32                    byte_cnt;
	__s32                    dsc_byte_cnt, tmp_byte_cnt;
    struct ali_dmx_data_buf *dest_buf;
	struct dmx_channel      *ch;
	__s32                    cur_data_len;
	__s32                    deencrypt_pkt_cnt;
	struct timeval current_time;
	struct timeval result;
	__u32 timedelta = 0;
	PVR_REC_VIDEO_PARAM video_param;
	PVR_RECORD_HEADER_PKT *tmp_pkt = (PVR_RECORD_HEADER_PKT *)temp_ts_pkt;
	static struct dmx_ts_pkt_inf pre_pkt_inf;
	
#if	1
	if(pkt_inf->continuity == DMX_TS_PKT_CONTINU_DUPLICATE && pre_pkt_inf.pkt_addr == pkt_inf->pkt_addr)
	{
		//printk(KERN_ERR "TS_PKG DUP [%x %d] \n",pkt_inf->pid,pkt_inf->conti_cnt);
		return 0;
	}
	memcpy(&pre_pkt_inf,pkt_inf,sizeof(struct dmx_ts_pkt_inf));
	
    byte_cnt = 0;

	ch = (struct dmx_channel *)param;

	
	if (ch->usr_param.uncache_para == 1)
	{
		dest_buf = &(ch->data_buf);
		byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
										DMX_DATA_SRC_TYPE_KERN);
		if (byte_cnt < 0)
		{		
			/* Must be buffer overflow, flush all data of this stream 
			* to free memory.
			*/
			dmx_data_buf_flush_all_pkt(dest_buf);	
		}

		wake_up_interruptible(&(dest_buf->rd_wq));

		return(byte_cnt);
	}	

	if((pkt_inf->scramble_flag != 0) && (!ch->data_buf_orig_have_scrambler))
	{
		ch->data_buf_orig_have_scrambler = 1;
	}
	
    dest_buf = &(ch->data_buf_orig);

	/* Write to interchange buffer.
	*/
    byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
                                    DMX_DATA_SRC_TYPE_KERN);

    cur_data_len = dmx_data_buf_total_len(dest_buf);


	if(((cur_data_len/(64 * 188)) > 0) && ((cur_data_len % (64 * 188)) == 0))
	{
		do_gettimeofday(&current_time);

		timeval_subtract(&result, &current_time, &ch->data_buf_orig_update_last_time);

		timedelta = result.tv_sec * 1000 + result.tv_usec/1000;	
	}

    /*  Read out TS packet from interchange buffer, de-encrypt it, write back to normal buffer,
     *  wait to be read out to userspace.
	*/
	//if ((cur_data_len >= /*48128*/ DMX_DEENCRYPT_BUF_LEN) || (timedelta > 300))
	if (cur_data_len >= ali_dmx_channel_module_legacy.m_dmx_record_blocksize)
	{
		video_param.ifm_offset = INVALID_IFRM_OFFSET;
		video_param.pid = ch->usr_param.video_pid;
		video_param.type = ch->usr_param.video_type;
	    //printk("%d, %u\n", cur_data_len, timedelta);
		/* Wrap to multiple of 64 pakcets to meet DSC requirement.
		 * DMX_DEENCRYPT_BUF_LEN must be multiple of 64 TS pakcets.
		 */
		deencrypt_pkt_cnt = (cur_data_len / 188);
		
#ifdef CONFIG_ARM
		dsc_byte_cnt = dmx_data_buf_rd((void *)(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
										&(ch->data_buf_orig), cur_data_len, DMX_DATA_CPY_DEST_KERN); 

#else
#ifdef DMX_USE_CACHE_BUF_ADDR
		dsc_byte_cnt = dmx_data_buf_rd((void *)KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
										&(ch->data_buf_orig), cur_data_len, DMX_DATA_CPY_DEST_KERN); 

		__CACHE_FLUSH_ALI(KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf), cur_data_len);
#else
		dsc_byte_cnt = dmx_data_buf_rd((void *)(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
										&(ch->data_buf_orig), cur_data_len, DMX_DATA_CPY_DEST_KERN); 
#endif
#endif

        //DMX_CH_LEGA_DBG("dmx_de_enc_input_buf:%x,dmx_de_enc_output_buf:%x\n", ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
			    //ali_dmx_channel_module_legacy.dmx_de_enc_output_buf);

		/* De-encrypt ts packets.
		*/
		#ifdef CONFIG_DSC_LEGACY_IOCTL
		
        if(ch->detail.ts_ch.enc_para != NULL)
        {
            if((ch->data_buf_orig_have_scrambler) 
				|| ((!ch->data_buf_orig_have_scrambler) && (ali_dmx_channel_module_legacy.dmx_clearstream_encrypt_mode == FTA_TO_ENCRYPT)))
    		{
    			#ifdef CONFIG_ALI_PVR_RPC
				__s32					 ret;
				//ret = ali_pvr_block_de_encrypt(ch->detail.ts_ch.enc_para, ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
				//					ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, deencrypt_pkt_cnt);
                ret = ali_pvr_ts_de_encrypt((DEEN_CONFIG *)ch->detail.ts_ch.enc_para, (UINT8 *)ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
									(UINT8 *)ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, deencrypt_pkt_cnt, &video_param);

				if (ret != 0)
				{
					DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

					return(-EFAULT);
				}
				if (INVALID_IFRM_OFFSET != video_param.ifm_offset)
				{
					DMX_CH_LEGA_DBG("DMX offset: 0x%.4x\n", video_param.ifm_offset);
				}
				#endif
            }
			else
			{
				video_param.ifm_offset = DETECT_IFRM_BY_USER;
			}
        }
		else
		{
			video_param.ifm_offset = DETECT_IFRM_BY_USER;
		}
		
		#endif

        /* Write back De-encrypted ts packets to normal buffer to be read out by userspace. 
		*/
        dest_buf = &(ch->data_buf);

		tmp_pkt->sync = PVR_RECORD_BLOCK_HEADER_PKT_SYNC;
		tmp_pkt->ifm_offset = video_param.ifm_offset;
		tmp_pkt->pkt_num = cur_data_len / 188;
		tmp_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)tmp_pkt, 188, DMX_DATA_SRC_TYPE_KERN);

		if((ch->detail.ts_ch.enc_para != NULL)
			&& ((ch->data_buf_orig_have_scrambler) 
				|| ((!ch->data_buf_orig_have_scrambler) && (ali_dmx_channel_module_legacy.dmx_clearstream_encrypt_mode == FTA_TO_ENCRYPT))))		
		{
#ifdef CONFIG_ARM
			dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)(ali_dmx_channel_module_legacy.dmx_de_enc_output_buf),
									cur_data_len, DMX_DATA_SRC_TYPE_KERN);

#else
#ifdef DMX_USE_CACHE_BUF_ADDR
			__CACHE_INV_ALI(KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_output_buf), cur_data_len);

 	        dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf,(__u8*)KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_output_buf),
									cur_data_len, DMX_DATA_SRC_TYPE_KERN);
#else
 	        dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, ali_dmx_channel_module_legacy.dmx_de_enc_output_buf,
												cur_data_len, DMX_DATA_SRC_TYPE_KERN);
#endif
#endif		

			
			ch->data_buf_orig_have_scrambler = 0;
		}
		else
		{
#ifdef CONFIG_ARM
			dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
												cur_data_len, DMX_DATA_SRC_TYPE_KERN);	

#else
#ifdef DMX_USE_CACHE_BUF_ADDR
			__CACHE_INV_ALI(KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf), cur_data_len);

			dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf,(__u8*)KSEG0ADDR(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf),
												cur_data_len, DMX_DATA_SRC_TYPE_KERN);
#else
 	        dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, (__u8*)ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
												cur_data_len, DMX_DATA_SRC_TYPE_KERN);	
#endif
#endif		
	
		}
		
		if ((dsc_byte_cnt < 0) || (tmp_byte_cnt < 0))
		{		
			/* Must be buffer overflow, flush all data of this stream 
			* to free memory.
			*/
			dmx_data_buf_flush_all_pkt(dest_buf);
			printk("%s,%d ts channel overflow\n", __FUNCTION__, __LINE__);		
		}			

		do_gettimeofday(&ch->data_buf_orig_update_last_time);
		wake_up_interruptible(&(dest_buf->rd_wq));
	}

    return(byte_cnt);
#else
    byte_cnt = 0;

	ch = (struct dmx_channel *)param;

	if((pkt_inf->scramble_flag != 0) && (ch->detail.ts_ch.enc_para != NULL))
	{
        dest_buf = &(ch->data_buf_orig);

		/* Write to interchange buffer.
		*/
        byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
                                        DMX_DATA_SRC_TYPE_KERN);

	    cur_data_len = dmx_data_buf_total_len(dest_buf);

        /*  Read out TS packet from interchange buffer, de-encrypt it, write back to normal buffer,
         *  wait to be read out to userspace.
		*/
		if (cur_data_len >= DMX_DEENCRYPT_BUF_LEN)
		{
    		/* Wrap to multiple of 64 pakcets to meet DSC requirement.
    		 * DMX_DEENCRYPT_BUF_LEN must be multiple of 64 TS pakcets.
    		 */
    		deencrypt_pkt_cnt = (DMX_DEENCRYPT_BUF_LEN / 188);
    		
            dsc_byte_cnt = dmx_data_buf_rd(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
											&(ch->data_buf_orig), DMX_DEENCRYPT_BUF_LEN, DMX_DATA_CPY_DEST_KERN); 
    
            //DMX_CH_LEGA_DBG("dmx_de_enc_input_buf:%x,dmx_de_enc_output_buf:%x\n", ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
				    //ali_dmx_channel_module_legacy.dmx_de_enc_output_buf);

			/* De-encrypt ts packets.
			*/
			#ifdef CONFIG_DSC_LEGACY_IOCTL
            ret = ali_DeEncrypt(ch->detail.ts_ch.enc_para, ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
				                ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, deencrypt_pkt_cnt);

            if (ret != 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
    
                return(-EFAULT);     
            }
			#endif

	        /* Write back De-encrypted ts packets to normal buffer to be read out by userspace. 
			*/
            dest_buf = &(ch->data_buf);
        	
            dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, ali_dmx_channel_module_legacy.dmx_de_enc_output_buf,
												DMX_DEENCRYPT_BUF_LEN, DMX_DATA_SRC_TYPE_KERN);
			if (dsc_byte_cnt < 0)
			{		
				/* Must be buffer overflow, flush all data of this stream 
				* to free memory.
				*/
				dmx_data_buf_flush_all_pkt(dest_buf);		
			}			
		}

        return(byte_cnt);
	}

	/*when ts scramble status change, we will handle original data to data buffer*/
	dest_buf = &(ch->data_buf_orig);
    cur_data_len = dmx_data_buf_total_len(dest_buf);
	if((cur_data_len > 0) && (ch->detail.ts_ch.enc_para != NULL))
	{
		/* Wrap to multiple of 64 pakcets to meet DSC requirement.
		*/		
		deencrypt_pkt_cnt = (cur_data_len / 188);

		deencrypt_pkt_cnt /= 64;

		deencrypt_pkt_cnt *= 64;	

        dsc_byte_cnt = dmx_data_buf_rd(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
										&(ch->data_buf_orig), deencrypt_pkt_cnt * 188, DMX_DATA_CPY_DEST_KERN); 

        //DMX_CH_LEGA_DBG("dmx_de_enc_input_buf:%x,dmx_de_enc_output_buf:%x\n", ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
			    //ali_dmx_channel_module_legacy.dmx_de_enc_output_buf);

		/* De-encrypt ts packets.
		*/
		#ifdef CONFIG_DSC_LEGACY_IOCTL
        ret = ali_DeEncrypt(ch->detail.ts_ch.enc_para, ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
			                ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, deencrypt_pkt_cnt);

        if (ret != 0)
        {
            DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

            return(-EFAULT);     
        }	    
		#endif
        /* Write back De-encrypted ts packets to normal buffer to be read out by userspace. 
		*/
        dest_buf = &(ch->data_buf);
    	
        dsc_byte_cnt = dmx_data_buf_wr_data(dest_buf, ali_dmx_channel_module_legacy.dmx_de_enc_output_buf,
											deencrypt_pkt_cnt * 188, DMX_DATA_SRC_TYPE_KERN);
		if (dsc_byte_cnt < 0)
		{		
			/* Must be buffer overflow, flush all data of this stream 
			* to free memory.
			*/
			dmx_data_buf_flush_all_pkt(dest_buf);	
		}	
	}

    dest_buf = &(((struct dmx_channel *)param)->data_buf);

    byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
                                    DMX_DATA_SRC_TYPE_KERN);
	if (byte_cnt < 0)
	{		
		/* Must be buffer overflow, flush all data of this stream 
		* to free memory.
		*/
		dmx_data_buf_flush_all_pkt(dest_buf);	
	}

    return(byte_cnt);
#endif	
}


__s32 dmx_channel_ts_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                           ret;
	__u32                           ts_flt_idx;
	__u32                           pid_flt_idx;
    __u32							flt_list_idx;
	struct Ali_DmxDrvTsStrmStatInfo *p_StatInfo;  
	struct Ali_DmxDrvTsStrmStatInfo *p_UsrInfo;

	p_UsrInfo = (struct Ali_DmxDrvTsStrmStatInfo *)arg;

	if (NULL == p_UsrInfo || NULL == ch)
    {
        DMX_API_DBG("%s(), line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
	    ||(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space))
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}

//    ts_flt_idx = p_UsrInfo->TsFltIdx;

    flt_list_idx = p_UsrInfo->TsFltIdx;
    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[flt_list_idx];	


	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);
	
	p_StatInfo = &ch->detail.ts_ch.stat_info;

	p_StatInfo->TsInCnt = ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info.TsInCnt;

    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_ts_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__u32                          ts_flt_idx;
	__u32                          pid_flt_idx;
    __u32 						   flt_list_idx;
	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;
	struct Ali_DmxDrvTsFltStatInfo *p_UsrInfo;

	p_UsrInfo = (struct Ali_DmxDrvTsFltStatInfo *)arg;

	if (NULL == p_UsrInfo || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
	    ||(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space))
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}    


	//Get the info about filter for Rec(TS)
    //ts_flt_idx = p_UsrInfo->TsFltIdx;
    flt_list_idx = p_UsrInfo->TsFltIdx;
    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[flt_list_idx];	
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;


	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_ts_block_read
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    __s32                     flags,
    char __user              *usr_buf,
    size_t                    usr_rd_len
)
{
	__u32 pkt_len,len;
	__s32 byte_cnt;
	//__s32  ret;
	//size_t to_cpy_len;
	//__s32  cur_data_len;
	//__s32  to_cpy_pkt_cnt;
   
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    if (0 == usr_rd_len)
    {
        //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        return(0);
    }

	pkt_len = dmx_data_buf_first_pkt_len(&(ch->data_buf));

	/* nothing could be read out.
	*/
	if ((pkt_len <= 0)&&(flags & O_NONBLOCK))
	{
		return(-EAGAIN);
	}   

    if(pkt_len <= usr_rd_len)
    {
		len = pkt_len;
    } 
    else
	{
		len = usr_rd_len;
    }

    /* byte_cnt may be less than need_cpy_len if data_buf contains less
     * data than need_cpy_len required.
     */
    byte_cnt = dmx_data_buf_rd(usr_buf, &(ch->data_buf), len, 
                               DMX_DATA_CPY_DEST_USER); 

    if (byte_cnt < 0)
    {
        DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d\n", __FUNCTION__, __LINE__,
                      byte_cnt);
    }

    return(byte_cnt);
}


__s32 dmx_channel_ts_read
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    __s32                     flags,
    char __user              *usr_buf,
    size_t                    usr_rd_len
)
{
   __s32 byte_cnt;
   //__s32  ret;
   //size_t to_cpy_len;
   //__s32  cur_data_len;
   //__s32  to_cpy_pkt_cnt;
   
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    if (0 == usr_rd_len)
    {
        //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        return(0);
    }    

    #if 0 
	if (ch->detail.ts_ch.enc_para != NULL)
	{
	    /* Atmost DMX_DEENCRYPT_BUF_LEN bytes could be read out for one read,
	     * due to DMX_DEENCRYPT_BUF limitation.
		*/
        if (usr_rd_len > DMX_DEENCRYPT_BUF_LEN)
        {
    	   to_cpy_len = DMX_DEENCRYPT_BUF_LEN;
    	}
    	else
    	{
    	   to_cpy_len = usr_rd_len;
    	}

		/* Atmost dmx_data_buf_total_len could be read out.
		*/
	    cur_data_len = dmx_data_buf_total_len(&(ch->data_buf));

		if (cur_data_len < to_cpy_len)
		{
            to_cpy_len = cur_data_len;
		}

		/* Wrap to multiple of 64 pakcets to meet DSC requirement.
		*/
		to_cpy_pkt_cnt = (to_cpy_len / 188);
		
		to_cpy_pkt_cnt /= 64;

		to_cpy_pkt_cnt *= 64;
		
        byte_cnt = dmx_data_buf_rd(ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
			                       &(ch->data_buf), to_cpy_pkt_cnt * 188, DMX_DATA_CPY_DEST_KERN); 

        if (byte_cnt > 0)
        {
            if (byte_cnt != (to_cpy_pkt_cnt * 188))
            {
                DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d,to_cpy_pkt_cnt * 188:%d\n", __FUNCTION__, __LINE__, byte_cnt, to_cpy_pkt_cnt * 188);
    
                return(-EFAULT);  
			}

            //DMX_CH_LEGA_DBG("dmx_de_enc_input_buf:%x,dmx_de_enc_output_buf:%x\n", ali_dmx_channel_module_legacy.dmx_de_enc_input_buf,
				    //ali_dmx_channel_module_legacy.dmx_de_enc_output_buf);
			
			#ifdef CONFIG_DSC_LEGACY_IOCTL
            ret = ali_DeEncrypt(ch->detail.ts_ch.enc_para, ali_dmx_channel_module_legacy.dmx_de_enc_input_buf, 
				                ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, to_cpy_pkt_cnt);

            if (ret != 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
    
                return(-EFAULT);     
            }		
            #endif

            ret = copy_to_user(usr_buf, ali_dmx_channel_module_legacy.dmx_de_enc_output_buf, to_cpy_pkt_cnt * 188);
			
            if (ret != 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
    
                return(-EFAULT);     
            }			
        }
		else
		{
            DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d\n", __FUNCTION__, __LINE__,
                            byte_cnt);
		}

		return(to_cpy_pkt_cnt * 188);
	}
	#endif

    /* byte_cnt may be less than need_cpy_len if data_buf contains less
     * data than need_cpy_len required.
     */
    byte_cnt = dmx_data_buf_rd(usr_buf, &(ch->data_buf), usr_rd_len, 
                               DMX_DATA_CPY_DEST_USER); 

    if (byte_cnt < 0)
    {
        DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d\n", __FUNCTION__, __LINE__,
                      byte_cnt);
    }
	
	//printk("%s, line:%d, %d\n", __FUNCTION__, __LINE__, byte_cnt);

    return(byte_cnt);
}


/* TS stream always readable to offload CPU loading from calling of 
 * wake_up_interruptible() for each TS pakcer received, with may be as 
 * much as 30,000 times per second.
*/
__s32 dmx_channel_ts_poll
(
    struct file              *filp,
    struct poll_table_struct *wait,
    struct dmx_output_device *dev,
    struct dmx_channel       *ch
)
{
	__u32 len = 0;
	__s32 mask = 0;

		
	poll_wait(filp, &(ch->data_buf.rd_wq), wait);

	len = dmx_data_buf_total_len(&ch->data_buf);

	if (len > 0)
	{    
		mask |= (POLLIN | POLLRDNORM); 
	}

    return(mask);
}


__s32 dmx_channel_sec_ts_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__s32                          ts_flt_idx;
//	__u32                          pid_flt_idx;
	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;

	if ( NULL == ch )
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}    


	//Get the info about filter for Section filter

    ts_flt_idx = ch->detail.sec_ch.sec_flt_id;
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_sec_stream_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                            ret;
	__u32                            sec_flt_idx;
	__u32                            ts_flt_idx;
//	__u32                            pid_flt_idx;
	struct dmx_sec_flt               *sec_flt;
	struct dmx_ts_flt                *ts_flt;
	struct Ali_DmxDrvSecStrmStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }
    
	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}    

    sec_flt_idx = ch->detail.sec_ch.sec_flt_id ;

	ts_flt_idx = dmx_sec_flt_link_ts_flt_idx(sec_flt_idx);

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	sec_flt = &ali_dmx_sec_flt_module.sec_flt[sec_flt_idx];

	ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx];

	p_StatInfo = &ch->detail.sec_ch.stat_info;
	
	p_StatInfo->TsInCnt = ts_flt->stat_info.TsInCnt;

	p_StatInfo->SecInCnt = sec_flt->stat_info.SecInCnt;

	p_StatInfo->SecOutCnt = sec_flt->stat_info.SecOutCnt;
	
    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    
    return(0);
}


__s32 dmx_channel_sec_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                           ret;
	__u32                           sec_flt_idx;
	__u32                           ts_flt_idx;
//	__u32                           pid_flt_idx;
	struct dmx_sec_flt              *sec_flt;
	struct Ali_DmxDrvSecFltStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	} 

	sec_flt_idx = ch->detail.sec_ch.sec_flt_id;

	ts_flt_idx = dmx_sec_flt_link_ts_flt_idx(sec_flt_idx);

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	sec_flt = &ali_dmx_sec_flt_module.sec_flt[sec_flt_idx];

	p_StatInfo = &sec_flt->stat_info;
	
    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}



__s32 dmx_channel_sec_wr
(
    __u8                     *src,
    __s32                     len,
    enum DMX_SEC_FLT_CB_TYPE  cb_type,
    __u32                     ch
)
{
    __s32                    ret;
    struct ali_dmx_data_buf *dest_buf;

    ret = 0;

    dest_buf = &(((struct dmx_channel *)ch)->data_buf);

    if (DMX_SEC_FLT_CB_TYPE_ERR == cb_type)
    {
    	//DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
        dmx_data_buf_drop_incomplete_pkt(dest_buf);
		//DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        return(0);
    }
    else if (DMX_SEC_FLT_CB_TYPE_PKT_DATA == cb_type)
    {
        ret = dmx_data_buf_wr_data(dest_buf, src, len, DMX_DATA_SRC_TYPE_KERN);
    
        if (ret < len)
        {
            //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
#if 0			
            dmx_data_buf_drop_incomplete_pkt(dest_buf);
#else
            dmx_data_buf_flush_all_pkt(dest_buf);
#endif
        	//DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
			
            return(ret);
        }
    }
    /*DMX_SEC_FLT_CB_TYPE_PKT_END == cb_type 
    */
    else 
    {   
       ret = dmx_data_buf_wr_pkt_end(dest_buf);

       /* Section data successfully stored in buffer.
       */
       if (0 == ret)
       {
           wake_up_interruptible(&(dest_buf->rd_wq));
       }
    }

    return(ret);
}





__s32 dmx_channel_sec_read
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    __s32                     flags,
    char __user              *usr_buf,
    size_t                    usr_rd_len
)
{
    __u32 pkt_len;

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

    if (0 == usr_rd_len)
    {
        return(0);
    }    

    for (;;)
    {
        pkt_len = dmx_data_buf_first_pkt_len(&(ch->data_buf));

        if (pkt_len > 0)
        {
            if (pkt_len > usr_rd_len)
            {
                /* Do nothing if user buffer is shorter than pkt len.
                 */
                return(-EFBIG);
            }

            /* Else data could be read out. */
            break;
        }  

        /* pkt_len <= 0, then nothing could be read out.
        */
        if (flags & O_NONBLOCK)
        {
            return(-EAGAIN);
        }    
		
		DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        dmx_mutex_output_unlock(dev->src_hw_interface_id);

		DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        if (wait_event_interruptible(ch->data_buf.rd_wq, 
            dmx_data_buf_first_pkt_len(&(ch->data_buf)) > 0))
        {
            return(-ERESTARTSYS);
        }

		DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

        if (dmx_mutex_output_lock(dev->src_hw_interface_id))
        {    
            return(-ERESTARTSYS);
        }    
    }

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    dmx_data_buf_rd(usr_buf, &(ch->data_buf), pkt_len, DMX_DATA_CPY_DEST_USER);

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    return(pkt_len);
}







__s32 dmx_channel_sec_poll
(
    struct file              *filp,
    struct poll_table_struct *wait,
    struct dmx_output_device *dev,
    struct dmx_channel       *ch
)
{
    __u32 pkt_len;
    __s32 mask;
    
    poll_wait(filp, &(ch->data_buf.rd_wq), wait);
      
    mask = 0;

    pkt_len = dmx_data_buf_first_pkt_len(&ch->data_buf);
    
    if (pkt_len > 0)
    {    
        mask |= (POLLIN | POLLRDNORM); 
    }

    return(mask);
}


/**************************************************************
* Add for PES
***************************************************************/
__s32 dmx_channel_pes_stream_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                            ret;
	__u32                            pes_flt_idx;
	__u32                            ts_flt_idx;
//	__u32                            pid_flt_idx;
	struct dmx_pes_flt               *pes_flt;
	struct dmx_ts_flt                *ts_flt;
	struct Ali_DmxDrvPesStrmStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }
    
	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}    

    pes_flt_idx = ch->detail.pes_ch.pes_flt_id ;

	ts_flt_idx = dmx_pes_flt_link_ts_flt_idx(pes_flt_idx);

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	pes_flt = &ali_dmx_pes_flt_module.pes_flt[pes_flt_idx];

	ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx];

	p_StatInfo = &ch->detail.pes_ch.stat_info;
	
	p_StatInfo->TsInCnt = ts_flt->stat_info.TsInCnt;

	p_StatInfo->PesInCnt = pes_flt->stat_info.PesInCnt;

	p_StatInfo->PesOutCnt = pes_flt->stat_info.PesOutCnt;
	
    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    
    return(0);
}


__s32 dmx_channel_pes_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                           ret;
	__u32                           pes_flt_idx;
	__u32                           ts_flt_idx;
//	__u32                           pid_flt_idx;
	struct dmx_pes_flt              *pes_flt;
	struct Ali_DmxDrvPesFltStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	} 

	pes_flt_idx = ch->detail.pes_ch.pes_flt_id;

	ts_flt_idx = dmx_pes_flt_link_ts_flt_idx(pes_flt_idx);

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	pes_flt = &ali_dmx_pes_flt_module.pes_flt[pes_flt_idx];

	p_StatInfo = &pes_flt->stat_info;
	
    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}

__s32 dmx_channel_pes_ts_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__s32                          ts_flt_idx;
//	__s32                          pid_flt_idx;
	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;

	if ( NULL == ch )
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if(DMX_OUTPUT_SPACE_USER != ch->usr_param.output_space)
	{
	    DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
	    
	    return(-EPERM);
	}    


	//Get the info about filter for Section filter

    ts_flt_idx = ch->detail.pes_ch.pes_flt_id;
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_pes_wr
(
    __u8                     *src,
    __s32                     len,
    enum DMX_PES_FLT_CB_TYPE  cb_type,
    __u32                     ch
)
{
    __s32                    ret;
    struct ali_dmx_data_buf *dest_buf;

    ret = 0;

//    DMX_CH_LEGA_DBG("%s(),L[%d],src[0x%x], len[%d] \n",__FUNCTION__, __LINE__, src, len);

    dest_buf = &(((struct dmx_channel *)ch)->data_buf);

    if (DMX_PES_FLT_CB_TYPE_ERR == cb_type)
    {
		DMX_CH_LEGA_DBG("%s(),L[%d], DMX_PES_FLT_CB_TYPE_ERR \n", __FUNCTION__, __LINE__);
        
        dmx_data_buf_drop_incomplete_pkt(dest_buf);
        
        return(0);
    }
    else if (DMX_PES_FLT_CB_TYPE_PKT_DATA == cb_type)
    {
//    	DMX_CH_LEGA_DBG("%s(),L[%d], PKT_DATA \n", __FUNCTION__,__LINE__);
        
        ret = dmx_data_buf_wr_data(dest_buf, src, len, DMX_DATA_SRC_TYPE_KERN);
    
        if (ret < len)
        {
            //DMX_CH_LEGA_DBG("%s(),L[%d]--2-11- \n", __FUNCTION__, __LINE__);

            dmx_data_buf_flush_all_pkt(dest_buf);
            
            return(ret);
        }
    }
    /*DMX_PES_FLT_CB_TYPE_PKT_END == cb_type  */
    else 
    {   
// 		DMX_CH_LEGA_DBG("%s(),L[%d], PKT_END \n", __FUNCTION__, __LINE__);
    
		ret = dmx_data_buf_wr_pkt_end(dest_buf);

		/* PES data successfully stored in buffer. */
		if (0 == ret)
		{
		   wake_up_interruptible(&(dest_buf->rd_wq));
		}

    }

//	DMX_CH_LEGA_DBG("%s(),L[%d]\n", __FUNCTION__, __LINE__);
    
    return(ret);
}


__s32 dmx_channel_pes_read
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    __s32                     flags,
    char __user              *usr_buf,
    size_t                    usr_rd_len
)
{
    __u32 pkt_len ,len;


    DMX_CH_LEGA_DBG("%s(),L[%d],buf[0x%x], usr_rd_len[%d]\n", \
        				__FUNCTION__, __LINE__, usr_buf, usr_rd_len);

    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
		DMX_CH_LEGA_DBG("%s(),L[%d], ch_state[%d] \n", __FUNCTION__, __LINE__, ch->state );
        
        return(-EPERM);
    }

    if (0 == usr_rd_len)
    {
    	DMX_CH_LEGA_DBG("%s(),L[%d] \n", __FUNCTION__, __LINE__);
        
        return(-EINVAL);
    }    


	pkt_len = dmx_data_buf_first_pkt_len(&(ch->data_buf));

    DMX_CH_LEGA_DBG("%s(),L[%d], pkt_len[%d], flags[%x] \n", __FUNCTION__, __LINE__, pkt_len, flags);

	/* nothing could be read out.
	*/
	if ((pkt_len <= 0)&&(flags & O_NONBLOCK))
	{
		return(-EAGAIN);
	}   


    if(pkt_len < usr_rd_len)
    {
		len = pkt_len;
    } 
    else
	{
		len = usr_rd_len;
    }


    dmx_data_buf_rd(usr_buf, &(ch->data_buf), len, DMX_DATA_CPY_DEST_USER);

    DMX_CH_LEGA_DBG("%s(),L[%d]\n", __FUNCTION__, __LINE__);

    return(len);
}


__s32 dmx_channel_pes_poll
(
    struct file              *filp,
    struct poll_table_struct *wait,
    struct dmx_output_device *dev,
    struct dmx_channel       *ch
)
{
    __u32 pkt_len;
    __s32 mask;

//	DMX_CH_LEGA_DBG("%s(),L[%d]\n", __FUNCTION__, __LINE__);
    
    poll_wait(filp, &(ch->data_buf.rd_wq), wait);
      
    mask = 0;

    pkt_len = dmx_data_buf_first_pkt_len(&ch->data_buf);
    
    if (pkt_len > 0)
    {    
        mask |= (POLLIN | POLLRDNORM); 
    }

//	DMX_CH_LEGA_DBG("%s(),L[%d]\n", __FUNCTION__, __LINE__);
    
    return(mask);
}



/*******************************************************
*For Video Info
********************************************************/
__s32 dmx_channel_video_stream_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                              ret;
	__u32                              ts_flt_idx;
//	__s32                              pid_flt_idx;
	struct Ali_DmxDrvVideoStrmStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
        ||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }
    

    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[0];

//	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);

	p_StatInfo = (struct Ali_DmxDrvVideoStrmStatInfo *)(&ch->detail.ts_ch.stat_info);

	p_StatInfo->TsInCnt = ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info.TsInCnt;

    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_video_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__u32                          ts_flt_idx;

	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;


	if ( NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
		||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }


	//Get the info about filter for Video
    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[0];
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}



__s32 dmx_channel_video_see_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    //__s32                                   ret;
    
	//volatile struct Ali_DmxSeePlyChStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }


    if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
        ||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }

    /* TODO: replaced by RPC.
	*/
	#if 0
	p_StatInfo = &ali_dmx_see_dev[0].see_buf_init.PlyChStatInfo[0];

    ret = copy_to_user((void __user *)arg, (const void *)p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
	#endif

	return(0);
}


/*******************************************************
*For Audio Info
********************************************************/
__s32 dmx_channel_audio_stream_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                              ret;
	__u32                              ts_flt_idx;
	struct Ali_DmxDrvVideoStrmStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

    if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
        ||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }
    

    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[0];

	p_StatInfo = (struct Ali_DmxDrvVideoStrmStatInfo *)(&ch->detail.ts_ch.stat_info);

	p_StatInfo->TsInCnt = ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info.TsInCnt;

    ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_audio_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__u32                          ts_flt_idx;

	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;


	if ( NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

	if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
		||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }


	//Get the info about filter for audio
    ts_flt_idx = ch->detail.ts_ch.ts_flt_id[0];
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}



__s32 dmx_channel_audio_see_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    //__s32                                   ret;
    
	//volatile struct Ali_DmxSeePlyChStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }


    if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
        ||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }

	/* TODO: replaced by RPC.
	*/
	#if 0
	p_StatInfo = &ali_dmx_see_dev[0].see_buf_init.PlyChStatInfo[1];

    ret = copy_to_user((void __user *)arg, (const void *)p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    #endif
	
	return(0);
}


__s32 dmx_channel_pcr_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                            ret;
	__u32                            ts_flt_idx;
	__u32                            pid_flt_idx;
//	__u32                            pcr_flt_idx;    
//	struct dmx_pcr_flt               *pcr_flt;
	struct dmx_ts_flt                *ts_flt;
	struct Ali_DmxDrvPcrStrmStatInfo *p_StatInfo;

	if (0 == arg || NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }

#if 0
    if((DMX_CHANNEL_OUTPUT_FORMAT_TS != ch->usr_param.output_format)
        ||(DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space))
    {
		DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);  
        
        return(-EPERM);
    }
#endif

    ts_flt_idx = ch->detail.pcr_ch.pcr_flt_id;
    
	pid_flt_idx = dmx_ts_flt_link_pid_flt_idx(ts_flt_idx);
	
//	pcr_flt = &ali_dmx_pcr_flt_module.pcr_flt[pcr_flt_idx];

	ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx];

	p_StatInfo = &ch->detail.pcr_ch.stat_info;

	p_StatInfo->TsInCnt = ts_flt->stat_info.TsInCnt;

	p_StatInfo->LastPcrVal = ch->detail.pcr_ch.latest_pcr;
	
	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }
    
    return(0);
}

__s32 dmx_channel_pcr_filter_info_get
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                          ret;
	__u32                          ts_flt_idx;

	struct Ali_DmxDrvTsFltStatInfo *p_StatInfo;


	if ( NULL == ch)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EPERM);
    }


    if (DMX_OUTPUT_SPACE_KERNEL != ch->usr_param.output_space)
    {
        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
    
        return(-EFAULT);
    }


	//Get the info about filter for audio
    ts_flt_idx = ch->detail.pcr_ch.pcr_flt_id;
    
    p_StatInfo = &ali_dmx_ts_flt_module.ts_flt[ts_flt_idx].stat_info;

	ret = copy_to_user((void __user *)arg, p_StatInfo, _IOC_SIZE(cmd));

    if (ret != 0)
    {
        DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    return(0);
}


__s32 dmx_channel_status_reset
(
    struct dmx_output_device *dev,
    struct dmx_channel        *ch
)
{
    __u32                     i;
    __s32                     ret = 0;
    struct dmx_ts_channel    *ts_ch;
    struct dmx_sec_channel   *sec_ch;
    struct dmx_pes_channel   *pes_ch;
    struct dmx_pcr_channel   *pcr_ch;
    struct dmx_channel_param *usr_para;
	struct dmx_ts_flt		 *ts_flt;
	struct dmx_sec_flt		 *sec_flt;
	struct dmx_pes_flt		 *pes_flt;

	usr_para = &ch->usr_param;
	
    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            ts_ch = &(ch->detail.ts_ch);
            
			memset(&ts_ch->stat_info, 0, sizeof(ts_ch->stat_info));

            if (DMX_OUTPUT_SPACE_KERNEL == usr_para->output_space)
            {
				ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_ch->ts_flt_id[0]];

				memset(&ts_flt->stat_info, 0, sizeof(ts_flt->stat_info));
            }
            else if (DMX_OUTPUT_SPACE_USER == usr_para->output_space)
            {
                for (i = 0; i < usr_para->ts_param.pid_list_len; i++)
                {
					ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_ch->ts_flt_id[i]];

					memset(&ts_flt->stat_info, 0, sizeof(ts_flt->stat_info));						
                }
            }
            else 
            {
                return(-EFAULT);
            }
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {            
            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                return(-EFAULT);
            }
            
            sec_ch = &(ch->detail.sec_ch);

			memset(&sec_ch->stat_info, 0, sizeof(sec_ch->stat_info));
            
			sec_flt = &ali_dmx_sec_flt_module.sec_flt[sec_ch->sec_flt_id];

			memset(&sec_flt->stat_info, 0, sizeof(sec_flt->stat_info));
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
		{		    
		    if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
		    {
		        return(-EFAULT);
		    }
		    
		    pes_ch = &(ch->detail.pes_ch);
		    
			memset(&pes_ch->stat_info, 0, sizeof(pes_ch->stat_info));
            
			pes_flt = &ali_dmx_pes_flt_module.pes_flt[pes_ch->pes_flt_id];

			memset(&pes_flt->stat_info, 0, sizeof(pes_flt->stat_info));
		}
        break;        

        case DMX_CHANNEL_OUTPUT_FORMAT_PCR:
        {
            if (DMX_OUTPUT_SPACE_KERNEL != usr_para->output_space)
            {
                return(-EFAULT);
            }

			if((dev->src_hw_interface_id == ALI_SWDMX0_OUTPUT_HWIF_ID) || (dev->src_hw_interface_id == ALI_SWDMX1_OUTPUT_HWIF_ID))
			{
				break;
			}

            pcr_ch = &(ch->detail.pcr_ch);

			memset(&pcr_ch->stat_info, 0, sizeof(pcr_ch->stat_info));
        }
        break;

        default:
        {            
            ret = -EINVAL;
        }
        break;
    }

    return(0);
}


__s32 dmx_channel_start
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    struct dmx_ts_channel     *ts_ch;
    struct dmx_sec_channel    *sec_ch;
    struct dmx_pcr_channel    *pcr_ch;
    struct dmx_pes_channel    *pes_ch;
    struct dmx_channel_param  *usr_para;
    __s32                      ret;
    __u32                      i;
    __u32                      j;
    struct ali_dmx_data_buf   *data_buf;
    struct Ali_DmxSecMaskInfo  sec_mask;
	struct dmx_data_engine_output *engine = NULL;
	__u32  src_hw_interface_id;
	struct dmx_dsc_fd_param i_dmx_dsc_fd_param;	
	DEEN_CONFIG *i_deen_config = NULL;

#ifdef CONFIG_DSC_LEGACY_IOCTL
	struct ca_session_attr decrypt_ca_session_attr;
#endif
#ifdef CONFIG_ALI_PVR_RPC
	struct pvr_ca_attr encrypt_ca_session_attr;
#endif
#ifdef CONFIG_ALI_SEC
	struct sec_ca_attr sec_ca_session_attr;
#endif
	struct dmx_channel       *dsc_ch;
	
    /* ch state validation.
    */
    if (ch->state != DMX_CHANNEL_STATE_CFG)
    {
        return(-EPERM);
    }

            //DMX_CH_LEGA_DBG("%s,%d\n",__FUNCTION__,__LINE__);
    usr_para = &(ch->usr_param);

    ret = copy_from_user(usr_para, (void __user *)arg, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    data_buf = &(ch->data_buf);

    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

            ts_ch = &(ch->detail.ts_ch);

            memset(ts_ch, 0, sizeof(struct dmx_ts_channel));

            if (DMX_OUTPUT_SPACE_KERNEL == usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                engine = &ali_dmx_data_engine_module.engine_output[dev->src_hw_interface_id];
                engine->dmx_see_dev =(void *) usr_para->ts_param.kern_recv_info.kern_recv_routine_para;

                ret = dmx_ts_flt_register(dev->src_hw_interface_id,
					                    /* App may add bit 13~14 of PID as a 
					                    *  video/audio format identification.
					                    *  Ugly.
				                        */
                                        usr_para->ts_param.pid_list[0] & 0x1FFF, 
                                        dmx_see_buf_wr_ts, dev->src_hw_interface_id);
                
                if (ret < 0)
                {
                    DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                    
                    return(ret);
                }

                ts_ch->ts_flt_id[0] = ret;

				dmx_ts_flt_start(ts_ch->ts_flt_id[0]);
            }
            else if (DMX_OUTPUT_SPACE_USER == usr_para->output_space)
            {
                //DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                dmx_data_buf_list_init(&(ch->data_buf));
				dmx_data_buf_list_init(&(ch->data_buf_orig));
				ch->data_buf_orig_have_scrambler = 0;
				ch->data_buf_orig_update_last_time.tv_sec = 0;
				ch->data_buf_orig_update_last_time.tv_usec = 0;
				
				if(1 == usr_para->rec_whole_tp)
				{
				    dmx_dbg_ch_legacy_api_show("%s,line:%d\n", __FUNCTION__, __LINE__);

					if(ali_dmx_channel_module_legacy.m_dmx_record_mode == DMX_REC_OUT_BLOCK)
					{
	                   	ret = dmx_pid_flt_register(dev->src_hw_interface_id, DMX_WILDCARD_PID, 
	                   							   dmx_channel_ts_block_wr, (__u32)ch);					
					}
					else
					{
	                   	ret = dmx_pid_flt_register(dev->src_hw_interface_id, DMX_WILDCARD_PID, 
	                   							   dmx_channel_ts_wr, (__u32)ch);
					}
                   	
                   	if (ret < 0)
                   	{                   
                   		dmx_dbg_ch_legacy_api_show("%s,%d,src_hw_interface_id:%d\n", 
							                       __FUNCTION__, __LINE__, dev->src_hw_interface_id);
                   	
                   		return(ret);
                   	}

                    ts_ch->ts_flt_id[0] = ret;
					
                    ret = dmx_pid_flt_start(ts_ch->ts_flt_id[0]);
                
                    if (ret < 0)
                    {
                        dmx_dbg_ch_legacy_api_show("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                
                        return(ret);
                    }
				}
                else
                {
                    dmx_dbg_ch_legacy_api_show("%s,%d,usr_para->dec_para:%d,usr_para->enc_para\n",
						                       __FUNCTION__, __LINE__, usr_para->dec_para,
						                       usr_para->enc_para);

                    if(usr_para->enc_para != NULL)
                    {
                        ts_ch->enc_para = (void *)kmalloc(sizeof(DEEN_CONFIG), GFP_KERNEL);
    
                        if(NULL == ts_ch->enc_para)
                        {
                            DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, -EFAULT);
                        
                            return(-EFAULT);
    					}						
						
						i_deen_config = (DEEN_CONFIG *)(ts_ch->enc_para);
						memset(i_deen_config, 0, sizeof(DEEN_CONFIG));

						if(usr_para->m_dmx_dsc_param_mode == DMX_DSC_PARAM_INTERNAL_ID_MODE)
						{
							dsc_ch = (struct dmx_channel *)usr_para->enc_para;
							memcpy(i_deen_config, dsc_ch->m_p_dmx_dsc_deen_param, sizeof(DEEN_CONFIG));
						}
						else if(usr_para->m_dmx_dsc_param_mode == DMX_DSC_PARAM_FD_MODE)
						{
	                        if (0 != copy_from_user(&i_dmx_dsc_fd_param, 
								                    (void __user *)(usr_para->enc_para), sizeof(struct dmx_dsc_fd_param)))
	                        {
	                            DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, -EFAULT);
	                        
	                            return(-EFAULT);
	                        } 

#ifdef CONFIG_DSC_LEGACY_IOCTL
#ifdef CONFIG_ALI_SEC
                                if(sec_check_is_sec_fd((__s32)i_dmx_dsc_fd_param.decrypt_fd))
                                {
                                            memset(&sec_ca_session_attr,0,sizeof(sec_ca_session_attr));
                                	    	if((i_dmx_dsc_fd_param.decrypt_fd >= 0) 
	    							&& (sec_get_session_attr((__s32)i_dmx_dsc_fd_param.decrypt_fd, &sec_ca_session_attr) >= 0))
	    						{
	    							i_deen_config->do_decrypt = 1;
	    							i_deen_config->dec_dev = sec_ca_session_attr.sub_dev_see_hdl;
	    							i_deen_config->Decrypt_Mode = sec_ca_session_attr.crypt_mode;
	    							i_deen_config->dec_dmx_id = sec_ca_session_attr.stream_id;
	    						}
                                }
                                else
#endif
                                {
							if((i_dmx_dsc_fd_param.decrypt_fd >= 0) 
								&& (ca_dsc_get_session_attr((__s32)i_dmx_dsc_fd_param.decrypt_fd, &decrypt_ca_session_attr) >= 0))
							{
								i_deen_config->do_decrypt = 1;
								i_deen_config->dec_dev = decrypt_ca_session_attr.sub_dev_see_hdl;
								i_deen_config->Decrypt_Mode = decrypt_ca_session_attr.crypt_mode;
								i_deen_config->dec_dmx_id = decrypt_ca_session_attr.stream_id;
							}
                                }
#ifdef CONFIG_ALI_PVR_RPC
							memset(&encrypt_ca_session_attr,0,sizeof(encrypt_ca_session_attr));
	                        memset(&decrypt_ca_session_attr,0,sizeof(decrypt_ca_session_attr));
	                        if(pvr_check_is_pvr_fd((__s32)i_dmx_dsc_fd_param.encrypt_fd))
	                        {
	    						if((i_dmx_dsc_fd_param.encrypt_fd >= 0) 
	    							&& (pvr_get_session_attr((__s32)i_dmx_dsc_fd_param.encrypt_fd, &encrypt_ca_session_attr) >= 0))
	    						{
	    							i_deen_config->do_encrypt = 1;
	    							i_deen_config->enc_dev = encrypt_ca_session_attr.sub_dev_see_hdl;
	    							i_deen_config->Encrypt_Mode = encrypt_ca_session_attr.crypt_mode;
	    							i_deen_config->enc_dmx_id = encrypt_ca_session_attr.stream_id;
	    						}
	                        }
#ifdef CONFIG_ALI_SEC
                                else if(sec_check_is_sec_fd((__s32)i_dmx_dsc_fd_param.encrypt_fd))
                                {
                                            memset(&sec_ca_session_attr,0,sizeof(sec_ca_session_attr));
                                	    	if((i_dmx_dsc_fd_param.encrypt_fd >= 0) 
	    							&& (sec_get_session_attr((__s32)i_dmx_dsc_fd_param.encrypt_fd, &sec_ca_session_attr) >= 0))
	    						{
	    							i_deen_config->do_encrypt = 1;
	    							i_deen_config->enc_dev = sec_ca_session_attr.sub_dev_see_hdl;
	    							i_deen_config->Encrypt_Mode = sec_ca_session_attr.crypt_mode;
	    							i_deen_config->enc_dmx_id = sec_ca_session_attr.stream_id;
	    						}
                                }
#endif

	                        else
	                        {
	                            if((i_dmx_dsc_fd_param.encrypt_fd >= 0)
	                            && (ca_dsc_get_session_attr((__s32)i_dmx_dsc_fd_param.encrypt_fd, &decrypt_ca_session_attr) >= 0))
	                            {
	                                i_deen_config->do_encrypt = 1;
	                                i_deen_config->enc_dev = decrypt_ca_session_attr.sub_dev_see_hdl;
	                                i_deen_config->Encrypt_Mode = decrypt_ca_session_attr.crypt_mode;
	                                i_deen_config->enc_dmx_id = decrypt_ca_session_attr.stream_id;
	                            }                             
	                        }
#endif/* CONFIG_ALI_PVR_RPC */
#endif
						}
                    }

                    for (i = 0; i < usr_para->ts_param.pid_list_len; i++)
                    {
                    	dmx_dbg_ch_legacy_api_show("%s,%d,PID:%d\n", __FUNCTION__, __LINE__,
							                       usr_para->ts_param.pid_list[i]);

						if(ali_dmx_channel_module_legacy.m_dmx_record_mode == DMX_REC_OUT_BLOCK)
						{
	                        ts_ch->ts_flt_id[i] = dmx_ts_flt_register(dev->src_hw_interface_id,
					                                                  usr_para->ts_param.pid_list[i], 
					                                                  dmx_channel_ts_block_wr, (__u32)ch);						
						}
						else
						{
							if (1 != usr_para->ts_param.needdiscramble[i])
							{
								src_hw_interface_id = dev->src_hw_interface_id;
							}
							else
							{							
								sed_add_scramble_pid(0, (UINT32)dev, usr_para->ts_param.pid_list[i]);
							
								ts_ch->ts_flt_id2see[i] = dmx_ts_flt_register(dev->src_hw_interface_id, usr_para->ts_param.pid_list[i], 
																				dmx_see_buf_wr_ts,
																				dev->src_hw_interface_id);
								
								if (ts_ch->ts_flt_id2see[i] < 0)
								{
									dmx_dbg_ch_legacy_api_show("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ts_ch->ts_flt_id2see[i]);

									for (j = 0; j < i; j++)
									{
										dmx_ts_flt_unregister(ts_ch->ts_flt_id[j]);
										if (1 == usr_para->ts_param.needdiscramble[j])
										{
											dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[j]);
										}
									}
							
									return(-EMFILE);
								}
							
								src_hw_interface_id = ALI_SEETOMAIN_BUF_HWIF_ID;
							}
								
	                        ts_ch->ts_flt_id[i] = dmx_ts_flt_register(src_hw_interface_id,
																		usr_para->ts_param.pid_list[i], 
																		dmx_channel_ts_wr, (__u32)ch);							
						}
						
                        if (ts_ch->ts_flt_id[i] < 0)
                        {
                            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
                            
                            for (j = 0; j < i; j++)
                            {
                                dmx_ts_flt_unregister(ts_ch->ts_flt_id[j]);
								if ((ali_dmx_channel_module_legacy.m_dmx_record_mode != DMX_REC_OUT_BLOCK)
									&& (1 == usr_para->ts_param.needdiscramble[j]))
								{
									dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[j]);
								}								
                            }
                        
                            return(-EMFILE);
                        }
                        else
                        {
							dmx_ts_flt_start(ts_ch->ts_flt_id[i]);
							if ((ali_dmx_channel_module_legacy.m_dmx_record_mode != DMX_REC_OUT_BLOCK)
									&& (1 == usr_para->ts_param.needdiscramble[i]))
							{
					        	dmx_ts_flt_start(ts_ch->ts_flt_id2see[i]);
							}							
                        }    
                    }
				}
            }
            else 
            {
                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }

            sec_ch = &(ch->detail.sec_ch);

            memset(sec_ch, 0, sizeof(struct dmx_sec_channel));

            dmx_data_buf_list_init(&(ch->data_buf));

            /* Section mask translation from legacy section param to
             * current section param.
            */
            memset(&sec_mask, 0, sizeof(sec_mask));

            sec_mask.MatchLen = usr_para->sec_param.mask_len;

            memcpy(&(sec_mask.Mask), &(usr_para->sec_param.mask),
                   sec_mask.MatchLen);

            memcpy(&(sec_mask.Match), &(usr_para->sec_param.value),
                   sec_mask.MatchLen);

			if (1 != usr_para->sec_param.needdiscramble)
			{
				src_hw_interface_id = dev->src_hw_interface_id;
			}
			else
			{				
				sed_add_scramble_pid(0, (UINT32)dev, usr_para->sec_param.pid);

				sec_ch->ts_flt_id2see = dmx_ts_flt_register(dev->src_hw_interface_id, usr_para->sec_param.pid, 
																dmx_see_buf_wr_ts,
																dev->src_hw_interface_id);
				
				if (sec_ch->ts_flt_id2see < 0)
				{
					dmx_dbg_ch_legacy_api_show("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, sec_ch->ts_flt_id2see);
					return(-EMFILE);
				}
			
				src_hw_interface_id = ALI_SEETOMAIN_BUF_HWIF_ID;				
			}

            ret = dmx_sec_flt_register(src_hw_interface_id, 
                                       usr_para->sec_param.pid, 
                                       &sec_mask,
                                       dmx_channel_sec_wr,
                                       (__u32)(ch));
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

				if (1 == usr_para->sec_param.needdiscramble)
				{
					dmx_ts_flt_unregister(sec_ch->ts_flt_id2see);
				}
			
                return(ret);
            }

            sec_ch->sec_flt_id = ret;

            dmx_sec_flt_start(sec_ch->sec_flt_id);
			
			if (1 == usr_para->sec_param.needdiscramble)
			{
				dmx_ts_flt_start(sec_ch->ts_flt_id2see);
			}	

        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
        
            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
        
                return(-EFAULT);
            }
        
            pes_ch = &(ch->detail.pes_ch);
        
            memset(pes_ch, 0, sizeof(struct dmx_pes_channel));
        
            dmx_data_buf_list_init(&(ch->data_buf));

			if (1 != usr_para->pes_param.needdiscramble)
			{
				src_hw_interface_id = dev->src_hw_interface_id;
			}
			else
			{			
				sed_add_scramble_pid(0, (UINT32)dev, usr_para->pes_param.pid);

				pes_ch->ts_flt_id2see = dmx_ts_flt_register(dev->src_hw_interface_id, usr_para->pes_param.pid, 
																dmx_see_buf_wr_ts,
																dev->src_hw_interface_id);
				
				if (pes_ch->ts_flt_id2see < 0)
				{
					dmx_dbg_ch_legacy_api_show("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, pes_ch->ts_flt_id2see);
					return(-EMFILE);
				}
			
				src_hw_interface_id = ALI_SEETOMAIN_BUF_HWIF_ID;				
			}
			
            /* Pes mask translation from legacy Pes param to
             * current Pes param.
            */        
            ret = dmx_pes_flt_register(src_hw_interface_id, 
                                       usr_para->pes_param.pid, 
                                       dmx_channel_pes_wr,
                                       (__u32)(ch));
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

				if (1 == usr_para->pes_param.needdiscramble)
				{
					dmx_ts_flt_unregister(pes_ch->ts_flt_id2see);
				}
			
                return(ret);
            }
        
            pes_ch->pes_flt_id = ret;
        
            dmx_pes_flt_start(pes_ch->pes_flt_id);

			if (1 == usr_para->pes_param.needdiscramble)
			{
				dmx_ts_flt_start(pes_ch->ts_flt_id2see);
			}			
        }      
        break;



        case DMX_CHANNEL_OUTPUT_FORMAT_PCR:
        {
            //DMX_CH_LEGA_DBG("%s,%d\n",__FUNCTION__,__LINE__);

            if (DMX_OUTPUT_SPACE_KERNEL != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }

			if((dev->src_hw_interface_id == ALI_SWDMX0_OUTPUT_HWIF_ID) || (dev->src_hw_interface_id == ALI_SWDMX1_OUTPUT_HWIF_ID))
			{
				break;
			}

            pcr_ch = &(ch->detail.pcr_ch);

            memset(pcr_ch, 0, sizeof(struct dmx_pcr_channel));

            //DMX_CH_LEGA_DBG("%s,%d,pid:%d\n",__FUNCTION__,__LINE__, usr_para->pcr_param.pid);

            ret = dmx_pcr_flt_register(dev->src_hw_interface_id, usr_para->pcr_param.pid, 
                                       dmx_channel_pcr_wr, usr_para->ts_param.kern_recv_info.kern_recv_routine_para,
                                       NULL, dev->src_hw_interface_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);
            
                return(ret);
            }

            //DMX_CH_LEGA_DBG("%s,%d,pid:%d\n",__FUNCTION__,__LINE__, usr_para->pcr_param.pid);
			
            pcr_ch->pcr_flt_id = ret;
            
            ret = dmx_pcr_flt_start(pcr_ch->pcr_flt_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
            
                return(ret);
            }       

            //DMX_CH_LEGA_DBG("%s,%d,pid:%d\n",__FUNCTION__,__LINE__, usr_para->pcr_param.pid);
			
        }
        break;

        default:
        {
            return(-EINVAL);
        }
        break;
    }

    ch->state = DMX_CHANNEL_STATE_RUN;

    return(0);
}




__s32 dmx_channel_stop
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch
)
{
    __u32                     i;
    __s32                     ret;
    struct dmx_ts_channel    *ts_ch;
    struct dmx_sec_channel   *sec_ch;
    struct dmx_pes_channel   *pes_ch;
    struct dmx_pcr_channel   *pcr_ch;
    struct dmx_channel_param *usr_para;
	struct dmx_ts_flt		 *ts_flt;
	struct dmx_sec_flt		 *sec_flt;
	struct dmx_pes_flt		 *pes_flt;

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    /* ch state validation, return success in all state.
    */
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(0);
    }

    usr_para = &ch->usr_param;

	memset(&g_stat_info, 0, sizeof(g_stat_info));

    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            ts_ch = &(ch->detail.ts_ch);
            
			memset(&ts_ch->stat_info, 0, sizeof(ts_ch->stat_info));

            if (DMX_OUTPUT_SPACE_KERNEL == usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

				ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_ch->ts_flt_id[0]];

				memset(&ts_flt->stat_info, 0, sizeof(ts_flt->stat_info));

                ret = dmx_ts_flt_stop(ts_ch->ts_flt_id[0]);
                
                if (ret < 0)
                {
                    DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                    
                    return(ret);
                }
            }
            else if (DMX_OUTPUT_SPACE_USER == usr_para->output_space)
            {
				if(1 == usr_para->rec_whole_tp)
				{
				    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
                    dmx_pid_flt_stop(ts_ch->ts_flt_id[0]);
				}
				else
				{
				    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
                    for (i = 0; i < usr_para->ts_param.pid_list_len; i++)
                    {
    					ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_ch->ts_flt_id[i]];
    
    					memset(&ts_flt->stat_info, 0, sizeof(ts_flt->stat_info));
    
                        dmx_ts_flt_stop(ts_ch->ts_flt_id[i]);

						if (1 == usr_para->ts_param.needdiscramble[i])
						{
				        	dmx_ts_flt_stop(ts_ch->ts_flt_id2see[i]);
						}						
                    }
				}
                                   
                dmx_data_buf_flush_all_pkt(&ch->data_buf);
				dmx_data_buf_flush_all_pkt(&ch->data_buf_orig);
				ch->data_buf_orig_have_scrambler = 0;
				ch->data_buf_orig_update_last_time.tv_sec = 0;
				ch->data_buf_orig_update_last_time.tv_usec = 0;				
            }
            else 
            {
                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
            
            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }
            
            sec_ch = &(ch->detail.sec_ch);

			memset(&sec_ch->stat_info, 0, sizeof(sec_ch->stat_info));
            
			sec_flt = &ali_dmx_sec_flt_module.sec_flt[sec_ch->sec_flt_id];

			memset(&sec_flt->stat_info, 0, sizeof(sec_flt->stat_info));

            ret = dmx_sec_flt_stop(sec_ch->sec_flt_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                
                return(ret);
            }

			if (1 == usr_para->sec_param.needdiscramble)
			{
				dmx_ts_flt_stop(sec_ch->ts_flt_id2see);
			}
			
            dmx_data_buf_flush_all_pkt(&ch->data_buf);

            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
		{
		    DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		    
		    if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
		    {
		        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

		        return(-EFAULT);
		    }
		    
		    pes_ch = &(ch->detail.pes_ch);
		    
			memset(&pes_ch->stat_info, 0, sizeof(pes_ch->stat_info));
            
			pes_flt = &ali_dmx_pes_flt_module.pes_flt[pes_ch->pes_flt_id];

			memset(&pes_flt->stat_info, 0, sizeof(pes_flt->stat_info));

		    ret = dmx_pes_flt_stop(pes_ch->pes_flt_id);
		    
		    if (ret < 0)
		    {
		        DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
		        
		        return(ret);
		    }

			if (1 == usr_para->pes_param.needdiscramble)
			{
				dmx_ts_flt_stop(pes_ch->ts_flt_id2see);
			}
			
		    dmx_data_buf_flush_all_pkt(&ch->data_buf);

		    DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		}
        break;        

        case DMX_CHANNEL_OUTPUT_FORMAT_PCR:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

            if (DMX_OUTPUT_SPACE_KERNEL != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }

			if((dev->src_hw_interface_id == ALI_SWDMX0_OUTPUT_HWIF_ID) || (dev->src_hw_interface_id == ALI_SWDMX1_OUTPUT_HWIF_ID))
			{
				break;
			}

            pcr_ch = &(ch->detail.pcr_ch);

			memset(&pcr_ch->stat_info, 0, sizeof(pcr_ch->stat_info));

            ret = dmx_pcr_flt_stop(pcr_ch->pcr_flt_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                
                return(ret);
            }
        }
        break;

        default:
        {
            DMX_CH_LEGA_DBG("%s,line:%d,format:%d\n", __FUNCTION__, __LINE__, usr_para->output_format);
            
            ret = -EINVAL;
        }
        break;
    }

    ch->state = DMX_CHANNEL_STATE_STOP;

    return(0);
}



__s32 dmx_channel_open
(
    struct inode *inode,
    struct file  *file
)
{
    __u32                     i;
    struct dmx_output_device *dev;
    struct dmx_channel       *ch;

    //DMX_CH_LEGA_DBG("%s, go\n", __FUNCTION__);

    dev = container_of(inode->i_cdev, struct dmx_output_device, cdev);

    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
    {
        return(-ERESTARTSYS);
    }

    for (i = 0; i < DMX_CHANNEL_CNT; i++)
    {
        ch = &(ali_dmx_channel_module_legacy.channel[i]);

        if (DMX_CHANNEL_STATE_IDLE == ch->state)
        {
            ch->state = DMX_CHANNEL_STATE_CFG;

            ch->dmx_output_device = (void *)dev;

            file->private_data = ch;

            //DMX_CH_LEGA_DBG("Got idle stream %d\n", i);

            break;
        }
    }

    dmx_mutex_output_unlock(dev->src_hw_interface_id);

    if (i >= DMX_CHANNEL_CNT)
    {
        //DMX_CH_LEGA_DBG("No idle ch!\n");

        return(-EMFILE);
    }

#ifdef DMX_HWDMX_DATA_ENGINE_DELAY
		if(((dev->src_hw_interface_id == ALI_HWDMX0_OUTPUT_HWIF_ID)
			|| (dev->src_hw_interface_id == ALI_HWDMX1_OUTPUT_HWIF_ID)
			|| (dev->src_hw_interface_id == ALI_HWDMX2_OUTPUT_HWIF_ID)
			|| (dev->src_hw_interface_id == ALI_HWDMX3_OUTPUT_HWIF_ID))
			&& (dmx_data_engine_module_get_state(dev->src_hw_interface_id) != DMX_DATA_ENGINE_TASK_HW_STATE_RUN))
		{
			DMX_PRINTK("hw dmx_data_engine_module_init_kern\n");
			if(dev->src_hw_interface_id == ALI_HWDMX0_OUTPUT_HWIF_ID)
			{
				dmx_data_engine_module_init_kern(ALI_HWDMX0_OUTPUT_HWIF_ID, ALI_HWDMX0_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
			}
			else if(dev->src_hw_interface_id == ALI_HWDMX1_OUTPUT_HWIF_ID)
			{
				dmx_data_engine_module_init_kern(ALI_HWDMX1_OUTPUT_HWIF_ID, ALI_HWDMX1_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
			}
			else if(dev->src_hw_interface_id == ALI_HWDMX2_OUTPUT_HWIF_ID)
			{
				dmx_data_engine_module_init_kern(ALI_HWDMX2_OUTPUT_HWIF_ID, ALI_HWDMX2_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
			}
			else if(dev->src_hw_interface_id == ALI_HWDMX3_OUTPUT_HWIF_ID)
			{
				dmx_data_engine_module_init_kern(ALI_HWDMX3_OUTPUT_HWIF_ID, ALI_HWDMX3_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
			}			
		}
#endif

    return(0);
}




__s32 dmx_channel_close
(
    struct inode *inode,
    struct file  *filep
)
{
    __s32                     ret;
    __u32                     i;
    struct dmx_channel       *ch;
    struct dmx_ts_channel    *ts_ch;
    struct dmx_sec_channel   *sec_ch;
    struct dmx_pes_channel   *pes_ch;
    struct dmx_pcr_channel   *pcr_ch;
    struct dmx_channel_param *usr_para;
    struct dmx_output_device *dev;

    ch = filep->private_data;

    dev = (struct dmx_output_device *)ch->dmx_output_device;

    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
    {
        return(-ERESTARTSYS);
    }

	/* for dsc mutiple process */
	if(ch->m_dmx_dsc_id && ch->m_p_dmx_dsc_deen_param)
	{
		ch->m_dmx_dsc_id = 0/*NULL*/;

		kfree(ch->m_p_dmx_dsc_deen_param);
		ch->m_p_dmx_dsc_deen_param = NULL;		

	    dmx_mutex_output_unlock(dev->src_hw_interface_id);
		
        return(0);		
	}

    if (ch->state == DMX_CHANNEL_STATE_CFG)
    {
	    ch->state = DMX_CHANNEL_STATE_IDLE;

	    dmx_mutex_output_unlock(dev->src_hw_interface_id);
		
        return(0);
    }

    ret = 0;
    
    usr_para = &(ch->usr_param);

    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            ts_ch = &(ch->detail.ts_ch);
            
            if (DMX_OUTPUT_SPACE_KERNEL == usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                ret = dmx_ts_flt_unregister(ts_ch->ts_flt_id[0]);
                
                if (ret < 0)
                {
                    DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
                }
            }
            else if (DMX_OUTPUT_SPACE_USER == usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

				if(1 == usr_para->rec_whole_tp)
				{
				    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
                    dmx_pid_flt_unregister(ts_ch->ts_flt_id[0]);
				}				
                else
                {
                    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
                    for (i = 0; i < usr_para->ts_param.pid_list_len; i++)
                    {
                        ret = dmx_ts_flt_unregister(ts_ch->ts_flt_id[i]);
    
                        if (ret < 0)
                        {
                            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
                        }

						/* Need Discrambling?
						*/
						if (1 == usr_para->ts_param.needdiscramble[i])
						{
				        	dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[i]);
							
						    sed_delete_scramble_pid(0, (UINT32)dev, usr_para->ts_param.pid_list[i]);
						}						
                    }
				}

                if (DMX_CHANNEL_STATE_RUN == ch->state)
                {
                    dmx_data_buf_flush_all_pkt(&ch->data_buf);
					dmx_data_buf_flush_all_pkt(&ch->data_buf_orig);
					ch->data_buf_orig_have_scrambler = 0;
					ch->data_buf_orig_update_last_time.tv_sec = 0;
					ch->data_buf_orig_update_last_time.tv_usec = 0;					
                }
				
                if(NULL != ts_ch->enc_para)
                {
                    kfree(ts_ch->enc_para);
					ts_ch->enc_para = NULL;
                    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
				}					
            }
            else 
            {
                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

                ret = -EFAULT;
            }
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
            
            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                ret = -EFAULT;
            }
            
            sec_ch = &(ch->detail.sec_ch);
            
            ret = dmx_sec_flt_unregister(sec_ch->sec_flt_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
            }

			/* Need Discrambling?
			*/
			if (1 == usr_para->sec_param.needdiscramble)
			{
				dmx_ts_flt_unregister(sec_ch->ts_flt_id2see);
				
				sed_delete_scramble_pid(0, (UINT32)dev, usr_para->sec_param.pid);
			}

            if (DMX_CHANNEL_STATE_RUN == ch->state)
            {
                dmx_data_buf_flush_all_pkt(&ch->data_buf);
            }
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
            
            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
        
                ret = -EFAULT;
            }
            
            pes_ch = &(ch->detail.pes_ch);
            
            ret = dmx_pes_flt_unregister(pes_ch->pes_flt_id);
            
            if (ret < 0)
            {
                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
            }

			/* Need Discrambling?
			*/
			if (1 == usr_para->pes_param.needdiscramble)
			{
				dmx_ts_flt_unregister(pes_ch->ts_flt_id2see);
				
				sed_delete_scramble_pid(0, (UINT32)dev, usr_para->pes_param.pid);
			}
		
            if (DMX_CHANNEL_STATE_RUN == ch->state)
            {
                dmx_data_buf_flush_all_pkt(&ch->data_buf);
            }
        }

        break;


        case DMX_CHANNEL_OUTPUT_FORMAT_PCR:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

            if (DMX_OUTPUT_SPACE_KERNEL != usr_para->output_space)
            {
                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

                ret = -EFAULT;
            }
            else
            {

				if((dev->src_hw_interface_id == ALI_SWDMX0_OUTPUT_HWIF_ID) || (dev->src_hw_interface_id == ALI_SWDMX1_OUTPUT_HWIF_ID))
				{
					break;
				}
				
                pcr_ch = &(ch->detail.pcr_ch);
                
                ret = dmx_pcr_flt_unregister(pcr_ch->pcr_flt_id);
                
                if (ret < 0)
                {
                    DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
                }
            }
        }
        break;


        default:
        {
            DMX_CH_LEGA_DBG("%s,line:%d,format:%d\n", __FUNCTION__, __LINE__, usr_para->output_format);
            
            ret = -EINVAL;
        }
        break;
    }

    ch->state = DMX_CHANNEL_STATE_IDLE;

    dmx_mutex_output_unlock(dev->src_hw_interface_id);

    return(ret);

}


__s32 dmx_channel_read
(
    struct file *file,
    char __user *buf,
    size_t       count,
    loff_t      *ppos
)
{
    __s32                     ret;
    struct dmx_output_device *dev;
    struct dmx_channel       *ch; 
    struct dmx_channel_param *usr_para;

    DMX_CH_LEGA_DBG("%s, %d\n", __FUNCTION__, __LINE__);

    ch = file->private_data;

    dev = (struct dmx_output_device *)ch->dmx_output_device;

    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
    {
        return(-ERESTARTSYS);
    }

    ret = 0;

    usr_para = &ch->usr_param;

    switch(usr_para->output_format)
    {
        //DMX_CH_LEGA_DBG("%s, %d\n", __FUNCTION__, __LINE__);

        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            //DMX_CH_LEGA_DBG("%s, %d\n", __FUNCTION__, __LINE__);
            if(ali_dmx_channel_module_legacy.m_dmx_record_mode == DMX_REC_OUT_BLOCK)
            {
            	ret = dmx_channel_ts_block_read(dev, ch, file->f_flags, buf, count);
            }
			else
			{
            	ret = dmx_channel_ts_read(dev, ch, file->f_flags, buf, count);
			}
        }
        break; 

        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            DMX_CH_LEGA_DBG("%s(), %d,DMX_CHANNEL_OUTPUT_FORMAT_SEC \n", __FUNCTION__, __LINE__);
            
            ret = dmx_channel_sec_read(dev, ch, file->f_flags, buf, count);
			
            DMX_CH_LEGA_DBG("%s(), %d, DMX_CHANNEL_OUTPUT_FORMAT_SEC \n", __FUNCTION__, __LINE__);
        }
        break; 

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
        {
            DMX_CH_LEGA_DBG("%s(), %d, DMX_CHANNEL_OUTPUT_FORMAT_PES \n", __FUNCTION__, __LINE__);
            
            ret = dmx_channel_pes_read(dev, ch, file->f_flags, buf, count);
			
            DMX_CH_LEGA_DBG("%s(), %d, DMX_CHANNEL_OUTPUT_FORMAT_PES\n", __FUNCTION__, __LINE__);
        }
        break; 


        default:
        {
            DMX_CH_LEGA_DBG("%s, %d\n", __FUNCTION__, __LINE__);
            
            ret = -EPERM;
        }
        break;  
    }

    dmx_mutex_output_unlock(dev->src_hw_interface_id);

    DMX_CH_LEGA_DBG("%s, %d\n", __FUNCTION__, __LINE__);

    return(ret);
}


__s32 dmx_channel_add_pid
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    struct dmx_ts_channel     *ts_ch;
    struct dmx_channel_param  *save_usr_para;
	struct dmx_channel_param   modify_usr_para;
    __s32                      ret;
    __u32                      i;
    __u32                      j;
	__u32                      k;
	__u32  src_hw_interface_id;
	
    /* ch state validation.
    */
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(-EPERM);
    }

            //DMX_CH_LEGA_DBG("%s,%d\n",__FUNCTION__,__LINE__);
    save_usr_para = &(ch->usr_param);

    ret = copy_from_user(&modify_usr_para, (void __user *)arg, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    switch(save_usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

            ts_ch = &(ch->detail.ts_ch);

            if (DMX_OUTPUT_SPACE_USER == save_usr_para->output_space)
            {
                //DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
				
				if(0 == save_usr_para->rec_whole_tp)
                {
                    for (i = 0; i < modify_usr_para.ts_param.pid_list_len; i++)
                    {
                    	dmx_dbg_ch_legacy_api_show("%s,%d,PID:%d\n", __FUNCTION__, __LINE__,
							                       modify_usr_para.ts_param.pid_list[i]);

						for (k = 0; k < save_usr_para->ts_param.pid_list_len; k++)
						{
							if(save_usr_para->ts_param.pid_list[k] == modify_usr_para.ts_param.pid_list[i])
								break;
						}

						if(k < save_usr_para->ts_param.pid_list_len)
						{
	                    	dmx_dbg_ch_legacy_api_show("%s,%d,exist PID:%d\n", __FUNCTION__, __LINE__,
								                       modify_usr_para.ts_param.pid_list[i]);							
							continue;
						}
						
						save_usr_para->ts_param.pid_list[save_usr_para->ts_param.pid_list_len] = modify_usr_para.ts_param.pid_list[i];
						save_usr_para->ts_param.needdiscramble[save_usr_para->ts_param.pid_list_len] = modify_usr_para.ts_param.needdiscramble[i];

						if(ali_dmx_channel_module_legacy.m_dmx_record_mode == DMX_REC_OUT_BLOCK)
						{
	                        ts_ch->ts_flt_id[save_usr_para->ts_param.pid_list_len] = dmx_ts_flt_register(dev->src_hw_interface_id,
					                                                  modify_usr_para.ts_param.pid_list[i], 
					                                                  dmx_channel_ts_block_wr, (__u32)ch);						
						}
						else
						{
							if (1 != modify_usr_para.ts_param.needdiscramble[i])
							{
								src_hw_interface_id = dev->src_hw_interface_id;
							}
							else
							{							
								sed_add_scramble_pid(0, (UINT32)dev, modify_usr_para.ts_param.pid_list[i]);
							
								ts_ch->ts_flt_id2see[save_usr_para->ts_param.pid_list_len] = dmx_ts_flt_register(dev->src_hw_interface_id, modify_usr_para.ts_param.pid_list[i], 
																				dmx_see_buf_wr_ts,
																				dev->src_hw_interface_id);
								
								if (ts_ch->ts_flt_id2see[save_usr_para->ts_param.pid_list_len] < 0)
								{
									dmx_dbg_ch_legacy_api_show("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ts_ch->ts_flt_id2see[save_usr_para->ts_param.pid_list_len]);

									for (j = 0; j < (save_usr_para->ts_param.pid_list_len); j++)
									{
										dmx_ts_flt_unregister(ts_ch->ts_flt_id[j]);
										if (1 == save_usr_para->ts_param.needdiscramble[j])
										{
											dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[j]);
										}
									}
							
									return(-EMFILE);
								}
							
								src_hw_interface_id = ALI_SEETOMAIN_BUF_HWIF_ID;
							}
								
	                        ts_ch->ts_flt_id[save_usr_para->ts_param.pid_list_len] = dmx_ts_flt_register(src_hw_interface_id,
																		modify_usr_para.ts_param.pid_list[i], 
																		dmx_channel_ts_wr, (__u32)ch);							
						}
						
                        if (ts_ch->ts_flt_id[save_usr_para->ts_param.pid_list_len] < 0)
                        {
                            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
                            
                            for (j = 0; j < (save_usr_para->ts_param.pid_list_len); j++)
                            {
                                dmx_ts_flt_unregister(ts_ch->ts_flt_id[j]);
								if ((ali_dmx_channel_module_legacy.m_dmx_record_mode != DMX_REC_OUT_BLOCK)
									&& (1 == save_usr_para->ts_param.needdiscramble[j]))
								{
									dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[j]);
								}								
                            }
                        
                            return(-EMFILE);
                        }
                        else
                        {
							dmx_ts_flt_start(ts_ch->ts_flt_id[save_usr_para->ts_param.pid_list_len]);
							if ((ali_dmx_channel_module_legacy.m_dmx_record_mode != DMX_REC_OUT_BLOCK)
									&& (1 == save_usr_para->ts_param.needdiscramble[save_usr_para->ts_param.pid_list_len]))
							{
					        	dmx_ts_flt_start(ts_ch->ts_flt_id2see[save_usr_para->ts_param.pid_list_len]);
							}							
                        }    

						save_usr_para->ts_param.pid_list_len++;
                    }
				}
            }
            else 
            {
                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }
        }
        break;

        default:
        {
            return(-EINVAL);
        }
        break;
    }

    return(0);
}




__s32 dmx_channel_del_pid
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg    
)
{
    __u32                     i;
	__u32                     j;
	__u32                     k;
    __s32                     ret;
    struct dmx_ts_channel    *ts_ch;
    struct dmx_channel_param *save_usr_para;
	struct dmx_channel_param modify_usr_para;
	struct dmx_ts_flt		 *ts_flt;

    DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    /* ch state validation, return success in all state.
    */
    if (ch->state != DMX_CHANNEL_STATE_RUN)
    {
        return(0);
    }

    save_usr_para = &ch->usr_param;

    ret = copy_from_user(&modify_usr_para, (void __user *)arg, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    switch(save_usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            ts_ch = &(ch->detail.ts_ch);
            
			if (DMX_OUTPUT_SPACE_USER == save_usr_para->output_space)
            {
				if(0 == save_usr_para->rec_whole_tp)
				{
				    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
                    for (i = 0; i < modify_usr_para.ts_param.pid_list_len; i++)
                    {
						for (j = 0; j < save_usr_para->ts_param.pid_list_len; j++)
						{
							if(save_usr_para->ts_param.pid_list[j] == modify_usr_para.ts_param.pid_list[i])
								break;
						}

						if(j >= save_usr_para->ts_param.pid_list_len)
						{
	                    	dmx_dbg_ch_legacy_api_show("%s,%d,not exist PID:%d\n", __FUNCTION__, __LINE__,
								                       modify_usr_para.ts_param.pid_list[i]);							
							continue;
						}
					
    					ts_flt = &ali_dmx_ts_flt_module.ts_flt[ts_ch->ts_flt_id[j]];
    
    					memset(&ts_flt->stat_info, 0, sizeof(ts_flt->stat_info));
    
                        dmx_ts_flt_stop(ts_ch->ts_flt_id[j]);

						if (1 == save_usr_para->ts_param.needdiscramble[j])
						{
							sed_delete_scramble_pid(0, (UINT32)dev, ts_ch->param.PidList[j]);
							
				        	dmx_ts_flt_stop(ts_ch->ts_flt_id2see[j]);
						}

						for (k = j; k < (save_usr_para->ts_param.pid_list_len - 1); k++)
						{
							ts_ch->ts_flt_id[k] = ts_ch->ts_flt_id[k+1];
							ts_ch->ts_flt_id2see[k] = ts_ch->ts_flt_id2see[k+1];

							save_usr_para->ts_param.pid_list[k] = save_usr_para->ts_param.pid_list[k+1];
							save_usr_para->ts_param.needdiscramble[k] = save_usr_para->ts_param.needdiscramble[k+1];												
						}

						save_usr_para->ts_param.pid_list_len--;
                    }
				}	
            }
            else 
            {
                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

                return(-EFAULT);
            }
        }
        break;

        default:
        {
            DMX_CH_LEGA_DBG("%s,line:%d,format:%d\n", __FUNCTION__, __LINE__, save_usr_para->output_format);
            
            ret = -EINVAL;
        }
        break;
    }

    return(0);
}


__s32 dmx_channel_rec_dsc_deen_info
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                      ret;
	struct dmx_record_deencrypt_info i_dmx_rec_dsc_deen_info;	
	DEEN_CONFIG *i_deen_config = NULL;
#ifdef CONFIG_DSC_LEGACY_IOCTL
	struct ca_session_attr decrypt_ca_session_attr;
#endif
#ifdef CONFIG_ALI_PVR_RPC
	struct pvr_ca_attr encrypt_ca_session_attr;
#endif
#ifdef CONFIG_ALI_SEC
	struct sec_ca_attr sec_ca_session_attr;
#endif

    /* ch state validation. */
    if (ch->state != DMX_CHANNEL_STATE_CFG)
    {
        return(-EPERM);
    }

    ret = copy_from_user(&i_dmx_rec_dsc_deen_info, (void __user *)arg, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
	
	ch->m_p_dmx_dsc_deen_param = (void *)kmalloc(sizeof(DEEN_CONFIG), GFP_KERNEL);

    if(NULL == ch->m_p_dmx_dsc_deen_param)
    {
        DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, -EFAULT);
    
        return(-EFAULT);
	}						
	
	i_deen_config = (DEEN_CONFIG *)(ch->m_p_dmx_dsc_deen_param);
	memset(i_deen_config, 0, sizeof(DEEN_CONFIG));

#ifdef CONFIG_DSC_LEGACY_IOCTL
#ifdef CONFIG_ALI_SEC
      if(sec_check_is_sec_fd((__s32)(i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.decrypt_fd)))
      {
             memset(&sec_ca_session_attr,0,sizeof(sec_ca_session_attr));
      	    	if((i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.decrypt_fd >= 0) 
  		&& (sec_get_session_attr((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.decrypt_fd, &sec_ca_session_attr) >= 0))
            {
          		i_deen_config->do_decrypt = 1;
          		i_deen_config->dec_dev = sec_ca_session_attr.sub_dev_see_hdl;
          		i_deen_config->Decrypt_Mode = sec_ca_session_attr.crypt_mode;
          		i_deen_config->dec_dmx_id = sec_ca_session_attr.stream_id;
  	      }
      }
      else
#endif
      {
	if((i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.decrypt_fd >= 0) 
		&& (ca_dsc_get_session_attr((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.decrypt_fd, &decrypt_ca_session_attr) >= 0))
	{
		i_deen_config->do_decrypt = 1;
		i_deen_config->dec_dev = decrypt_ca_session_attr.sub_dev_see_hdl;
		i_deen_config->Decrypt_Mode = decrypt_ca_session_attr.crypt_mode;
		i_deen_config->dec_dmx_id = decrypt_ca_session_attr.stream_id;
	}
      }
#ifdef CONFIG_ALI_PVR_RPC
	memset(&encrypt_ca_session_attr,0,sizeof(encrypt_ca_session_attr));
    memset(&decrypt_ca_session_attr,0,sizeof(decrypt_ca_session_attr));
    if(pvr_check_is_pvr_fd((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd))
    {
		if((i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd >= 0) 
			&& (pvr_get_session_attr((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd, &encrypt_ca_session_attr) >= 0))
		{
			i_deen_config->do_encrypt = 1;
			i_deen_config->enc_dev = encrypt_ca_session_attr.sub_dev_see_hdl;
			i_deen_config->Encrypt_Mode = encrypt_ca_session_attr.crypt_mode;
			i_deen_config->enc_dmx_id = encrypt_ca_session_attr.stream_id;
		}
    }
#ifdef CONFIG_ALI_SEC
    else if(sec_check_is_sec_fd((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd))
    {
             memset(&sec_ca_session_attr,0,sizeof(sec_ca_session_attr));
    	    	if((i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd >= 0) 
		&& (sec_get_session_attr((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd, &sec_ca_session_attr) >= 0))
	{
		i_deen_config->do_encrypt = 1;
		i_deen_config->enc_dev = sec_ca_session_attr.sub_dev_see_hdl;
		i_deen_config->Encrypt_Mode = sec_ca_session_attr.crypt_mode;
		i_deen_config->enc_dmx_id = sec_ca_session_attr.stream_id;
	}
    }
#endif
    else
    {
        if((i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd >= 0)
        && (ca_dsc_get_session_attr((__s32)i_dmx_rec_dsc_deen_info.m_dmx_dsc_fd.encrypt_fd, &decrypt_ca_session_attr) >= 0))
        {
            i_deen_config->do_encrypt = 1;
            i_deen_config->enc_dev = decrypt_ca_session_attr.sub_dev_see_hdl;
            i_deen_config->Encrypt_Mode = decrypt_ca_session_attr.crypt_mode;
            i_deen_config->enc_dmx_id = decrypt_ca_session_attr.stream_id;
        }                             
    }
#endif/* CONFIG_ALI_PVR_RPC */
#endif

	i_dmx_rec_dsc_deen_info.m_dmx_dsc_id = (__u32)ch;

	if((copy_to_user((void __user *)arg, &i_dmx_rec_dsc_deen_info, _IOC_SIZE(cmd)) != 0))
	{
		if(NULL != ch->m_p_dmx_dsc_deen_param)
		{
			kfree(ch->m_p_dmx_dsc_deen_param);
			ch->m_p_dmx_dsc_deen_param = NULL;
		}
		return -EFAULT;
	}
	
	ch->m_dmx_dsc_id = (__u32)ch;

    return(0);
}


__s32 dmx_tp_autoscan_task
(
    void *param
)
{
    __s32                     ret;
    struct dmx_tp_autoscan   *autoscan;
	__s32                     byte_cnt;

    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

	autoscan = &(ali_dmx_channel_module_legacy.autoscan);
	
    for(;;)
    {
        msleep(5);

        if (dmx_mutex_output_lock(0))
        {
            return(-ERESTARTSYS);
        }
    	
		if (DMX_TPSCAN_STATE_RUN == autoscan->state)
		{
    		if (autoscan->tp_scan_buf_wr < (660 * 188))
    		{
                byte_cnt = dmx_data_buf_rd((void *)(autoscan->tp_scan_buf + autoscan->tp_scan_buf_wr), &(autoscan->tp_data_buf), 660 * 188, 
                						   DMX_DATA_CPY_DEST_KERN);             
                if (byte_cnt <= 0)
                {
                	//DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d\n", __FUNCTION__, __LINE__, byte_cnt);
                }
    			else
    			{
                    autoscan->tp_scan_buf_wr += byte_cnt;
                	//DMX_CH_LEGA_DBG("%s,%d,tp_scan_buf_wr:%d\n", __FUNCTION__, __LINE__, autoscan->tp_scan_buf_wr);				
    			}
    		}	
		}
		
		dmx_mutex_output_unlock(0);
    }

	return ret;
}


__s32 dmx_tp_autoscan_task_create(void)
{
    __s32 ret = 0;
	struct task_struct *p = NULL;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};

	p = kthread_create(dmx_tp_autoscan_task, NULL, "autoscan");

	if (IS_ERR(p))
	{
		return(PTR_ERR(p));	
	}

	sched_setscheduler(p, SCHED_RR, &param);

	wake_up_process(p);
	
    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

	return ret;
}


__s32 dmx_tp_autoscan_write
(
    struct dmx_ts_pkt_inf *pkt_inf,
    __u32                  param
)
{
    __u32                            data_len;
    __s32                            byte_cnt;
    struct ali_dmx_data_buf         *dest_buf;
    struct dmx_tp_autoscan          *autoscan;

    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
	
	autoscan = (struct dmx_tp_autoscan *)param;

    dest_buf = &(autoscan->tp_data_buf);

    byte_cnt = dmx_data_buf_wr_data(dest_buf, pkt_inf->pkt_addr, 188, 
                                    DMX_DATA_SRC_TYPE_KERN);

	if (byte_cnt < 0)
	{		
        DMX_CH_LEGA_DBG("%s,%d,wr len:%d,buf len:%d,ret:%d,pkt_inf:%x\n",
               __FUNCTION__, __LINE__, 188, 
               dest_buf->cur_len, byte_cnt, (__u32)pkt_inf);

        /* Must be buffer overflow, flush all section data of this stream 
         * to free memory.
		 */
		dmx_data_buf_flush_all_pkt(dest_buf);		
	}

    #if 0
    /* Wakeup waiting poll only if we are storing into an empty buffer,
     * to offload CPU loading.
    */
    if (data_len <= 0)
    {
        //DMX_CH_LEGA_DBG("%s,%d,data_len:%d\n", __FUNCTION__, __LINE__, data_len);
    	
        wake_up_interruptible(&(dest_buf->rd_wq));
    }
	#endif

    data_len = dmx_data_buf_total_len(dest_buf);

    //DMX_CH_LEGA_DBG("%s,%d,data_len:%d\n", __FUNCTION__, __LINE__, data_len);

    return(byte_cnt);
}




__s32 dmx_tp_autoscan_read
(
    void
)
{
    __s32                   byte_cnt;
    struct dmx_tp_autoscan *autoscan;
	
    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

	autoscan = &(ali_dmx_channel_module_legacy.autoscan);
    	
    byte_cnt = dmx_data_buf_rd((void *)autoscan->tp_scan_buf, &(autoscan->tp_data_buf), 660 * 188, 
    						   DMX_DATA_CPY_DEST_KERN); 
    
    if (byte_cnt < 0)
    {
    	DMX_CH_LEGA_DBG("%s,%d,byte_cnt:%d\n", __FUNCTION__, __LINE__, byte_cnt);
    }

    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    return(byte_cnt / 188);
}




__s32 dmx_tp_autoscan_stop
(
    void
)
{
    __s32                   ret;
    struct dmx_tp_autoscan *autoscan;
	
    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

	autoscan = &(ali_dmx_channel_module_legacy.autoscan);

    if (autoscan->state != DMX_TPSCAN_STATE_RUN)
    {
        return(-EPERM);
    }		
    
    ret = dmx_pid_flt_stop(autoscan->tp_filter_id);

    if (ret < 0)
    {
        DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

        return(ret);
    }

    ret = dmx_pid_flt_unregister(autoscan->tp_filter_id);

    if (ret < 0)
    {
        DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

        return(ret);
    }
	
    dmx_data_buf_flush_all_pkt(&(autoscan->tp_data_buf));

    autoscan->state = DMX_TPSCAN_STATE_IDLE;
	
    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
	
	return(0);
}




__s32 dmx_tp_autoscan_start
(
    struct dmx_output_device *dev,
    struct dmx_channel       *ch,
    unsigned int              cmd,
    unsigned long             arg
)
{
    __s32                     ret;
    struct dmx_tp_autoscan   *autoscan;
	//__s32                     byte_cnt;

	autoscan = &(ali_dmx_channel_module_legacy.autoscan);

    //DMX_PRINTK("%s,%d,autoscan state:%d\n", __FUNCTION__, __LINE__, autoscan->state);

	if(autoscan->state == DMX_TPSCAN_STATE_CREATE_TASK)
	{
		DMX_PRINTK("dmx_tp_autoscan_task_create\n");
	
		dmx_tp_autoscan_task_create();

		autoscan->state = DMX_TPSCAN_STATE_IDLE;
	}

    if (autoscan->state != DMX_TPSCAN_STATE_IDLE)
    {
        return(-EPERM);
    }	
	
    autoscan->tp_scan_buf = arg;
	autoscan->tp_scan_buf_wr = 0;
	
    dmx_data_buf_list_init(&(autoscan->tp_data_buf));
    
    autoscan->tp_filter_id = dmx_pid_flt_register(dev->src_hw_interface_id, DMX_WILDCARD_PID, 
    							                  dmx_tp_autoscan_write, (__u32)(autoscan));
    
    if (autoscan->tp_filter_id < 0)
    {        
    	DMX_CH_LEGA_DBG("%s,%d,tp_filter_id:%d\n", __FUNCTION__, __LINE__, autoscan->tp_filter_id);
    
    	return(autoscan->tp_filter_id);
    }	

    ret = dmx_pid_flt_start(autoscan->tp_filter_id);

    if (ret < 0)
    {
        DMX_API_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);

        return(ret);
    }

    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);

    autoscan->state = DMX_TPSCAN_STATE_RUN;
		
	return(0);
}


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
long dmx_channel_ioctl
(
    struct file  *filp,
    unsigned int  cmd,
    unsigned long arg
)
#else
__s32 dmx_channel_ioctl
(
    struct inode *inode,
    struct file  *filp,
    unsigned int  cmd,
    unsigned long arg
)
#endif
{
    struct dmx_channel          *ch; 
    struct dmx_output_device    *dev;
    __s32                        ret;
    struct Ali_DmxSeeStatistics  see_statistics;

    ret = 0;

    ch = filp->private_data;

    dev = (struct dmx_output_device *)ch->dmx_output_device;

    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
    {
        return(-ERESTARTSYS);
    }

    //DMX_CH_LEGA_DBG("%s,%d,cmd:%x\n", __FUNCTION__, __LINE__, cmd);

    switch(cmd)
    {
        case ALI_DMX_CHANNEL_START:
        {
            ret = dmx_channel_start(dev, ch, cmd, arg);
        }
        break;

        case ALI_DMX_CHANNEL_STOP:
        {
            ret = dmx_channel_stop(dev, ch);
        }
        break;

        case ALI_DMX_CHANNEL_GET_CUR_PKT_LEN:
        {
            ret = dmx_channel_get_pkt_len(dev, ch, cmd, arg);
        }
        break;

        case ALI_DMX_HW_GET_FREE_BUF_LEN:
        {
            ret = dmx_hw_buf_free_len(dev, ch, cmd, arg);
        }
        break;

        case ALI_DMX_GET_PROG_BITRATE:
        {
            ret = dmx_bitrate_get(dev, ch, cmd, arg);
        }
        break;

        case ALI_DMX_IO_SET_BYPASS_MODE:
        {
			ret = dmx_tp_autoscan_start(dev, ch, cmd, arg);
        }
        break;
        
        case ALI_DMX_IO_BYPASS_GETDATA:
        {		
			ret = dmx_tp_autoscan_read();
        }
        break;	
        
        case ALI_DMX_IO_CLS_BYPASS_MODE:
        {	
			ret = dmx_tp_autoscan_stop();
        }
        break;
              
        case ALI_DMX_BYPASS_ALL:
        {
            DMX_CH_LEGA_DBG("%s,%d,arg:%x\n", __FUNCTION__, __LINE__, (__s32)arg);
        }
        break;
		
        case ALI_DMX_RESET_BITRATE_DETECT:
        {

        }
        break;
		
        case ALI_DMX_SET_HW_INFO:
        {

        }
        break;

		/* Add for sat2ip
		*  TODO:implement
		*/
        case ALI_DMX_CHANNEL_ADD_PID:
        {
            ret = dmx_channel_add_pid(dev, ch, cmd, arg);
            break;
            return RET_SUCCESS;              
        }
        break;

		/* Add for sat2ip
		*  TODO:implement
		*/		
        case ALI_DMX_CHANNEL_DEL_PID:
        {
            ret = dmx_channel_del_pid(dev, ch, cmd, arg);
            break;
            return RET_SUCCESS;    			
        }			
        break;


        /* All below commnad are Debug statistics, not direcly related to 
         * legacy commands.
		*/
#if 0
        case ALI_DMX_GET_LATEST_PCR:
        {
             latest_pcr = dmx_pcr_get_latest(dmx);
             
             copy_to_user((void *)arg, &latest_pcr, sizeof(UINT32));
             
             break;
        }
#endif

        case ALI_DMX_SEE_GET_STATISTICS:
        {            
             dmx_see_get_statistics(&see_statistics);
             
             ret = copy_to_user((void *)arg, &see_statistics, 
                                sizeof(struct Ali_DmxSeeStatistics));

             if (ret != 0)
             {
                 DMX_API_DBG("%s, line:%d\n", __FUNCTION__, __LINE__);
         
                 return(-EFAULT);
             }
             
             break;
        }

        case ALI_DMX_CHAN_KERN_GLB_CFG:
        {
			ret = dmx_channel_kern_glb_cfg(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_KERN_GLB_START:
        {
			ret = dmx_channel_kern_glb_start(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_KERN_GLB_STOP:
        {
			ret = dmx_channel_kern_glb_stop(dev, ch);
            
			break;
        }
        
        case ALI_DMX_CHAN_KERN_GLB_INFO_GET:
        {
			ret = dmx_channel_kern_glb_info_get(dev, ch, cmd, arg);
            
			break;
        }
		
        case ALI_DMX_CHAN_KERN_GLB_REALTIME_SET:
        {
			ret = dmx_channel_kern_glb_realtime_set(dev, ch, cmd, arg);
            
			break;
        }

        case ALI_DMX_CHAN_SEE_GLB_CFG:
        {
			ret = dmx_channel_see_glb_cfg(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_SEE_GLB_START:
        {
			ret = dmx_channel_see_glb_start(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_SEE_GLB_STOP:
        {
			ret = dmx_channel_see_glb_stop(dev, ch);
            
			break;
        }
        
        case ALI_DMX_CHAN_SEE_GLB_INFO_GET:
        {
			ret = dmx_channel_see_glb_info_get(dev, ch, cmd, arg);
            
			break;
        }

        case ALI_DMX_CHAN_SEE_GLB_REALTIME_SET:
        {
			ret = dmx_channel_see_glb_realtime_set(dev, ch, cmd, arg);
            
			break;
        }

        case ALI_DMX_CHAN_HW_REG_CFG:
        {
			ret = dmx_channel_hw_reg_cfg(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_HW_REG_START:
        {
			ret = dmx_channel_hw_reg_start(dev, ch);
            
			break;
        }

        case ALI_DMX_CHAN_HW_REG_STOP:
        {
			ret = dmx_channel_hw_reg_stop(dev, ch);
            
			break;
        }
        
        case ALI_DMX_CHAN_HW_REG_INFO_GET:
        {
			ret = dmx_channel_hw_reg_info_get(dev, ch, cmd, arg);
            
			break;
        }

        case ALI_DMX_CHAN_TS_INFO_GET:
        {
            ret = dmx_channel_ts_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_TS_FILTER_INFO_GET:
        {
            ret = dmx_channel_ts_filter_info_get(dev, ch, cmd, arg);
        }
        break;

        case ALI_DMX_CHAN_SEC_TS_FILTER_INFO_GET:
        {
            ret = dmx_channel_sec_ts_filter_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_SEC_INFO_GET:
        {
            ret = dmx_channel_sec_stream_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_SEC_FILTER_INFO_GET:
        {
            ret = dmx_channel_sec_filter_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_PES_TS_FILTER_INFO_GET:
        {
            ret = dmx_channel_pes_ts_filter_info_get(dev, ch, cmd, arg);

            break;
        }
 
        case ALI_DMX_CHAN_PES_INFO_GET:
        {
            ret = dmx_channel_pes_stream_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_PES_FILTER_INFO_GET:
        {
            ret = dmx_channel_pes_filter_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_VIDEO_INFO_GET:
        {
			ret = dmx_channel_video_stream_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_VIDEO_FILTER_INFO_GET:
        {
            ret = dmx_channel_video_filter_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_VIDEO_SEE_INFO_GET:
        {
			ret = dmx_channel_video_see_info_get(dev, ch, cmd, arg);

			break;
        }

        case ALI_DMX_CHAN_AUDIO_INFO_GET:
        {
			ret = dmx_channel_audio_stream_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_AUDIO_FILTER_INFO_GET:
        {
            ret = dmx_channel_audio_filter_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_AUDIO_SEE_INFO_GET:
        {
			ret = dmx_channel_audio_see_info_get(dev, ch, cmd, arg);

			break;
        }

        case ALI_DMX_CHAN_PCR_INFO_GET:
        {
            ret = dmx_channel_pcr_info_get(dev, ch, cmd, arg);

            break;
        }

        case ALI_DMX_CHAN_PCR_FILTER_INFO_GET:
        {
            ret = dmx_channel_pcr_filter_info_get(dev, ch, cmd, arg);

            break;
        }

		case ALI_DMX_CHAN_STATUS_RESET:
		{
            ret = dmx_channel_status_reset(dev, ch);

            break;			
		}

        case ALI_DMX_CACHE_SET:
        {
			struct Ali_DmxCacheParam dmx_cache_param;
            __u16 pid_buf[DMX_CACHE_PID_LIST_MAX_LEN];
            __u32 cnt;
			
			ret = copy_from_user(&dmx_cache_param, (void __user *)arg, _IOC_SIZE(cmd));
			
			if (0 != ret)
			{
				DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
			
				return(-EFAULT);
			}

            for (cnt = 0; cnt < DMX_CACHE_PID_LIST_MAX_LEN; cnt++)
            {
                pid_buf[cnt] = 0x1FFF;
            }

            if (dmx_cache_param.pid_list != NULL && dmx_cache_param.pid_list_len <= DMX_CACHE_PID_LIST_MAX_LEN
            &&  dmx_cache_param.pid_list_len != 0
            &&  dmx_cache_param.type == DMX_CACHE_PID)
            {
                ret = copy_from_user(&pid_buf[0],
                        (void __user *)(dmx_cache_param.pid_list),
                        (dmx_cache_param.pid_list_len) * sizeof(__u16));

    			if (0 != ret)
    			{
    				DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

    				return(-EFAULT);
    			}
            }
            dmx_cache_param.pid_list = &pid_buf[0];

			ret = dmx_data_engine_cache_set(dev->src_hw_interface_id, &dmx_cache_param);
			
            break;
        }

        case ALI_DMX_HW_BUF_CLEAN:
        {
			ret = dmx_data_engine_cache_hwbuf_reset(dev->src_hw_interface_id);
            break;
        }	
		
		case ALI_DMX_CACHE_RETRACE_SET:
		{
			struct Ali_DmxCacheRetraceParam dmx_cache_retrace_param;
			
			ret = copy_from_user(&dmx_cache_retrace_param, (void __user *)arg, _IOC_SIZE(cmd));
			
			if (0 != ret)
			{
				DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
			
				return(-EFAULT);
			}

			ret = dmx_data_engine_cache_retrace_set(dev->src_hw_interface_id, &dmx_cache_retrace_param);
			break;
		}
		
		case ALI_DMX_CACHE_RETRACE_START:
		{
			ret = dmx_data_engine_cache_retrace_start(dev->src_hw_interface_id);
			break;
		}

		case ALI_DMX_IO_SET_FTA_REC_MODE:
		{
			ali_dmx_channel_module_legacy.dmx_clearstream_encrypt_mode = (enum Ali_Dmx_ClearStreamEncryptMode)arg;
			break;
		}

		case ALI_DMX_IO_GET_FTA_REC_MODE:
		{
			if((copy_to_user((void __user *)arg, &ali_dmx_channel_module_legacy.dmx_clearstream_encrypt_mode, _IOC_SIZE(cmd)) != 0))
			{
				return -EFAULT;
			}	
			break;
		}

		case ALI_DMX_RECORD_MODE_SET:
		{
			ali_dmx_channel_module_legacy.m_dmx_record_mode = (enum Ali_DmxRecordMode)arg;
			break;
		}

		case ALI_DMX_RECORD_BLOCKSIZE_SET:
		{
			if(((__u32)arg * 2) > DMX_DEENCRYPT_BUF_LEN)
			{
				return -EINVAL;
			}
			
			if(ali_dmx_channel_module_legacy.m_dmx_record_mode == DMX_REC_OUT_BLOCK)
			{
				ali_dmx_channel_module_legacy.m_dmx_record_blocksize = (__u32)arg;
			}
			break;
		}

		case ALI_DMX_RECORD_MODE_GET:
		{
			if((copy_to_user((void __user *)arg, &ali_dmx_channel_module_legacy.m_dmx_record_mode, _IOC_SIZE(cmd)) != 0))
			{
				return -EFAULT;
			}
			
			break;
		}

		case ALI_DMX_RECORD_BLOCKSIZE_GET:
		{
			if((copy_to_user((void __user *)arg, &ali_dmx_channel_module_legacy.m_dmx_record_blocksize, _IOC_SIZE(cmd)) != 0))
			{
				return -EFAULT;
			}			
			break;
		}

		case ALI_DMX_RECORD_DEENCRYPT_INFO:
		{
			ret = dmx_channel_rec_dsc_deen_info(dev, ch, cmd, arg);			
			break;
		}
		
        default: 
        {
			DMX_CH_LEGA_DBG("%s,%d,cmd:%x\n", __FUNCTION__, __LINE__, cmd);

            ret = -ENOTTY;
        }
        break;
    }

    //DMX_CH_LEGA_DBG("%s,%d,cmd:%x\n", __FUNCTION__, __LINE__, cmd);

    dmx_mutex_output_unlock(dev->src_hw_interface_id);

    return(ret);
}



__u32 dmx_channel_poll
(
    struct file              *filp,
    struct poll_table_struct *wait
)
{
    __s32                     mask;
    struct dmx_channel       *ch; 
    struct dmx_output_device *dev;
    struct dmx_channel_param *usr_para;

    ch = filp->private_data;

	if (DMX_CHANNEL_STATE_RUN != ch->state)
	{
        DMX_CH_LEGA_DBG("%s,%d,state:%d\n", __FUNCTION__, __LINE__, ch->state);

        return(0);
	}	

    dev = (struct dmx_output_device *)ch->dmx_output_device;

    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
    {
        return(0);
    }

    usr_para = &ch->usr_param;
	mask = 0;

    switch(usr_para->output_format)
    {
        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
        {
            mask = dmx_channel_sec_poll(filp, wait, dev, ch);
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
        {
            mask = dmx_channel_pes_poll(filp, wait, dev, ch);
        }
        break;

        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
        {
            mask = dmx_channel_ts_poll(filp, wait, dev, ch);
        }
        break;

        default:
        {
            DMX_CH_LEGA_DBG("%s,line:%d,format:%d\n", __FUNCTION__, __LINE__, usr_para->output_format);

            //mask = -EINVAL;
        }
        break;
    }

    dmx_mutex_output_unlock(dev->src_hw_interface_id);

    return(mask);
}





extern unsigned long __G_ALI_MM_DMX_MEM_SIZE;
extern unsigned long __G_ALI_MM_DMX_MEM_START_ADDR;

__s32 dmx_channel_module_legacy_init
(
    void
)
{
    __u32 i;

    for (i = 0; i < DMX_CHANNEL_CNT; i++)
    {
        ali_dmx_channel_module_legacy.channel[i].state = DMX_CHANNEL_STATE_IDLE;
    }

	ali_dmx_channel_module_legacy.dmx_de_enc_input_buf = (__G_ALI_MM_DMX_MEM_START_ADDR + __G_ALI_MM_DMX_MEM_SIZE - (2 * DMX_SEE_BUF_SIZE) - DMX_DEENCRYPT_BUF_LEN);
	ali_dmx_channel_module_legacy.dmx_de_enc_output_buf = ali_dmx_channel_module_legacy.dmx_de_enc_input_buf - DMX_DEENCRYPT_BUF_LEN;

	ali_dmx_channel_module_legacy.dmx_clearstream_encrypt_mode = FTA_TO_FTA;
	ali_dmx_channel_module_legacy.m_dmx_record_mode = DMX_REC_OUT_TS;
	ali_dmx_channel_module_legacy.m_dmx_record_blocksize = DMX_DEFAULT_REC_BLOCK_SIZE;
	
    //DMX_CH_LEGA_DBG("%s,%d\n", __FUNCTION__, __LINE__);
	
    return(0);
}


__s32 dmx_channel_module_legacy_deinit
(
    void
)
{
    __u32 i;
	__s32                     ret;
    struct dmx_channel       *ch;
    struct dmx_ts_channel    *ts_ch;
    struct dmx_sec_channel   *sec_ch;
    struct dmx_pes_channel   *pes_ch;
    struct dmx_pcr_channel   *pcr_ch;
	struct dmx_channel_param *usr_para;
    struct dmx_output_device *dev;

    for (i = 0; i < DMX_CHANNEL_CNT; i++)
    {
		if(ali_dmx_channel_module_legacy.channel[i].state != DMX_CHANNEL_STATE_IDLE)
		{
			ch = &ali_dmx_channel_module_legacy.channel[i];
		
		    dev = (struct dmx_output_device *)ch->dmx_output_device;

		    if (dmx_mutex_output_lock(dev->src_hw_interface_id))
		    {
		        continue;
		    }

		    if (ch->state == DMX_CHANNEL_STATE_CFG)
		    {
			    ch->state = DMX_CHANNEL_STATE_IDLE;

			    dmx_mutex_output_unlock(dev->src_hw_interface_id);
				
		        continue;
		    }

		    ret = 0;
		    
		    usr_para = &(ch->usr_param);

		    switch(usr_para->output_format)
		    {
		        case DMX_CHANNEL_OUTPUT_FORMAT_TS:
		        {
		            ts_ch = &(ch->detail.ts_ch);
		            
		            if (DMX_OUTPUT_SPACE_KERNEL == usr_para->output_space)
		            {
		                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

		                ret = dmx_ts_flt_unregister(ts_ch->ts_flt_id[0]);
		                
		                if (ret < 0)
		                {
		                    DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		                }
		            }
		            else if (DMX_OUTPUT_SPACE_USER == usr_para->output_space)
		            {
		                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

						if(1 == usr_para->rec_whole_tp)
						{
						    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
		                    dmx_pid_flt_unregister(ts_ch->ts_flt_id[0]);
						}				
		                else
		                {
		                    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
		                    for (i = 0; i < usr_para->ts_param.pid_list_len; i++)
		                    {
		                        ret = dmx_ts_flt_unregister(ts_ch->ts_flt_id[i]);
		    
		                        if (ret < 0)
		                        {
		                            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		                        }

								/* Need Discrambling?
								*/
								if (1 == usr_para->ts_param.needdiscramble[i])
								{
						        	dmx_ts_flt_unregister(ts_ch->ts_flt_id2see[i]);
									
								    sed_delete_scramble_pid(0, (UINT32)dev, usr_para->ts_param.pid_list[i]);
								}						
		                    }
						}

		                if (DMX_CHANNEL_STATE_RUN == ch->state)
		                {
		                    dmx_data_buf_flush_all_pkt(&ch->data_buf);
							dmx_data_buf_flush_all_pkt(&ch->data_buf_orig);
							ch->data_buf_orig_have_scrambler = 0;
							ch->data_buf_orig_update_last_time.tv_sec = 0;
							ch->data_buf_orig_update_last_time.tv_usec = 0;					
		                }
						
		                if(NULL != ts_ch->enc_para)
		                {
		                    kfree(ts_ch->enc_para);
							ts_ch->enc_para = NULL;
		                    dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);
						}					
		            }
		            else 
		            {
		                dmx_dbg_ch_legacy_api_show("%s,%d\n", __FUNCTION__, __LINE__);

		                ret = -EFAULT;
		            }
		        }
		        break;

		        case DMX_CHANNEL_OUTPUT_FORMAT_SEC:
		        {
		            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		            
		            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
		            {
		                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

		                ret = -EFAULT;
		            }
		            
		            sec_ch = &(ch->detail.sec_ch);
		            
		            ret = dmx_sec_flt_unregister(sec_ch->sec_flt_id);
		            
		            if (ret < 0)
		            {
		                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
		            }

		            if (DMX_CHANNEL_STATE_RUN == ch->state)
		            {
		                dmx_data_buf_flush_all_pkt(&ch->data_buf);
		            }
		        }
		        break;

		        case DMX_CHANNEL_OUTPUT_FORMAT_PES:
		        {
		            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		            
		            if (DMX_OUTPUT_SPACE_USER != usr_para->output_space)
		            {
		                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);
		        
		                ret = -EFAULT;
		            }
		            
		            pes_ch = &(ch->detail.pes_ch);
		            
		            ret = dmx_pes_flt_unregister(pes_ch->pes_flt_id);
		            
		            if (ret < 0)
		            {
		                DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
		            }

					/* Need Discrambling?
					*/
					if (1 == usr_para->pes_param.needdiscramble)
					{
						dmx_ts_flt_unregister(pes_ch->ts_flt_id2see);
						
						sed_delete_scramble_pid(0, (UINT32)dev, usr_para->pes_param.pid);
					}
				
		            if (DMX_CHANNEL_STATE_RUN == ch->state)
		            {
		                dmx_data_buf_flush_all_pkt(&ch->data_buf);
		            }
		        }

		        break;


		        case DMX_CHANNEL_OUTPUT_FORMAT_PCR:
		        {
		            DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

		            if (DMX_OUTPUT_SPACE_KERNEL != usr_para->output_space)
		            {
		                DMX_CH_LEGA_DBG("%s,line:%d\n", __FUNCTION__, __LINE__);

		                ret = -EFAULT;
		            }
		            else
		            {
						if((dev->src_hw_interface_id == ALI_SWDMX0_OUTPUT_HWIF_ID) || (dev->src_hw_interface_id == ALI_SWDMX1_OUTPUT_HWIF_ID))
						{
							break;
						}		
						
		                pcr_ch = &(ch->detail.pcr_ch);
		                
		                ret = dmx_pcr_flt_unregister(pcr_ch->pcr_flt_id);
		                
		                if (ret < 0)
		                {
		                    DMX_CH_LEGA_DBG("%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
		                }
		            }
		        }
		        break;


		        default:
		        {
		            DMX_CH_LEGA_DBG("%s,line:%d,format:%d\n", __FUNCTION__, __LINE__, usr_para->output_format);
		            
		            ret = -EINVAL;
		        }
		        break;
		    }

		    ch->state = DMX_CHANNEL_STATE_IDLE;

		    dmx_mutex_output_unlock(dev->src_hw_interface_id);
		}
    }
	
    return(0);
}


#endif

