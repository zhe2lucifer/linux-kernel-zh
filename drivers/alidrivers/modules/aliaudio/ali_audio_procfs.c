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

/****************************************************************************(I)(S)
 *  File: ali_audio_procfs.c
 *  (I)
 *  Description: debug fs for aliaudio
 *  (S)
 *  History:(M)
 *  Version     Date        		Author         		Comment
 *  ======   	=======        	======			=======
 * 	0.		2016.10.12		william			Create
 ****************************************************************************/
#include <linux/module.h>

#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/poll.h>

#include <linux/ali_rpc.h>
#include <ali_audio_common.h>
#include "ali_m36_audio_rpc.h"


/*Proc based contron intertace*/
#define AUDIO_DEBUG_PROC_DIR "aliaudio"
#define AUDIO_DEBUG_PROC_INFO "debuginfo"
#define MAX_BUF_WT_LEN 200 //do not bigger than one page size 1024 bytes
#define MAX_BUF_RD_LEN 2048

static struct proc_dir_entry *audio_proc_dir = NULL;
static struct proc_dir_entry *audio_proc_dbginfo_file = NULL;

static struct deca_device *audio_deca_dev=NULL;
static struct snd_device *audio_snd_dev=NULL;

static char *audio_info_buffer = NULL;
static __u32 g_dbg_show_en = 0;
static __u32 g_dbg_show_interval = 3;
static 	struct mutex audio_dbg_mutex;
wait_queue_head_t audio_dbg_wq;
static 	struct task_struct *audio_dbg_show_thread_ptr;

static inline char *audio_state(unsigned long state)
{
    char *ret = NULL;
    switch(state)
    {
        case DECA_STATE_DETACH:
            ret="DETACH";
            break;
        case DECA_STATE_ATTACH:
            ret="ATTACH";
            break;
        case DECA_STATE_IDLE:
            ret="IDLE";
            break;
        case DECA_STATE_PLAY:
            ret="PLAY";
            break;
        case DECA_STATE_PAUSE:
            ret="PAUSE";
            break;
        default:
            ret="UNKNOWN";
            break;
    }
    return ret;
}

static inline char *chan_state(unsigned long state)
{
    char *ret = NULL;
    switch(state)
    {
        case SND_DUP_NONE:
            ret="L/R";
            break;
        case SND_DUP_L:
            ret="L/L";
            break;
        case SND_DUP_R:
            ret="R/R";
            break;
        default:
            ret="UNKNOWN";
            break;
    }
    return ret;
}

static inline char *decoder_support(unsigned int supported)
{
    char *ret = NULL;
    switch(supported)
    {
        case 0:
            ret="support";
            break;
        case 1:
            ret="NOT support";
            break;
        case 2:
            ret="OTP bit not setted";
            break;
        case 3:
            ret="signature verify fail";
            break;
        default:
            ret="UNKNOWN";
            break;
    }
    return ret;
}
#define INTF_SPDIF  1
#define INTF_HDMI   2
static inline char *output_type(unsigned int intf, unsigned int type)
{
    char *ret = NULL;
    switch(type)
    {
        case 0:
            ret="PCM";
            break;
        case 1:
            ret="DD";
            break;
        case 2:
            ret=(INTF_HDMI==intf)?"BS":"UNKNOWN";
            break;
        case 3:
            ret=(INTF_HDMI==intf)?"AUTO":"UNKNOWN";
            break;
        default:
            ret="UNKNOWN";
            break;
    }
    return ret;
}
static inline char *audio_sub_state(unsigned long state)
{
    char *ret = NULL;
    switch(state)
    {
        case 0:
            ret="NORMAL";
            break;
        case DECA_SUB_STATE_BUSY:
            ret="NORMAL";
            break;
        case DECA_SUB_STATE_NO_DATA:
            ret="NO DATA";
            break;
        case DECA_SUB_STATE_NO_BUFF:
            ret="NO BUFF";
            break;
        default:
            ret="UNKNOWN";
            break;
    }
    return ret;
}

static inline char *audio_type(unsigned int str_type)
{
    char *ret = NULL;
    switch(str_type)
    {
        case AUDIO_INVALID:
            ret = "INVALID";
            break;
        case AUDIO_MPEG1:
            ret = "MPEG1";
            break;                
        case AUDIO_MPEG2:
            ret = "MPEG2";
            break;
        case AUDIO_MPEG_AAC:
            ret = "AAC_LATM";
            break;
        case AUDIO_AC3:
            ret = "AC3";
            break;
        case AUDIO_DTS:
            ret = "DTS";
            break;
        case AUDIO_PPCM:        //Packet PCM for DVD-Audio
            ret = "Packet_PCM";
            break;
        case AUDIO_LPCM_V:      //Linear PCM audio for DVD-Video
            ret = "LPCM_V";
            break;
        case AUDIO_LPCM_A:      //Linear PCM audio for DVD-Audio
            ret = "LPCM_A";
            break;
        case AUDIO_PCM:
            ret = "PCM";
            break;
        case AUDIO_BYE1:
            ret = "BYE1";
            break;
        case AUDIO_RA8:          //Real audio 8
            ret = "REAL_AUDIO_8";
            break;
        case AUDIO_MP3:          //MP3 audio
            ret = "MP3";
            break;
        case AUDIO_MPEG_ADTS_AAC:
            ret = "AAC_ADTS";
            break;
        case AUDIO_OGG:
            ret = "OGG";
            break;
        case AUDIO_EC3:
            ret = "EAC3";
            break;
        case AUDIO_MP3_L3:       //for TS MPEG Layer 3
            ret = "MP3_L3";
            break;
    	case AUDIO_MP3_2:
            ret = "MP3_2";
            break;
    	case AUDIO_MP2_2:
            ret = "MP2_2";
            break;
    	case AUDIO_PCM_RAW:      //PCM DECODE PARAMS SET BY APP
            ret = "PCM_RAW";
            break;
        case AUDIO_DTSHD:
            ret = "DTSHD";
            break;
        case AUDIO_DOLBY_PULSE:
            ret = "DOLBY_PULSE";
            break;
        case AUDIO_ADPCM:
            ret = "ADPCM";
            break;
        case AUDIO_APE:
            ret = "APE";
            break;
        case AUDIO_FLAC:
            ret = "FLAC";
            break;
        case AUDIO_BYE1PRO:
            ret = "BYE1PRO";
            break;
        case AUDIO_VORBIS:
            ret = "VORBIS";
            break;
        case AUDIO_AMR:
            ret = "AMR";
            break;
        case AUDIO_ALAC:
            ret = "ALAC";
            break;
        case AUDIO_STREAM_TYPE_END:
        default:
            ret = "INVALID";
            break;
    }

    return ret;
}
static int audio_read_debug_info(char * buffer)
{
	int len = 0;
    unsigned long len_max = MAX_BUF_RD_LEN - 100;
    struct audio_dbg_info dbg_info;
    struct cur_stream_info deca_play_info;
    unsigned int temp = 0xff; 

	if(!audio_deca_dev || !audio_snd_dev)
	{
		return -EUNATCH;
	}

    if (RET_SUCCESS == deca_io_control(audio_deca_dev, \
        DECA_GET_DBG_INFO, (UINT32)(&dbg_info)))
    {           
		len += sprintf(&buffer[len],"\ndeca state         : %s(%d)\n", \
            audio_state(dbg_info.deca.state), (int)dbg_info.deca.state);
        if (len_max <= len)
            goto out;

		len += sprintf(&buffer[len],"deca sub state     : %s(%d)\n", \
            audio_sub_state(dbg_info.deca.sub_state), (int)dbg_info.deca.sub_state);
        if (len_max <= len)
            goto out;
        
		len += sprintf(&buffer[len],"audio desc enable  : %s\n", \
            (1==dbg_info.snd.ad_en)?"yes":"no");
        if (len_max <= len)
            goto out;
        
        if (RET_SUCCESS == deca_io_control(audio_deca_dev, \
            DECA_GET_PLAY_PARAM, (UINT32)(&deca_play_info)))
        {
    		len += sprintf(&buffer[len],"format             : %s", \
                audio_type(deca_play_info.str_type));
            if (len_max <= len)
                goto out;      

            temp = (unsigned int)deca_play_info.str_type;
            if (RET_SUCCESS == deca_io_control(audio_deca_dev, \
                DECA_GET_AUDIO_SUPPORT_STATUS, (UINT32)(&temp)))
            {
        		len += sprintf(&buffer[len],"(%s)\n", decoder_support(temp));
            }
            else
            {
        		len += sprintf(&buffer[len],"\n");
            }
            if (len_max <= len)
                goto out;      
        
    		len += sprintf(&buffer[len],"sample rate        : %d\n", \
                (int)deca_play_info.sample_rate);
            if (len_max <= len)
                goto out;

            // (4 == dbg_info.deca.state)
            { 
        		len += sprintf(&buffer[len],"sync success/fail  : %d/%d\n", \
                    (int)deca_play_info.sync_success_cnt, (int)deca_play_info.sync_error_cnt);
                if (len_max <= len)
                    goto out;
                
        		len += sprintf(&buffer[len],"decode success/fail: %d/%d\n", \
                    (int)deca_play_info.decode_success_cnt, (int)deca_play_info.decode_error_cnt);
                if (len_max <= len)
                    goto out;
                
        		len += sprintf(&buffer[len],"current pts        : 0x%x \n", \
                    (int)deca_play_info.cur_frm_pts);
                if (len_max <= len)
                    goto out;
            }
        }

		len += sprintf(&buffer[len],"snd  state         : %s(%d)\n", \
            audio_state(dbg_info.snd.state), (int)dbg_info.snd.state);
        if (len_max <= len)
            goto out;

		len += sprintf(&buffer[len],"snd  sub state     : %s(%d)\n", \
            audio_sub_state(dbg_info.snd.sub_state), (int)dbg_info.snd.sub_state);
        if (len_max <= len)
            goto out;

		len += sprintf(&buffer[len],"volume             : %d\n", \
            snd_get_volume(audio_snd_dev));
        if (len_max <= len)
            goto out;
        
        if (RET_SUCCESS == snd_io_control(audio_snd_dev, SND_IO_GET_MUTE_STATE, (UINT32)&temp))
        {
            len += sprintf(&buffer[len],"mute enable        : %s\n", \
                (1==temp)?"yes":"no");
            if (len_max <= len)
                goto out;
        }
        
        if (RET_SUCCESS == snd_io_control(audio_snd_dev, SND_IO_GET_CHAN_STATE, (UINT32)&temp))
        {
            len += sprintf(&buffer[len],"channel state      : %s\n", \
                chan_state(temp));
            if (len_max <= len)
                goto out;
        }
        
        if (RET_SUCCESS == snd_io_control(audio_snd_dev, SND_IO_SPO_INTF_CFG_GET, (UINT32)&temp))
        {
            len += sprintf(&buffer[len],"spdif output type  : %s\n", \
                output_type(INTF_SPDIF, temp));
            if (len_max <= len)
                goto out;
        }
        if (RET_SUCCESS == snd_io_control(audio_snd_dev, SND_IO_DDP_SPO_INTF_CFG_GET, (UINT32)&temp))
        {
            len += sprintf(&buffer[len],"hdmi  output type  : %s\n", \
                output_type(INTF_HDMI, temp));
            if (len_max <= len)
                goto out;
        }

        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n");
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"|BUFF |PCM(HEX BYTE)     |DESC(HEX BYTE)    |DD(HEX BYTE)      |DDP(HEX BYTE)     |\n");
        if (len_max <= len)
            goto out;

        /* deca buff info */
        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n");
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"|BS   |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |                  |                  |\n",
            (int)((dbg_info.deca.prog_bs_buff_rm*100)/(dbg_info.deca.prog_bs_buff_len)),
            (unsigned int)dbg_info.deca.prog_bs_buff_rm, (unsigned int)dbg_info.deca.prog_bs_buff_len,
            (int)((dbg_info.deca.desc_bs_buff_rm*100)/(dbg_info.deca.desc_bs_buff_len)),
            (unsigned int)dbg_info.deca.desc_bs_buff_rm, (unsigned int)dbg_info.deca.desc_bs_buff_len);
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n");
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"|CB   |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |                  |                  |\n",
            (int)((dbg_info.deca.prog_cb_buff_rm*100)/(dbg_info.deca.prog_cb_buff_len)),
            (unsigned int)dbg_info.deca.prog_cb_buff_rm, (unsigned int)dbg_info.deca.prog_cb_buff_len,
            (int)((dbg_info.deca.desc_cb_buff_rm*100)/(dbg_info.deca.desc_cb_buff_len)),
            (unsigned int)dbg_info.deca.desc_cb_buff_rm, (unsigned int)dbg_info.deca.desc_cb_buff_len);
        if (len_max <= len)
            goto out;

        /* snd buff info */
        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n");
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"|SYNC |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |\n",
            (int)((dbg_info.snd.sync_buff_pcm_rm*100)/(dbg_info.snd.sync_buff_pcm_len)),
            (unsigned int)dbg_info.snd.sync_buff_pcm_rm, (unsigned int)dbg_info.snd.sync_buff_pcm_len,
            (int)((dbg_info.snd.sync_buff_desc_pcm_rm*100)/(dbg_info.snd.sync_buff_desc_pcm_len)),
            (unsigned int)dbg_info.snd.sync_buff_desc_pcm_rm, (unsigned int)dbg_info.snd.sync_buff_desc_pcm_len,
            (int)((dbg_info.snd.sync_buff_dd_rm*100)/(dbg_info.snd.sync_buff_dd_len)),
            (unsigned int)dbg_info.snd.sync_buff_dd_rm, (unsigned int)dbg_info.snd.sync_buff_dd_len,
            (int)((dbg_info.snd.sync_buff_ddp_rm*100)/(dbg_info.snd.sync_buff_ddp_len)),
            (unsigned int)dbg_info.snd.sync_buff_ddp_rm, (unsigned int)dbg_info.snd.sync_buff_ddp_len        );
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n");
        if (len_max <= len)
            goto out;
        len += sprintf(&buffer[len],"|DMA  |%03d%%(%05x/%05x) |                  |%03d%%(%05x/%05x) |%03d%%(%05x/%05x) |\n",
            (int)((dbg_info.snd.dma_buff_pcm_rm*100)/(dbg_info.snd.dma_buff_pcm_len)),
            (unsigned int)dbg_info.snd.dma_buff_pcm_rm, (unsigned int)dbg_info.snd.dma_buff_pcm_len,
            (int)((dbg_info.snd.dma_buff_dd_rm*100)/(dbg_info.snd.dma_buff_dd_len)),
            (unsigned int)dbg_info.snd.dma_buff_dd_rm, (unsigned int)dbg_info.snd.dma_buff_dd_len,
            (int)((dbg_info.snd.dma_buff_ddp_rm*100)/(dbg_info.snd.dma_buff_ddp_len)),
            (unsigned int)dbg_info.snd.dma_buff_ddp_rm, (unsigned int)dbg_info.snd.dma_buff_ddp_len        );
        if (len_max <= len)
            goto out;

        len += sprintf(&buffer[len],"+-----+------------------+------------------+------------------+------------------+\n\n");
        if (len_max <= len)
            goto out;
    }

out:
	return len;
}

static int audio_show_debug_info(void)
{
    int cnt = 5;
    int len = 0;
    int pos = 0;
    int output_len = 0;
    
	if(!audio_deca_dev || !audio_snd_dev)
	{
		return -EUNATCH;
	}
    
    printk(KERN_ERR"\n");
	memset(audio_info_buffer, 0, MAX_BUF_RD_LEN);
    len = audio_read_debug_info(audio_info_buffer);
	if (0 < len)
	{
        while ((cnt > 0) && (pos < len))
        {
            cnt --;
            output_len = printk(KERN_ERR"%s", audio_info_buffer+pos);
            if (output_len <= 0)
            {
                break;
            }
            pos += output_len;
        }
        return 0;
	}
    
	return -EFAULT;
}

/*Process debug info*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static ssize_t audio_dbginfo_procfile_read(struct file *file, char __user *ubuf, size_t size, loff_t *ppos)
{
	int len = 0;
	ssize_t ret_len = 0;

	if(audio_info_buffer)
	{
    	memset(audio_info_buffer, 0, MAX_BUF_RD_LEN);
    	len = audio_read_debug_info(audio_info_buffer);
        if (0 < len)
        {
        	ret_len = simple_read_from_buffer(ubuf, size, ppos, audio_info_buffer, len);
        }
	}

	return ret_len;
}


static ssize_t audio_dbginfo_procfile_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char            buf[MAX_BUF_WT_LEN] = {0};
    char            *eq_ch = NULL;
    char            *endp = NULL;
    unsigned long   value = 0;

	if ((0>=count) || (MAX_BUF_WT_LEN<count))
        return 0;

	if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    eq_ch = strstr(buf, "=");
    if (NULL == eq_ch)
    {
		printk(KERN_ERR "param error: incorrect value: %s \n", buf);
		return -EINVAL;
	}

    value = simple_strtoul((char *)(eq_ch+1), &endp, 0);
	if ((eq_ch+1) == endp || value >= INT_MAX)
    {
		printk(KERN_ERR "param error: incorrect value: %s \n", (eq_ch+1));
		return -EINVAL;
	}

    switch(*buf)
    {
        case 'a':
        {
            if (strstr(buf, "ad_en"))
            {
                if (0==value || 1==value)
                {
                    if (RET_SUCCESS != snd_io_control(audio_snd_dev, SND_SET_AD_DYNAMIC_EN, (UINT32)value))
                    {
                        printk("\033[40;31m%s->%s.%u, set ad_en(%d) fail!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, (int)value);
                        return -EFAULT;
                    }
                }
                else
                {
            		printk(KERN_ERR "param error: incorrect value: %d\n", (int)value);
            		return -EINVAL;
                }
            }
            else
            {
        		printk(KERN_ERR "param error: incorrect value: %s\n", buf);
        		return -EINVAL;
            }
            break;
        }
        case 'm':
        {
            if (strstr(buf, "monitor"))
            {
                if (0==value || 1==value)
                {
                    g_dbg_show_en = value;
                    if (g_dbg_show_en)
                    {
                        if (mutex_lock_interruptible(&audio_dbg_mutex))
                        {
                            return(-ERESTARTSYS);
                        }
                        wake_up_interruptible(&audio_dbg_wq);
                    	mutex_unlock(&audio_dbg_mutex);
                    }
                }
                else
                {
            		printk(KERN_ERR "param error: incorrect value: %d\n", (int)value);
            		return -EINVAL;
                }
            }
            else
            {
        		printk(KERN_ERR "param error: incorrect value: %s\n", buf);
        		return -EINVAL;
            }
            break;
        }
        case 'i':
        {
            if (strstr(buf, "interval"))
            {
                if (0<value && 100>value)
                {
                    g_dbg_show_interval = value;
                }
                else
                {
                    printk(KERN_ERR "param error: incorrect value: %d\n", (int)value);
                    return -EINVAL;
                }
            }
            else
            {
                printk(KERN_ERR "param error: incorrect value: %s\n", buf);
                return -EINVAL;
            }
            break;
        }
        default:
    		printk(KERN_ERR "param error: incorrect value: %s\n", buf);
            return -EINVAL;
    }


	#if 0 /* Modified by William.Zeng on 2016-10-14 18:57 */
	if ((1 != sscanf(buf, "ad_en=%d", &ad_en)) && (0==ad_en || 1==ad_en))
	{
		return 0;
	}
    if (RET_SUCCESS != snd_io_control(audio_snd_dev, SND_SET_AD_DYNAMIC_EN, (enum adec_desc_channel_enable)ad_en))
    {
        printk("\033[40;31m%s->%s.%u, set ad_en(%d) fail!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, ad_en);
        return 0;
    }
    printk("\033[40;31m%s->%s.%u, set ad_en(%d) success!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, ad_en);
	#endif /* #if 0, End of Modified by William.Zeng on 2016-10-14 18:57 */

	return count;
}
#else
static int audio_dbginfo_procfile_read(char*buffer, char**buffer_localation, off_t offset,int buffer_length,int* eof, void *data )
{
	int len = 0;
	len =  audio_read_debug_info(buffer);
	*eof = 1;
    return len;
}

static int audio_dbginfo_procfile_write(struct file *filp, const char *buffer,unsigned long count,void *data)
{
    char            buf[MAX_BUF_WT_LEN] = {0};
    char            *eq_ch = NULL;
    char            *endp = NULL;
    unsigned long   value = 0;

    if ((0>=count) || (MAX_BUF_WT_LEN<count))
        return 0;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    eq_ch = strstr(buf, "=");
    if (NULL == eq_ch)
    {
        printk(KERN_ERR "param error: incorrect value: %s\n", buf);
        return -EINVAL;
    }

    value = simple_strtoul((char *)(eq_ch+1), &endp, 0);
    if ((eq_ch+1) == endp || value >= INT_MAX)
    {
        printk(KERN_ERR "param error: incorrect value: %s\n", (eq_ch+1));
        return -EINVAL;
    }

    switch(*buf)
    {
        case 'a':
            if (strstr(buf, "ad_en"))
            {
                if (0==value || 1==value)
                {
                    if (RET_SUCCESS != snd_io_control(audio_snd_dev, SND_SET_AD_DYNAMIC_EN, (UINT32)value))
                    {
                        printk("\033[40;31m%s->%s.%u, set ad_en(%d) fail!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, (int)value);
                        return -EFAULT;
                    }
                }
                else
                {
                    printk(KERN_ERR "param error: incorrect value: %d\n", (int)value);
                    return -EINVAL;
                }
            }
            else
            {
                printk(KERN_ERR "param error: incorrect value: %s\n", buf);
                return -EINVAL;
            }
            break;
        case 'm':
            if (strstr(buf, "monitor"))
            {
                if (0==value || 1==value)
                {
                    g_dbg_show_en = value;
                    if (g_dbg_show_en)
                    {
                        if (mutex_lock_interruptible(&audio_dbg_mutex))
                        {
                            return(-ERESTARTSYS);
                        }
                        wake_up_interruptible(&audio_dbg_wq);
                        mutex_unlock(&audio_dbg_mutex);
                    }
                }
                else
                {
                    printk(KERN_ERR "param error: incorrect value: %d\n", (int)value);
                    return -EINVAL;
                }
            }
            else
            {
                printk(KERN_ERR "param error: incorrect value: %s\n", buf);
                return -EINVAL;
            }
            break;
        default:
            printk(KERN_ERR "param error: incorrect value: %s\n", buf);
            return -EINVAL;
    }


#if 0 /* Modified by William.Zeng on 2016-10-14 18:57 */
    if ((1 != sscanf(buf, "ad_en=%d", &ad_en)) && (0==ad_en || 1==ad_en))
    {
        return 0;
    }
    if (RET_SUCCESS != snd_io_control(audio_snd_dev, SND_SET_AD_DYNAMIC_EN, (enum adec_desc_channel_enable)ad_en))
    {
        printk("\033[40;31m%s->%s.%u, set ad_en(%d) fail!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, ad_en);
        return 0;
    }
    printk("\033[40;31m%s->%s.%u, set ad_en(%d) success!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, ad_en);
#endif /* #if 0, End of Modified by William.Zeng on 2016-10-14 18:57 */

    return count;
}


#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static const struct file_operations aliaudio_debuginfo_fops = {
	.read = audio_dbginfo_procfile_read,
	.write = audio_dbginfo_procfile_write,
	.llseek = default_llseek,
};
#endif

static int audio_dbg_show_thread(void *param)
{
    __s32 ret;

    for(;;)
    {
        if (mutex_lock_interruptible(&audio_dbg_mutex))
        {
            return(-ERESTARTSYS);
        }

        /* Wait until we are allowed to show debug info.
		*/
		while (g_dbg_show_en == 0)
		{
            mutex_unlock(&audio_dbg_mutex);
	        if (wait_event_interruptible(audio_dbg_wq, (1==g_dbg_show_en)))
            {
                return(-ERESTARTSYS);
            }
            if (mutex_lock_interruptible(&audio_dbg_mutex))
            {
                return(-ERESTARTSYS);
            }
		}

	    ret = audio_show_debug_info();

		if (ret < 0)
		{
            g_dbg_show_en = 0;
            printk(KERN_ERR"\033[40;31m%s->%s.%u, audio_show_debug_info failed!.\033[0m\n", __FILE__, __FUNCTION__, __LINE__);
		}
        mutex_unlock(&audio_dbg_mutex);

        msleep(g_dbg_show_interval*1000);
    }

	return(0);
}

int  audio_debug_procfs_init(void)
{
    audio_info_buffer = kmalloc(MAX_BUF_RD_LEN, GFP_KERNEL);
    if (NULL == audio_info_buffer)
    {
        printk("kmall audio_info_buffer %d failed!!\n", MAX_BUF_RD_LEN);
        return -1;
    }
	mutex_init(&audio_dbg_mutex);
	init_waitqueue_head(&audio_dbg_wq);

    audio_proc_dir = proc_mkdir(AUDIO_DEBUG_PROC_DIR, NULL);
    if (audio_proc_dir == NULL) {
        printk("audio_debug_procfs_init create dir aliaudio failed!!\n");
        kfree(audio_info_buffer);
        return -1;
    }

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0))
    /*For Debug info*/
    audio_proc_dbginfo_file = proc_create(AUDIO_DEBUG_PROC_INFO,0644,audio_proc_dir, &aliaudio_debuginfo_fops);
#else
    /*For Debug info*/
    audio_proc_dbginfo_file = create_proc_entry(AUDIO_DEBUG_PROC_INFO,0644,audio_proc_dir);
#endif
    if(audio_proc_dbginfo_file == NULL)
    {
        remove_proc_entry(AUDIO_DEBUG_PROC_DIR, NULL);
        kfree(audio_info_buffer);
        printk("Error:could not initialize /proc/%s/%s\n", AUDIO_DEBUG_PROC_DIR, AUDIO_DEBUG_PROC_INFO);
        return -1;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
    audio_proc_dbginfo_file->read_proc = audio_dbginfo_procfile_read;
    audio_proc_dbginfo_file->write_proc = audio_dbginfo_procfile_write;
#endif

	audio_deca_dev=(struct deca_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_DECA);
	audio_snd_dev=(struct snd_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_SND);

	audio_dbg_show_thread_ptr = kthread_create(audio_dbg_show_thread, NULL, "audio_dbg");

	if (IS_ERR(audio_dbg_show_thread_ptr))
	{
	    printk("%s,%d\n", __FUNCTION__, __LINE__);
        remove_proc_entry(AUDIO_DEBUG_PROC_INFO, audio_proc_dir);
        remove_proc_entry(AUDIO_DEBUG_PROC_DIR, NULL);
        kfree(audio_info_buffer);
		return(PTR_ERR(audio_dbg_show_thread_ptr));
	}

	wake_up_process(audio_dbg_show_thread_ptr);

    return 0;
}


void  audio_debug_procfs_exit(void)
{
    remove_proc_entry(AUDIO_DEBUG_PROC_INFO, audio_proc_dir);
    remove_proc_entry(AUDIO_DEBUG_PROC_DIR, NULL);

    if (audio_info_buffer)
        kfree(audio_info_buffer);

    return ;
}
