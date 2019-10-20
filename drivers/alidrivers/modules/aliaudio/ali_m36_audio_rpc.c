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

#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/poll.h>
//#include <linux/byteorder/swabb.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#include <linux/firmware.h>
#endif
#include <linux/crc32.h>
#include <linux/slab.h>

#include <ali_cache.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <ali_soc.h>

//#include <ali_rpcng.h>

//#include <asm/system.h>
//#include <asm/semaphore.h>

#include "ali_m36_audio_rpc.h"
#include <ali_shm.h> // add by jacket 2013.7.17
#include <ali_board_config.h> // add by jacket 2013.10.23
#include "ali_audio_info.h"

/*add for exporting sys_open and sys_close on otv5 by kinson*/
#include <linux/syscalls.h>
/*add for send the message from kernel space to user space*/
#include <linux/ali_transport.h>

volatile unsigned long *g_ali_audio_rpc_arg[MAX_AUDIO_RPC_ARG_NUM];
volatile unsigned long *g_ali_audio_rpc_tmp;
volatile int g_ali_audio_rpc_arg_size[MAX_AUDIO_RPC_ARG_NUM];

static struct semaphore m_audio_sem;

static int ali_m36_audio_open(struct inode *inode, struct file *file);
static int ali_m36_audio_release(struct inode *inode, struct file *file);
static ssize_t ali_m36_audio_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long ali_m36_audio_ioctl(struct file *file, unsigned int cmd, unsigned long parg);
#else
static int ali_m36_audio_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long parg);
#endif
static int ali_m36_audio_mmap(struct file *file, struct vm_area_struct *vma);

/******************************************************************************
 * driver registration
 ******************************************************************************/

static struct file_operations ali_m36_audio_fops = {
	.owner		= THIS_MODULE,
	.write		= ali_m36_audio_write,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_m36_audio_ioctl,
#else
	.ioctl		= ali_m36_audio_ioctl,
#endif
	.mmap		= ali_m36_audio_mmap,
	.open		= ali_m36_audio_open,
	.release	=  ali_m36_audio_release,
	//.poll		= dvb_audio_poll,
};

struct ali_audio_device ali_m36_audio_dev;

struct class *ali_m36_audio_class;
struct device *ali_m36_audio_dev_node;

static int m_audio_pause = 0;
static int m_audio_start = 0;
static int deca_stop_in_kernel = 0;

static void hld_deca_rpc_release(int force)
{
	if((m_audio_start == 1) || (force == 1))
	{
		deca_stop(ali_m36_audio_dev.deca_dev, 0, ADEC_STOP_IMM);
		snd_stop(ali_m36_audio_dev.snd_dev);

		deca_decore_ioctl(ali_m36_audio_dev.deca_dev,DECA_DECORE_RLS,NULL,NULL);

		m_audio_start = 0;
		if(force == 0)
		{
			deca_stop_in_kernel = 1;
		}
	}
}

void ali_deca_rpc_release(void)
{
	down(&m_audio_sem);

	hld_deca_rpc_release(0);

	up(&m_audio_sem);
}

static RET_CODE audio_rpc_operation(struct ali_audio_device *dev,UINT32 ID)
{
    RET_CODE ret = 0;

    switch(ID)
    {
/*multi args RPC calling*/
       case AUDIO_SND_GET_STC:
                ret = get_stc((UINT32 *)g_ali_audio_rpc_arg[0],*(UINT8 *) g_ali_audio_rpc_arg[1]);
                break;
       case AUDIO_SND_SET_STC:
                set_stc(*(UINT32 *)g_ali_audio_rpc_arg[0],*(UINT8 *) g_ali_audio_rpc_arg[1]);
                break;
       case AUDIO_SND_PAUSE_STC:
                stc_pause(*(UINT8 *)g_ali_audio_rpc_arg[0],*(UINT8 *) g_ali_audio_rpc_arg[1]);
                break;
	case AUDIO_DECA_IO_COMMAND_ADV:
 		   deca_io_control(dev->deca_dev,*(UINT32 *)g_ali_audio_rpc_arg[0], (UINT32)g_ali_audio_rpc_arg[1]);
                break;
       case AUDIO_SND_IO_COMMAND_ADV:
 		 snd_io_control(dev->snd_dev,*(UINT32 *)g_ali_audio_rpc_arg[0], (UINT32)g_ali_audio_rpc_arg[1]);
              break;
       case RPC_AUDIO_DECORE_IOCTL:
	   	if(*(int *)(g_ali_audio_rpc_arg[0]) == DECA_DECORE_INIT)
	   	{
			hld_deca_rpc_release(0);
			m_audio_pause = 0;
			m_audio_start = 1;
		}
		else if(*(int *)(g_ali_audio_rpc_arg[0]) == DECA_DECORE_PAUSE_DECODE)
		{
			if(*(int *)g_ali_audio_rpc_arg[1])
				m_audio_pause = 1;
			else
				m_audio_pause = 0;

			//pr_debug("<0>""%s : pause flag %d\n", __FUNCTION__, m_audio_pause);
		}
		else if(*(int *)(g_ali_audio_rpc_arg[0]) == DECA_DECORE_RLS)
		{
			m_audio_pause = 0;
			m_audio_start = 0;
		}

        ret = deca_decore_ioctl(ali_m36_audio_dev.deca_dev,*(int *)g_ali_audio_rpc_arg[0], (void *)g_ali_audio_rpc_arg[1],(void *) g_ali_audio_rpc_arg[2]);

        break;
       case AUDIO_DECA_PROCESS_PCM_SAMPLES:
             {
                UINT8 *tmp_buf = NULL;

				if (*(UINT32 *)g_ali_audio_rpc_arg[0] > __G_ALI_MM_DECA_MEM_SIZE)
				{
    			    return RET_FAILURE;
				}

				tmp_buf = (UINT8 *)__G_ALI_MM_DECA_MEM_START_ADDR;

                if (copy_from_user(tmp_buf,(void *)(*(UINT32 *)g_ali_audio_rpc_arg[1]),*(UINT32 *)g_ali_audio_rpc_arg[0]))
                {
					ret = -EFAULT;
					break;
				}

				tmp_buf = (UINT8 *)((((UINT32)tmp_buf)&0x0fffffff) | 0xa0000000);

                 deca_process_pcm_samples(*(UINT32 *)g_ali_audio_rpc_arg[0], tmp_buf,*(UINT32 *) g_ali_audio_rpc_arg[2],\
                *(UINT32 *)g_ali_audio_rpc_arg[3],*(UINT32 *)g_ali_audio_rpc_arg[4]);
              }
              break;
        case AUDIO_DECA_PROCESS_PCM_BITSTREAM:
             {
                UINT8 *pcm_buf,*bs_buf;

				if (((*(UINT32 *)g_ali_audio_rpc_arg[0]) + (*(UINT32 *)g_ali_audio_rpc_arg[2])) > __G_ALI_MM_DECA_MEM_SIZE)
				{
    			    return RET_FAILURE;
				}

				pcm_buf = (UINT8 *)__G_ALI_MM_DECA_MEM_START_ADDR;
                if (copy_from_user(pcm_buf,(void *)(*(UINT32 *)g_ali_audio_rpc_arg[1]),*(UINT32 *)g_ali_audio_rpc_arg[0]))
				{
					ret = -EFAULT;
					break;
				}
				pcm_buf = (UINT8 *)((((UINT32)pcm_buf)&0x0fffffff) | 0xa0000000);

				bs_buf = (UINT8 *)__G_ALI_MM_DECA_MEM_START_ADDR + (*(UINT32 *)g_ali_audio_rpc_arg[0]);
                if (copy_from_user(bs_buf,(void *)(*(UINT32 *)g_ali_audio_rpc_arg[3]),*(UINT32 *)g_ali_audio_rpc_arg[2]))
				{
					ret = -EFAULT;
					break;
				}
				bs_buf = (UINT8 *)((((UINT32)bs_buf)&0x0fffffff) | 0xa0000000);

                //pr_debug("user%p kernel%p user%p kernel%p\n",*(UINT32 *)g_ali_audio_rpc_arg[1],pcm_buf,*(UINT32 *)g_ali_audio_rpc_arg[3],bs_buf);
                 deca_process_pcm_bitstream(*(UINT32 *)g_ali_audio_rpc_arg[0], pcm_buf,
                    *(UINT32 *)g_ali_audio_rpc_arg[2], bs_buf, *(UINT32 *)g_ali_audio_rpc_arg[4],
                    *(UINT32 *)g_ali_audio_rpc_arg[5], *(UINT32 *)g_ali_audio_rpc_arg[6]);
              }
              break;
       default:
                break;
    }
    return ret;

}


static void audio_pcb_first_frame_output(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_FIRST_FRAME_OUTPUT;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

static void audio_pcb_deca_moniter_new_frame(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_MONITOR_NEW_FRAME;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}


static void audio_pcb_deca_moniter_start(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_MONITOR_START;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

static void audio_pcb_deca_moniter_stop(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_MONITOR_STOP;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

static void audio_pcb_deca_moniter_decode_err(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_MONITOR_DECODE_ERR;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

static void audio_pcb_deca_moniter_other_err(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_MONITOR_OTHER_ERR;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

static void audio_pcb_deca_state_changed(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_DECA_STATE_CHANGED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

#if 0
static void audio_pcb_snd_moniter_remain_data_below_threshold(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}
#endif

static void audio_pcb_snd_moniter_output_data_end(UINT32 uParam1, UINT32 uParam2)
{
	UINT8 msg[4];

	msg[0] = MSG_SND_MONITOR_OUTPUT_DATA_END;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}

#if 0
static void audio_pcb_snd_moniter_errors_occured(UINT32 uParam1, UINT32 uParam2)
{
    INT32 port_pid = ali_m36_audio_dev.socket_port_id;
	UINT8 msg[4];

	msg[0] = MSG_SND_MONITOR_ERRORS_OCCURED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;

	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);
}
#endif
static void audio_pcb_snd_sbm_mix_end(UINT32 uParam1, UINT32 uParam2)
{
	unsigned int port_pid = 0;
	UINT8 msg[4];

	pr_debug("%s line[%d]  <uParam1:%lu>--<uParam2:%lu>\n", __FUNCTION__, __LINE__, uParam1, uParam2);
	port_pid = ali_m36_audio_dev.socket_port_id;
	msg[0] = MSG_SND_MONITOR_SBM_MIX_END;
	msg[1] = 2;
	msg[2] = (UINT8)uParam1;
	msg[3] = (UINT8)uParam2;

	pr_debug("%s line[%d]  <port_pid:%u>--<msg[0]:%u>\n", __FUNCTION__, __LINE__, port_pid, msg[0]);
	//ali_transport_send_msg(port_pid, (void *)msg, 4);
	ali_kumsgq_sendmsg(ali_m36_audio_dev.audio_kumsgq, (void *)msg, 4);

    pr_debug("sound sbm mix end!\n");
}

static void audio_cb_routine(UINT32 param1, UINT32 param2)
{
	struct audio_io_reg_callback_para *callback_temp = (struct audio_io_reg_callback_para *)param1;

	switch(callback_temp->e_cb_type)
	{
		case SND_MSG_CB_FIRST_FRM_OUTPUT:
        {
			if(ali_m36_audio_dev.call_back.pcb_first_frame_output)
			{
            	ali_m36_audio_dev.call_back.pcb_first_frame_output(param1, param2);
			}
			break;
		}
        case SND_MSG_CB_OUTPUT_DATA_END:
        {
			if(ali_m36_audio_dev.call_back.pcb_snd_moniter_output_data_end)
			{
            	ali_m36_audio_dev.call_back.pcb_snd_moniter_output_data_end(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_NEW_FRAME:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_moniter_new_frame)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_moniter_new_frame(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_START:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_moniter_start)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_moniter_start(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_STOP:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_moniter_stop)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_moniter_stop(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_FAIL:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_moniter_decode_err)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_moniter_decode_err(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_DATA_INVALID:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_moniter_other_err)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_moniter_other_err(param1, param2);
			}
            break;
        }
        case DECA_MSG_CB_DECODE_STATE_SWITCH:
        {
			if(ali_m36_audio_dev.call_back.pcb_deca_state_changed)
			{
            	ali_m36_audio_dev.call_back.pcb_deca_state_changed(param1, param2);
			}
            break;
        }
        case SND_MONITOR_SBM_MIX_END:
        {
			if(ali_m36_audio_dev.call_back.pcb_snd_moniter_sbm_mix_end)
			{
				pr_debug("==========the callback come!!!=============\n\n");
            	ali_m36_audio_dev.call_back.pcb_snd_moniter_sbm_mix_end(callback_temp->reversed, param2);
			}
            break;
        }
#if 0
        case AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD:
        {
			if(ali_m36_audio_dev.call_back.pcb_snd_moniter_remain_data_below_threshold)
			{
            	ali_m36_audio_dev.call_back.pcb_snd_moniter_remain_data_below_threshold(param1, param2);
			}
            break;
        }
        case AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END:
        {
			if(ali_m36_audio_dev.call_back.pcb_snd_moniter_output_data_end)
			{
            	ali_m36_audio_dev.call_back.pcb_snd_moniter_output_data_end(param1, param2);
			}
            break;
        }
        case AUDIO_CB_SND_MONITOR_ERRORS_OCCURED:
        {
			if(ali_m36_audio_dev.call_back.pcb_snd_moniter_errors_occured)
			{
            	ali_m36_audio_dev.call_back.pcb_snd_moniter_errors_occured(param1, param2);
			}
            break;
        }
#endif /* 0 */
		default:
			break;
	}

    return ;
}


void audio_cb_rpc_register(struct ali_audio_device *p_ali_audio_dev, struct audio_callback_register_param *param)
{
    struct deca_device *see_deca_dev = p_ali_audio_dev->deca_dev;
    struct snd_device *see_snd_dev = p_ali_audio_dev->snd_dev;

    switch(param->e_cb_type)
    {
        case SND_MSG_CB_FIRST_FRM_OUTPUT:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_SND_FIRST_FRAME_OUTPUT, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_first_frame_output = audio_pcb_first_frame_output;


            para.e_cb_type = SND_MSG_CB_FIRST_FRM_OUTPUT;
            para.p_cb = p_ali_audio_dev->call_back.pcb_first_frame_output;
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case SND_MSG_CB_OUTPUT_DATA_END:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_snd_moniter_output_data_end = audio_pcb_snd_moniter_output_data_end;//done

            para.e_cb_type = SND_MSG_CB_OUTPUT_DATA_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_output_data_end;
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);
            break;
        }
        case DECA_MSG_CB_DECODE_NEW_FRAME:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_NEW_FRAME, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_moniter_new_frame = audio_pcb_deca_moniter_new_frame;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_NEW_FRAME;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_new_frame;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_START:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_START, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_moniter_start = audio_pcb_deca_moniter_start;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_START;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_start;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_STOP:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_STOP, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_moniter_stop = audio_pcb_deca_moniter_stop;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_STOP;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_stop;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_FAIL:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_DECODE_ERR, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_moniter_decode_err = audio_pcb_deca_moniter_decode_err;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_FAIL;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_decode_err;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_DATA_INVALID:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_OTHER_ERR, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_moniter_other_err = audio_pcb_deca_moniter_other_err;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_DATA_INVALID;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_other_err;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_STATE_SWITCH:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_DECA_STATE_CHANGED, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_deca_state_changed = audio_pcb_deca_state_changed;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_STATE_SWITCH;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_state_changed;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case SND_MONITOR_SBM_MIX_END:
        {
            struct audio_io_reg_callback_para para;

            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_SBM_MIX_END, audio_cb_routine);
            p_ali_audio_dev->call_back.pcb_snd_moniter_sbm_mix_end = audio_pcb_snd_sbm_mix_end;

            para.e_cb_type = SND_MONITOR_SBM_MIX_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_sbm_mix_end;
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);
            break;
        }
#if 0
        case AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD:
        {
            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_snd_moniter_remain_data_below_threshold = audio_pcb_snd_moniter_remain_data_below_threshold;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_remain_data_below_threshold;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }

        case AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END:
        {
            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_snd_moniter_output_data_end = audio_pcb_snd_moniter_output_data_end;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_first_frame_output;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case AUDIO_CB_SND_MONITOR_ERRORS_OCCURED:
        {
            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_ERRORS_OCCURED, audio_cb_routine);

            p_ali_audio_dev->call_back.pcb_snd_moniter_errors_occured = audio_pcb_snd_moniter_errors_occured;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_ERRORS_OCCURED;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_errors_occured;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
#endif /* 0 */
        default:
            break;
    }

	return;
}


void audio_cb_rpc_unregister(struct ali_audio_device *p_ali_audio_dev, struct audio_callback_register_param *param)
{
    struct deca_device *see_deca_dev = p_ali_audio_dev->deca_dev;
    struct snd_device *see_snd_dev = p_ali_audio_dev->snd_dev;

    switch(param->e_cb_type)
    {
        case SND_MSG_CB_FIRST_FRM_OUTPUT:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_SND_FIRST_FRAME_OUTPUT, NULL);

            p_ali_audio_dev->call_back.pcb_first_frame_output = NULL;

            para.e_cb_type = SND_MSG_CB_FIRST_FRM_OUTPUT;
            para.p_cb = p_ali_audio_dev->call_back.pcb_first_frame_output;
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case SND_MSG_CB_OUTPUT_DATA_END:
        {
            struct audio_io_reg_callback_para para;
            ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END, NULL);

            p_ali_audio_dev->call_back.pcb_snd_moniter_output_data_end = NULL;

            para.e_cb_type = SND_MSG_CB_OUTPUT_DATA_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_first_frame_output;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);
            break;
        }
        case DECA_MSG_CB_DECODE_NEW_FRAME:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_NEW_FRAME, NULL);

            p_ali_audio_dev->call_back.pcb_deca_moniter_new_frame = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_NEW_FRAME;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_new_frame;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_START:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_START, NULL);

            p_ali_audio_dev->call_back.pcb_deca_moniter_start = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_START;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_start;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_STOP:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_STOP, NULL);

            p_ali_audio_dev->call_back.pcb_deca_moniter_stop = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_STOP;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_stop;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_FAIL:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_DECODE_ERR, NULL);

            p_ali_audio_dev->call_back.pcb_deca_moniter_decode_err = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_FAIL;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_decode_err;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_DATA_INVALID:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_MONITOR_OTHER_ERR, NULL);

            p_ali_audio_dev->call_back.pcb_deca_moniter_other_err = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_DATA_INVALID;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_moniter_other_err;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case DECA_MSG_CB_DECODE_STATE_SWITCH:
        {
            struct audio_io_reg_callback_para para;
            //ali_rpc_register_callback(ALI_RPC_CB_DECA_STATE_CHANGED, NULL);

            p_ali_audio_dev->call_back.pcb_deca_state_changed = NULL;//done

            para.e_cb_type = DECA_MSG_CB_DECODE_STATE_SWITCH;
            para.p_cb = p_ali_audio_dev->call_back.pcb_deca_state_changed;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            deca_io_control(see_deca_dev, DECA_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case SND_MONITOR_SBM_MIX_END:
        {
            struct audio_io_reg_callback_para para;

            //ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_SBM_MIX_END, NULL);

            p_ali_audio_dev->call_back.pcb_snd_moniter_sbm_mix_end= NULL;

            para.e_cb_type = SND_MONITOR_SBM_MIX_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_sbm_mix_end;
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            pr_debug("[SND_IO_REG_CALLBACK] <e_cb_type:%lu>--<p_cb:%08x> <para:%08x>\n", para.e_cb_type, (unsigned int)para.p_cb, (unsigned int)&para);
            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);
            pr_debug("%s line[%d] out\n", __FUNCTION__, __LINE__);
            break;
        }
#if 0
        case AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD:
        {
            //ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD, NULL);

            p_ali_audio_dev->call_back.pcb_snd_moniter_remain_data_below_threshold = NULL;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_remain_data_below_threshold;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }

        case AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END:
        {
            //ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END, NULL);

            p_ali_audio_dev->call_back.pcb_snd_moniter_output_data_end = NULL;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END;
            para.p_cb = p_ali_audio_dev->call_back.pcb_first_frame_output;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
        case AUDIO_CB_SND_MONITOR_ERRORS_OCCURED:
        {
            //ali_rpc_register_callback(ALI_RPC_CB_SND_MONITOR_ERRORS_OCCURED, NULL);

            p_ali_audio_dev->call_back.pcb_snd_moniter_errors_occured = NULL;//done

            struct audio_io_reg_callback_para para;
            para.e_cb_type = AUDIO_CB_SND_MONITOR_ERRORS_OCCURED;
            para.p_cb = p_ali_audio_dev->call_back.pcb_snd_moniter_errors_occured;//done
            para.monitor_rate = param->monitor_rate;
            para.timeout_threshold = param->timeout_threshold;

            snd_io_control(see_snd_dev, SND_IO_REG_CALLBACK, (UINT32)&para);

            break;
        }
#endif /* 0 */
        default:
            break;
    }

	return;
}

static int audio_dd_protect_load(unsigned char *dst, unsigned int size)
{
    int ret;
    struct file *fp;
    struct kstat stat;
    mm_segment_t fs;
    loff_t pos;
    char *dd_path = "/etc/sign/plugin_dd_decoder.bin";

	if(dst == NULL || size <= 0) {
		pr_err("invalid address\n");
		return -EFAULT;
	}
	
    fs = get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_stat(dd_path, &stat);
    if(unlikely(ret != 0)) {
        pr_warn("can not get file property: %s", dd_path);
        return -EFAULT;
    }

	if(size < stat.size) {
		pr_err("memory size for load decoder not enough, need %lld, actual %d\n", stat.size, size);
        return -EFAULT;
	}
	
    memset(dst, 0, size);
    fp = filp_open(dd_path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        pr_warn("open file error %s\n", dd_path);
        return -EFAULT;
    }

    pos = 0;
    vfs_read(fp, dst, stat.size, &pos);
    if(unlikely(pos != stat.size)){
        pr_err("short byte read: %lld, expect %lld\n", pos, stat.size);
    }

    filp_close(fp,NULL);
    set_fs(fs);

    return pos;
}

static void check_dec_audio_support_status(struct deca_device *dev)
{
    unsigned int supported;
    if(dev == NULL) {
        pr_err("invalid device for %s\n", __func__);
        return;
    }

    pr_warn("DECA: \n");
    supported = AUDIO_MPEG2;
    deca_io_control(dev, DECA_GET_AUDIO_SUPPORT_STATUS, (unsigned int)&supported);
    if(supported != 0 && supported != 0xff)
        pr_warn("MP2: %d\n", supported);

    supported = AUDIO_MPEG_AAC;
    deca_io_control(dev, DECA_GET_AUDIO_SUPPORT_STATUS, (unsigned int)&supported);
    if(supported != 0 && supported != 0xff)
        pr_warn("AAC: %d\n", supported);

    supported = AUDIO_AC3;
    deca_io_control(dev, DECA_GET_AUDIO_SUPPORT_STATUS, (unsigned int)&supported);
    if(supported != 0 && supported != 0xff)
        pr_warn("DD: %d\n", supported);

    supported = AUDIO_EC3;
    deca_io_control(dev, DECA_GET_AUDIO_SUPPORT_STATUS, (unsigned int)&supported);
    if(supported != 0 && supported != 0xff)
        pr_warn("DDP: %d\n", supported);

    supported = AUDIO_MP3;
    deca_io_control(dev, DECA_GET_AUDIO_SUPPORT_STATUS, (unsigned int)&supported);
    if(supported != 0 && supported != 0xff)
        pr_warn("MPX: %d\n", supported);
}

static void check_mp_audio_support_status(struct deca_device *dev)
{
    unsigned int supported = 0xff, audio_type = 0;
    void *ptype = (void *)&audio_type;

    if(dev == NULL) {
        pr_err("invalid device for %s\n", __func__);
        return;
    }

    pr_warn("MPLG:\n");
    audio_type = AUDIO_MPEG2;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("MP2: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_MPEG_AAC;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("AAC: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_AC3;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("DD/DDP: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_MP3;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("MPX: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_ADPCM;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("ADPCM: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_AMR;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("AMR: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_BYE1;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("BYE1: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_FLAC;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("FLAC: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_OGG;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("OGG: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_PCM;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("PCM: %d\n", supported);

    supported = 0xff;
    audio_type = AUDIO_VORBIS;
    deca_decore_ioctl(dev, DECA_DECORE_GET_SUPPORT_STATUS, ptype, (void *)&supported);
    if(supported != 0)
        pr_warn("VORBIS: %d\n\n", supported);
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long ali_m36_audio_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
#else
static int ali_m36_audio_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long parg)
#endif
{
	struct ali_audio_device *dev = file->private_data;
	unsigned char volume = 0;
	RET_CODE ret = 0;
    RET_CODE copy_ret = 0;
	static int tone_voice_status=0;
	unsigned int udr_times = 0;

	down(&m_audio_sem);

	switch (cmd)
    {
		case AUDIO_STOP:
			if(deca_stop_in_kernel)
			{
				/*if deca stop was called in kernel before rather than App/hld ,we need to call deca start first,then
				call deca stop,otherwise deca stop return failure!*/
				deca_start(dev->deca_dev, 0);
				deca_stop_in_kernel = 0;
			}
			ret = deca_stop(dev->deca_dev, 0, ADEC_STOP_IMM);
			break;

		case AUDIO_PLAY:
			m_audio_pause = 0;
			ret = deca_start(dev->deca_dev, 0);
			break;

		case AUDIO_PAUSE:
			ret = deca_pause(dev->deca_dev);
			break;

		case AUDIO_CONTINUE:
			ret = deca_start(dev->deca_dev, 0);
			break;
	        case AUDIO_ASE_INIT:        //added
	        {
	            deca_init_ase(dev->deca_dev);
	            break;
	        }
	        case AUDIO_ASE_STR_PLAY:
	        {
	    		struct ali_audio_rpc_pars pars;

	    		if (copy_from_user((void *)&pars, (void *)parg, sizeof(pars)))
	    		{
	    			up(&m_audio_sem);
					return -EFAULT;
				}

				if(pars.arg_num != 0)
	            {
	                if (copy_from_user((void *)g_ali_audio_rpc_arg[0], pars.arg[0].arg,pars.arg[0].arg_size))
					{
		    			up(&m_audio_sem);
						return -EFAULT;
					}
	            }

				if ((((struct ase_str_play_param *)g_ali_audio_rpc_arg[0])->len) > __G_ALI_MM_DECA_MEM_SIZE)
				{
					pr_err("request buf size is too big, pls change it smaller!\n");

					up(&m_audio_sem);
	                return -1;
				}

				g_ali_audio_rpc_tmp = (volatile unsigned long *)__G_ALI_MM_DECA_MEM_START_ADDR;

				if (copy_from_user((void *)g_ali_audio_rpc_tmp, ((struct ase_str_play_param *)g_ali_audio_rpc_arg[0])->src,((struct ase_str_play_param *)g_ali_audio_rpc_arg[0])->len))
				{
					up(&m_audio_sem);
					return -EFAULT;
				}
                    __CACHE_FLUSH_ALI(g_ali_audio_rpc_tmp,((struct ase_str_play_param *)g_ali_audio_rpc_arg[0])->len);
	            ((struct ase_str_play_param *)g_ali_audio_rpc_arg[0])->src = (UINT8 *)((((UINT32)g_ali_audio_rpc_tmp)&0x0fffffff) | 0xa0000000);

	           // ret = deca_io_control(dev->deca_dev, DECA_STR_PLAY, (struct ase_str_play_param *)g_ali_audio_rpc_arg[0]);
	            ret = deca_io_control(dev->deca_dev, DECA_STR_PLAY, (UINT32)g_ali_audio_rpc_arg[0]);
	            break;
	        }
	        case AUDIO_ASE_STR_STOP:
	        {
	            ret = deca_io_control(dev->deca_dev, DECA_STR_STOP, 0);
	            break;
	        }
	        case AUDIO_ASE_DECA_BEEP_INTERVAL:
	        {
	            ret = deca_io_control(dev->deca_dev, DECA_BEEP_INTERVAL, parg);
	            break;
	        }
		case AUDIO_SELECT_SOURCE:
			//av7110->audiostate.stream_source = (audio_stream_source_t) arg;
			break;

		case AUDIO_SET_MUTE:
			ret = snd_set_mute(dev->snd_dev,SND_SUB_OUT, parg);
			break;

		case AUDIO_SET_AV_SYNC:
			ret = deca_set_sync_mode(dev->deca_dev, (enum adec_sync_mode)parg);
			break;

		case AUDIO_SET_BYPASS_MODE:
        {
            ret = snd_set_spdif_type(dev->snd_dev, (enum asnd_out_spdif_type)parg);

			break;
		}

		case AUDIO_CHANNEL_SELECT:
			ret = snd_set_duplicate(dev->snd_dev, (enum snd_dup_channel)parg);
			break;

		case AUDIO_GET_STATUS:
			//memcpy(parg, &av7110->audiostate, sizeof(struct audio_status));
			break;

		case AUDIO_GET_CAPABILITIES:
	/*		if (FW_VERSION(av7110->arm_app) < 0x2621)
				*(unsigned int *)parg = AUDIO_CAP_LPCM | AUDIO_CAP_MP1 | AUDIO_CAP_MP2;
			else
				*(unsigned int *)parg = AUDIO_CAP_LPCM | AUDIO_CAP_DTS | AUDIO_CAP_AC3 |
							AUDIO_CAP_MP1 | AUDIO_CAP_MP2;
	*/
			break;

		case AUDIO_CLEAR_BUFFER:
	/*		dvb_ringbuffer_flush_spinlock_wakeup(&av7110->aout);
			av7110_ipack_reset(&av7110->ipack[0]);
			if (av7110->playing == RP_AV)
				ret = av7110_fw_cmd(av7110, COMTYPE_REC_PLAY,
						    __Play, 2, AV_PES, 0);
	*/
			break;

		case AUDIO_SET_ID:
			break;

		case AUDIO_SET_MIXER:
			break;

		case AUDIO_SET_STREAMTYPE:
			ret = deca_io_control(dev->deca_dev, DECA_SET_STR_TYPE, (enum audio_stream_type)parg);
			break;
		case AUDIO_SET_VOLUME:
			ret = snd_set_volume(dev->snd_dev, SND_SUB_OUT, parg);
			break;
        case AUDIO_GET_VOLUME:
            volume = snd_get_volume(dev->snd_dev);
            if (copy_to_user((void *)parg, &volume, sizeof(volume)))
			{
				ret = -EFAULT;
				break;
			}
            break;
        case AUDIO_GET_UNDERRUN_TIMES:
            udr_times = snd_get_underrun_times(dev->snd_dev);
            if (copy_to_user((void *)parg, &udr_times, sizeof(udr_times)))
			{
				ret = -EFAULT;
			}
            break;
		case AUDIO_GET_INPUT_CALLBACK_ROUTINE:
			break;
		case AUDIO_GEN_TONE_VOICE:
        {
            UINT8 *tmp_buf = NULL;
            struct ali_audio_ioctl_tone_voice rpc_tone_voice ={0};

            pr_debug("%s line[%d] step a\n",__FUNCTION__, __LINE__);
            if (copy_from_user(&rpc_tone_voice, (void *)parg, sizeof(struct ali_audio_ioctl_tone_voice)))
    		{
    			up(&m_audio_sem);
				return -EFAULT;
			}
            pr_debug("%s line[%d] <buffer_add:0x%x>--<buffer_len:0x%x>\n",__FUNCTION__, __LINE__ , (unsigned int)rpc_tone_voice.buffer_add, (unsigned int)rpc_tone_voice.buffer_len );

        	if (rpc_tone_voice.buffer_len > __G_ALI_MM_DECA_MEM_SIZE)
        	{
        	    pr_err("beep tone data is larger than our Audio Decode Memory!\n");
			    up(&m_audio_sem);
        	    return RET_FAILURE;
        	}

        	tmp_buf = (UINT8 *)__G_ALI_MM_DECA_MEM_START_ADDR;

            if (copy_from_user(tmp_buf, (void *)rpc_tone_voice.buffer_add, rpc_tone_voice.buffer_len))
    		{
    			up(&m_audio_sem);
				return -EFAULT;
			}
			__CACHE_FLUSH_ALI(tmp_buf, rpc_tone_voice.buffer_len);
			tmp_buf = (UINT8*)((((UINT32)tmp_buf)&0x0fffffff) | 0xa0000000);
			deca_tone_voice(dev->deca_dev, (UINT32)tmp_buf, rpc_tone_voice.buffer_len);
			pr_debug("%s line[%d] step b\n",__FUNCTION__, __LINE__);
			break;
		}
		case AUDIO_STOP_TONE_VOICE:
			deca_stop_tone_voice(dev->deca_dev);
			tone_voice_status=0;
			break;

		case AUDIO_EMPTY_BS_SET:
	 		ret = deca_io_control(dev->deca_dev, DECA_EMPTY_BS_SET, parg);
			break;

		case AUDIO_ADD_BS_SET:
	      	ret = deca_io_control(dev->deca_dev, DECA_ADD_BS_SET,  (enum audio_stream_type)parg);
	        break;

		case AUDIO_DEL_BS_SET:
	      	ret = deca_io_control(dev->deca_dev, DECA_DEL_BS_SET, (enum audio_stream_type)parg);
			break;

		case AUDIO_DECA_SET_DBG_LEVEL:
		    deca_set_dbg_level(dev->deca_dev, parg);
		    break;

		case AUDIO_SND_SET_DBG_LEVEL:
		    snd_set_dbg_level(dev->snd_dev, parg);
		   break;

		case AUDIO_SND_START:
		    snd_start(dev->snd_dev);
		    break;
		case AUDIO_SND_STOP:
		    snd_stop(dev->snd_dev);
		    break;
		case AUDIO_SND_STC_INVALID:
		   stc_invalid();
		   break;
		case AUDIO_SND_STC_VALID:
		   stc_valid();
		   break;
		case AUDIO_SET_CTRL_BLK_INFO:
			if (copy_from_user(&dev->audio_cb,(void *)parg,sizeof(ali_audio_ctrl_blk)))
			{
				up(&m_audio_sem);
				return -EFAULT;
			}
			dev->cb_avail = 1;
			break;
		case AUDIO_DECA_IO_COMMAND:
		{
			struct ali_audio_ioctl_command io_param;

            if (copy_from_user(&io_param, (void *)parg, sizeof(struct ali_audio_ioctl_command)))
			{
				up(&m_audio_sem);
				return -EFAULT;
			}

            if ((DECA_GET_STR_TYPE == io_param.ioctl_cmd) || \
                (DECA_GET_HIGHEST_PTS == io_param.ioctl_cmd) || \
                (DECA_GET_AC3_BSMOD == io_param.ioctl_cmd) || \
                (DECA_CHECK_DECODER_COUNT == io_param.ioctl_cmd) || \
                (DECA_GET_DESC_STATUS == io_param.ioctl_cmd) || \
                (DECA_GET_DECODER_HANDLE == io_param.ioctl_cmd) || \
                (DECA_GET_DECA_STATE == io_param.ioctl_cmd) || \
                (DECA_SOFTDEC_GET_ELAPSE_TIME2 == io_param.ioctl_cmd) || \
                (DECA_DOLBYPLUS_CONVERT_STATUS == io_param.ioctl_cmd) || \
                (DECA_GET_BS_FRAME_LEN == io_param.ioctl_cmd) || \
                (DECA_GET_DDP_INMOD == io_param.ioctl_cmd))
            {
                UINT32 deca_param;
                ret = deca_io_control(dev->deca_dev, io_param.ioctl_cmd, (UINT32)&deca_param);

				if (RET_SUCCESS == ret)
				{
					if((copy_ret = copy_to_user((void *)io_param.param, (void *)&deca_param, sizeof(unsigned int))) !=  0)
					{
						up(&m_audio_sem);
						return -EFAULT;
					}
				}
            }
            else
            {
                if(DECA_GET_PLAY_PARAM == io_param.ioctl_cmd)
                {
                    struct cur_stream_info deca_play_info;
                    ret = deca_io_control(dev->deca_dev, io_param.ioctl_cmd, (UINT32)&deca_play_info);
    				if (RET_SUCCESS == ret)
    				{
    					if((copy_ret = copy_to_user((void *)io_param.param, (void *)&deca_play_info, sizeof(struct cur_stream_info))) !=  0)
    					{
						    up(&m_audio_sem);
    						return -EFAULT;
    					}
    				}
                }
                else if (DECA_GET_ES_BUFF_STATE == io_param.ioctl_cmd)
                {
                    struct deca_buf_info deca_es_buff_info;
                    ret = deca_io_control(dev->deca_dev, io_param.ioctl_cmd, (UINT32)&deca_es_buff_info);
    				if (RET_SUCCESS == ret)
    				{
    					if((copy_ret = copy_to_user((void *)io_param.param, (void *)&deca_es_buff_info, sizeof(struct deca_buf_info))) !=  0)
    					{
						    up(&m_audio_sem);
    						return -EFAULT;
    					}
    				}
                }
                else if (DECA_GET_AUDIO_INFO == io_param.ioctl_cmd)
                {
                    struct AUDIO_INFO audio_info;
                    ret = deca_io_control(dev->deca_dev, io_param.ioctl_cmd, (UINT32)(&audio_info));
    				if (RET_SUCCESS == ret)
    				{
    					if((copy_ret = copy_to_user((void *)io_param.param, (void *)&audio_info, sizeof(struct AUDIO_INFO))) !=  0)
    					{
    						pr_err("%s error line%d\n", __FUNCTION__, __LINE__);

    						// Invalid user space address
						    up(&m_audio_sem);
    						return -EFAULT;
    					}
    				}
                }
                else if (DECA_GET_DBG_INFO == io_param.ioctl_cmd)
                {
			pr_err("\033[40;31m%s->%s.%lu, the DECA_GET_DBG_INFO cmd only use inside!\033[0m\n",
                        __FILE__, __FUNCTION__, parg);
                    ret = -EINVAL;
                }
                else
                {
                    ret = deca_io_control(dev->deca_dev, io_param.ioctl_cmd, io_param.param);
                }
            }

			break;
		}
		case AUDIO_SND_IO_COMMAND:
		{
			struct ali_audio_ioctl_command io_param;

			if (copy_from_user(&io_param, (void *)parg, sizeof(struct ali_audio_ioctl_command)))
			{
				up(&m_audio_sem);
				return -EFAULT;
			}

            if ((SND_CHK_DAC_PREC == io_param.ioctl_cmd) || \
                (SND_GET_RAW_PTS == io_param.ioctl_cmd) || \
                (SND_REQ_REM_DATA == io_param.ioctl_cmd) || \
                (SND_GET_TONE_STATUS == io_param.ioctl_cmd) || \
                (SND_CHK_PCM_BUF_DEPTH == io_param.ioctl_cmd) || \
                (SND_GET_SAMPLES_REMAIN == io_param.ioctl_cmd) || \
                (SND_REQ_REM_PCM_DATA == io_param.ioctl_cmd) || \
                (SND_REQ_REM_PCM_DURA == io_param.ioctl_cmd) || \
                (SND_GET_SPDIF_TYPE == io_param.ioctl_cmd) || \
                (SND_GET_MUTE_TH == io_param.ioctl_cmd) ||
                (SND_ONLY_GET_SPDIF_DELAY_TIME == io_param.ioctl_cmd) ||
                (SND_IO_SPO_INTF_CFG_GET == io_param.ioctl_cmd) ||
                (SND_IO_DDP_SPO_INTF_CFG_GET == io_param.ioctl_cmd) || \
                (SND_GET_AD_DYNAMIC_EN == io_param.ioctl_cmd) || \
                (SND_IO_GET_MUTE_STATE == io_param.ioctl_cmd) || \
                (SND_IO_GET_CHAN_STATE == io_param.ioctl_cmd))
            {
                UINT32 snd_param;
			    ret = snd_io_control(dev->snd_dev, io_param.ioctl_cmd, (UINT32)&snd_param);

				if (RET_SUCCESS == ret)
				{
					if ((copy_ret = copy_to_user((void *)io_param.param, (void *)&snd_param, sizeof(unsigned int))) !=  0)
					{
						up(&m_audio_sem);
						return -EFAULT;
					}
				}
            }
            else if (SND_SET_DESC_VOLUME_OFFSET_NEW == io_param.ioctl_cmd)
            {
                if ((((int)io_param.param)>AUDIO_DESC_LEVEL_MAX) || (((int)io_param.param)<AUDIO_DESC_LEVEL_MIN))
                {
                    pr_err("#%s->%s.%u, param(%d) should be [-16, 16].\n", __FILE__, __FUNCTION__, __LINE__, (int)io_param.param);
                    ret = -EINVAL;
                }
                else
                {
                    ret = snd_io_control(dev->snd_dev, SND_SET_DESC_VOLUME_OFFSET_NEW, io_param.param);
                }
            }
            else if (SND_GET_DBG_INFO == io_param.ioctl_cmd)
            {
                pr_err("\033[40;31m%s->%s.%lu, the DECA_GET_DBG_INFO cmd only use inside!\033[0m\n",
                    __FILE__, __FUNCTION__, parg);
                ret = -EINVAL;
            }
            else if (SND_SET_AD_DYNAMIC_EN == io_param.ioctl_cmd)
            {
                //pr_debug("\033[40;31m%s->%s.%u, parg=%d.\033[0m\n", __FILE__, __FUNCTION__, __LINE__, parg);
                if ((0==(io_param.param)) || (1==(io_param.param)))
                {
                    ret = snd_io_control(dev->snd_dev, SND_SET_AD_DYNAMIC_EN, (io_param.param));
                }
                else
                {
                    pr_err("\033[40;31m%s->%s.%u, parg=%d is invalid!\033[0m\n", __FILE__, __FUNCTION__, __LINE__, (int)(io_param.param));
                    ret = -EINVAL;
                }
            }
            else if (SND_IO_GET_PLAY_PTS == io_param.ioctl_cmd)
            {
                struct snd_get_pts_param pts_param;
                memset(&pts_param, 0, sizeof(pts_param));
                ret = snd_io_control(dev->snd_dev, io_param.ioctl_cmd, (UINT32)&pts_param);
                printk("pts_valid: %d, pts: 0x%x\n", pts_param.pts_valid, (int)pts_param.pts);
                if (RET_SUCCESS == ret)
				{
					if((copy_ret = copy_to_user((void *)io_param.param, (void *)&pts_param, sizeof(struct snd_get_pts_param))) !=  0)
					{
					    up(&m_audio_sem);
						return -EFAULT;
					}
				}
            }
            else
            {
			    ret = snd_io_control(dev->snd_dev, io_param.ioctl_cmd, io_param.param);
            }

			break;
		}

		case AUDIO_RPC_CALL_ADV:
		{
			struct ali_audio_rpc_pars pars;
			int i = 0;

		    if (copy_from_user((void *)&pars, (void *)parg, sizeof(pars)))
			{
				up(&m_audio_sem);
				return -EFAULT;
			}

		    if(pars.arg_num > MAX_AUDIO_RPC_ARG_NUM)
		    {
    		    up(&m_audio_sem);
			    return -EINVAL;
		    }

		    for(i = 0; i < pars.arg_num; i++)
		    {
		        g_ali_audio_rpc_arg_size[i] = pars.arg[i].arg_size;	//check size
		        if(g_ali_audio_rpc_arg_size[i] > 0)
		        {
		        	if(g_ali_audio_rpc_arg[i] == NULL || g_ali_audio_rpc_arg_size[i] > MAX_AUDIO_RPC_ARG_SIZE)
		        	{
		                ret = -ENOMEM;
		                goto RPC_EXIT_2;
		        	}
		        	if (copy_from_user((void *)g_ali_audio_rpc_arg[i], pars.arg[i].arg, g_ali_audio_rpc_arg_size[i]))//check param
					{
						up(&m_audio_sem);
						return -EFAULT;
					}
		        }
		    }

		    ret = audio_rpc_operation(dev,pars.API_ID);

RPC_EXIT_2:
			for(i = 0; i < pars.arg_num;i++)
			{
				if(g_ali_audio_rpc_arg_size[i] > 0)
				{
					if(pars.arg[i].out)
					{
						if (copy_to_user(pars.arg[i].arg, (void *)g_ali_audio_rpc_arg[i], g_ali_audio_rpc_arg_size[i]))
						{
							ret = -EFAULT;
							break;
						}
					}
		        }
			}

			break;
		}

    case AUDIO_DECORE_COMMAND:
    {
		struct ali_audio_rpc_pars pars;
		int i = 0;

		if (copy_from_user((void *)&pars, (void *)parg, sizeof(pars)))
		{
			up(&m_audio_sem);
			return -EFAULT;
		}

		for(i = 0; i < pars.arg_num; i++)
		{
			g_ali_audio_rpc_arg_size[i] = pars.arg[i].arg_size;
			if(g_ali_audio_rpc_arg_size[i] > 0)
			{
				if(g_ali_audio_rpc_arg[i] == NULL || g_ali_audio_rpc_arg_size[i] > MAX_AUDIO_RPC_ARG_SIZE)
				{
                    ret = -ENOMEM;
				    goto RPC_EXIT;
				}
				if (copy_from_user((void *)g_ali_audio_rpc_arg[i], pars.arg[i].arg, g_ali_audio_rpc_arg_size[i]))
				{
					pr_err("copy_from_user warning need fix it %s %d \n", __func__, __LINE__);
					up(&m_audio_sem);
					return -EFAULT;
				}
			}
		}

        ret = deca_decore_ioctl(ali_m36_audio_dev.deca_dev, *(int *)g_ali_audio_rpc_arg[0], (void *)g_ali_audio_rpc_arg[1], (void *)g_ali_audio_rpc_arg[2]);

RPC_EXIT:
		for(i = 0; i < pars.arg_num;i++)
		{
			if(g_ali_audio_rpc_arg_size[i] > 0)
			{
				if(pars.arg[i].out) {
					if (copy_to_user(pars.arg[i].arg, (void *)g_ali_audio_rpc_arg[i], g_ali_audio_rpc_arg_size[i]))
					{
						pr_err("copy_to_user warning need fix it %s %d \n", __func__, __LINE__);
						break;
					}
				}
			}
		}
        break;
    }
    case AUDIO_GET_PTS:
    {
        unsigned long cur_pts = 0;
        cur_pts = *((unsigned long *)0xb8002044);
        if (copy_to_user((unsigned long *)parg,&cur_pts,4))
        {
			ret = -EFAULT;
		}

        break;
    }

	case AUDIO_GET_KUMSGQ:
	{
		int flags = -1;
		mutex_lock(&ali_m36_audio_dev.audio_mutex);
		if(copy_from_user(&flags, (int *)parg, sizeof(int)))
		{
			pr_err("Err: copy_from_user\n");
			mutex_unlock(&ali_m36_audio_dev.audio_mutex);
			up(&m_audio_sem);
			return -EFAULT;
		}
		ret  = ali_kumsgq_newfd(ali_m36_audio_dev.audio_kumsgq, flags);
		if(ret> 0)
		{
			mutex_unlock(&ali_m36_audio_dev.audio_mutex);
			up(&m_audio_sem);
			return ret;
		}
	}

    case AUDIO_CB_RPC_REGISTER:
    {
        struct audio_callback_register_param param;

        if (copy_from_user((void *)&param, (void *)parg, sizeof(struct audio_callback_register_param)))
		{
			up(&m_audio_sem);
			return -EFAULT;
		}

        audio_cb_rpc_register(&ali_m36_audio_dev, &param);
        break;
    }
    case AUDIO_CB_RPC_UNREGISTER:
    {
        struct audio_callback_register_param param;

        if (copy_from_user((void *)&param, (void *)parg, sizeof(struct audio_callback_register_param)))
		{
			up(&m_audio_sem);
			return -EFAULT;
		}

        audio_cb_rpc_unregister(&ali_m36_audio_dev, &param);
        break;
    }
    case AUDIO_INIT_TONE_VOICE:
    {
	    pr_debug("%s line[%d] deca_m36_init_tone_voice init!\n", __FUNCTION__, __LINE__);
        deca_m36_init_tone_voice(dev->deca_dev);
        pr_debug("%s line[%d] snd_m36_init_tone_voice init!\n", __FUNCTION__, __LINE__);
    	snd_m36_init_tone_voice(dev->snd_dev);
        pr_debug("%s line[%d] AUDIO_INIT_TONE_VOICE done!\n", __FUNCTION__, __LINE__);
        break;
    }
        case AUDIO_SND_SET_MIX_INFO:
        {
            struct snd_mix_info mix_info;
            if (copy_from_user((void *)&mix_info, (void *)parg, sizeof(struct snd_mix_info)))
			{
				up(&m_audio_sem);
				return -EFAULT;
			}
    	    ret = snd_io_control(dev->snd_dev, SND_IO_SET_MIX_INFO, (UINT32)&mix_info);
            break;
        }
        case AUDIO_SND_SET_MIX_END:
        {
    	    ret = snd_io_control(dev->snd_dev, SND_IO_SET_MIX_END, parg);
            break;
        }
        case AUDIO_SND_GET_MIX_STATE:
        {
            unsigned long state = 0;
            snd_io_control(dev->snd_dev, SND_IO_GET_MIX_STATE, (UINT32)&state);
            if (copy_to_user((unsigned long *)parg,&state,4))
            {
				ret = -EFAULT;
			}

            break;
        }
        case SND_SET_PCM_CAPTURE_BUFF_RD:
        {
			ret = snd_set_pcm_capture_buff_info(dev->snd_dev, parg, (UINT8)(1<<2));
			break;
        }
        case SND_GET_PCM_CAPTURE_BUFF_INFO:
        {
			struct pcm_capture_buff info = {0};
			ret = snd_get_pcm_capture_buff_info(dev->snd_dev, &info);
			if (RET_SUCCESS == ret)
			{
				if (!info.buff_len)
				{
					info.buff_len = ali_m36_audio_dev.pcm_capture_buff_len/16;
				}

				if ((copy_ret = copy_to_user((void *)parg, (void *)&info, sizeof(struct pcm_capture_buff))) !=	0)
				{
					ret = -EFAULT;
				}
			}
			break;
        }
	default:
        {
    		ret = -ENOIOCTLCMD;
            break;
    	}

	}

	up(&m_audio_sem);
	return ret;
}

static int ali_m36_audio_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ali_audio_device *dev = file->private_data;

	if (!ali_m36_audio_dev.pcm_capture_buff || (1<<12) > ali_m36_audio_dev.pcm_capture_buff_len)
	{
		pr_err("%s line[%d] mmap failed! Invalid args!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	if ((vma->vm_flags & VM_WRITE))
	{
		pr_err("%s line[%d] mmap failed! Memory Can't be writed!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	vma->vm_flags |= VM_IO;
	vma->vm_flags |= VM_SHARED | VM_READ;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma,
				vma->vm_start,
				virt_to_phys(ali_m36_audio_dev.pcm_capture_buff) >> PAGE_SHIFT,
				(unsigned long)(vma->vm_end - vma->vm_start),
				vma->vm_page_prot))
	{
		pr_err("%s line[%d] remap_pfn_range failed!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	if (snd_set_pcm_capture_buff_info(dev->snd_dev, (UINT32)ali_m36_audio_dev.pcm_capture_buff, (UINT8)(1<<0)))
	{
		pr_err("%s line[%d] snd_set_pcm_capture_buff_info set pcm_capture_buff failed!\n", __FUNCTION__, __LINE__);
		return -EAGAIN;
	}
	if (snd_set_pcm_capture_buff_info(dev->snd_dev, (UINT32)ali_m36_audio_dev.pcm_capture_buff_len, (UINT8)(1<<1)))
	{
		pr_err("%s line[%d] snd_set_pcm_capture_buff_info set pcm_capture_buff_len failed!\n", __FUNCTION__, __LINE__);
		return -EAGAIN;
	}

	pr_debug("%s line[%d] pcm capture mmap success.\n", __FUNCTION__, __LINE__);
	return 0;
}

static int ali_m36_audio_open(struct inode *inode, struct file *file)
{
	struct deca_device *see_deca_dev=NULL;
	struct snd_device *see_snd_dev=NULL;
    int i = 0;

//	ali_audio_api_show("%s,%d.\n", __FUNCTION__, __LINE__);
	if (!ali_m36_audio_dev.audio_kumsgq)
	{
		if (!(ali_m36_audio_dev.audio_kumsgq = ali_new_kumsgq()))
		{
			goto out0;
		}
	}

	down(&m_audio_sem);

	if(g_ali_audio_rpc_arg[0] == NULL)
    {
		for(i = 0; i < MAX_AUDIO_RPC_ARG_NUM; i++)
        {
			g_ali_audio_rpc_arg[i] = kmalloc(MAX_AUDIO_RPC_ARG_SIZE, GFP_KERNEL);
			if(g_ali_audio_rpc_arg[i] == NULL)
            {
				up(&m_audio_sem);
				return -EINVAL;
			}
		}
	}

    if((!ali_m36_audio_dev.deca_open_flag) || (!ali_m36_audio_dev.snd_open_flag)
			|| (!ali_m36_audio_dev.snd_open_flag))
    {
        see_deca_dev=(struct deca_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_DECA);
        see_snd_dev=(struct snd_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_SND);

        pr_info("dev:%x-%x-%d-%d\n",(unsigned int)see_deca_dev,(unsigned int)see_snd_dev,
		ali_m36_audio_dev.snd_open_flag,ali_m36_audio_dev.deca_open_flag);

        if((NULL==see_deca_dev) ||(NULL==see_snd_dev))
        {
            up(&m_audio_sem);
            return -ENODEV;
        }

        if(!ali_m36_audio_dev.snd_open_flag)
        {
        	ali_m36_audio_dev.snd_open_flag = 1;
        	snd_open(see_snd_dev);
        }

        //deca_open(see_deca_dev, AUDIO_MPEG2, AUDIO_SAMPLE_RATE_48, AUDIO_QWLEN_24, 2, 0);
        //snd_open(see_snd_dev);
        if(!ali_m36_audio_dev.deca_open_flag)
        {
			if(__G_ALI_MM_DECA_MEM_START_ADDR != 0 && __G_ALI_MM_DECA_MEM_SIZE > 0) {
				unsigned char *load_mem = (UINT8 *)__G_ALI_MM_DECA_MEM_START_ADDR;
				unsigned int  mem_size = __G_ALI_MM_DECA_MEM_SIZE;
				int loaded_size = 0;
				
	            loaded_size = audio_dd_protect_load(load_mem, mem_size);
	            //printk(KERN_ERR "load %d bytes to addr %p, mem size %u\n", loaded_size, load_mem, mem_size);
	            
				if(loaded_size > 0) {
					unsigned int  phy_addr = 0;
					unsigned char *noncache_addr = NULL;
					
					phy_addr = (unsigned int)virt_to_phys(load_mem);
					noncache_addr = (unsigned char *)((phy_addr & 0x0fffffff) | 0xa0000000);
					
					//printk(KERN_ERR "phy address %u, noncahce address %p\n", phy_addr, noncache_addr);
					deca_set_dd_plugin_addr(see_deca_dev, noncache_addr);					
	            }
			} else {
				printk(KERN_ERR "Error: invalid deca mem addr %lu or size %lu\n", 
					__G_ALI_MM_DECA_MEM_START_ADDR, __G_ALI_MM_DECA_MEM_SIZE);
			}

			ali_m36_audio_dev.deca_open_flag = 1;
			deca_open(see_deca_dev, AUDIO_MPEG2, AUDIO_SAMPLE_RATE_48, AUDIO_QWLEN_24, 2, 0);
			
			check_dec_audio_support_status(see_deca_dev);
			check_mp_audio_support_status(see_deca_dev);
        }

        //pr_debug("malloc priv OK: %08x\n", priv);
        ali_m36_audio_dev.deca_dev= see_deca_dev;
        ali_m36_audio_dev.snd_dev= see_snd_dev;
    }

    file->private_data = (void*)&ali_m36_audio_dev;

    ali_m36_audio_dev.open_count++;

    if(ali_m36_audio_dev.open_count == 1)
    {
        hld_deca_rpc_release(1);
    }
	//pr_debug("<0>""%s : open count %d\n", __FUNCTION__, ali_m36_audio_dev.open_count);

	up(&m_audio_sem);

    pr_debug("audio_open<---\n");

	return 0;
out0:
	WARN(1,"False to new m36_audio kumsgq!!!!!!");
	return -1;
}

static int ali_m36_audio_release(struct inode *inode, struct file *file)
{
	struct ali_audio_device *dev = file->private_data;
    int i = 0;

	down(&m_audio_sem);

    	dev->open_count--;

   	if(dev->open_count == 0)
 	{
 		m_audio_pause = 0;
		hld_deca_rpc_release(0);

		for (i = 0; i < MAX_AUDIO_RPC_ARG_NUM; i++)
		{
			if (g_ali_audio_rpc_arg[i])
			{
				kfree((void *)g_ali_audio_rpc_arg[i]);
				g_ali_audio_rpc_arg[i] = NULL;
			}
		}

		ali_destroy_kumsgq(ali_m36_audio_dev.audio_kumsgq);
		ali_m36_audio_dev.audio_kumsgq = NULL;
		//deca_close(see_deca_dev);
		//snd_close(see_snd_dev);
      }

	//pr_debug("<0>""%s : open count %d\n", __FUNCTION__, ali_m36_audio_dev.open_count);

	up(&m_audio_sem);
	return 0;
}

static ssize_t ali_m36_audio_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;

#if 0
	struct ali_audio_device *dev = (struct ali_audio_device *)file->private_data;
    void *req_buf = NULL;
    int req_ret = 0;

    unsigned int req_size = count;
    unsigned int ret_size;

START:

	if(dev->cb_avail)
	{
		req_ret = deca_request_write(dev->deca_dev,req_size, &req_buf, (UINT32 *)&ret_size, &dev->audio_cb);
		dev->cb_avail = 0;
	}
	else
	{
		req_ret =  deca_request_write(dev->deca_dev,req_size, &req_buf, (UINT32 *)&ret_size, NULL);
	}

	if(req_ret == RET_SUCCESS)
	{
		{
				void *tmp_buf = NULL;

				#if defined(CONFIG_ARM)
                void *tmp_req_buf = NULL;
				#endif

			//	pr_debug("request ok buf %x size 0x%x\n", (int)req_buf, ret_size);
				tmp_buf = kmalloc(ret_size, GFP_KERNEL);
				if(tmp_buf == NULL)
				{
					pr_debug("kmalloc request buf fail\n");
					ret = -EINVAL;
					goto EXIT;
				}

				if (copy_from_user(tmp_buf, buf, ret_size))
				{
					up(&m_audio_sem);
					return -EFAULT;
				}

				#if defined(CONFIG_ARM)
				__CACHE_FLUSH_ALI((unsigned long)tmp_buf, ret_size);
				//VIDEO_PRF("start to transfer the data to see src %x dst %x size 0x%x\n"
				//	, (int)tmp_buf, (int)req_buf, req_size);
				/* Main CPU should use below mempy to transfer the data to private
				memory */
                tmp_req_buf = (void *)(__VSTMALI((unsigned int)req_buf));

				//hld_dev_memcpy(req_buf, tmp_buf, ret_size);
				memcpy(tmp_req_buf, tmp_buf, ret_size); // change by jacket for s3921
				#else
				dma_cache_wback((unsigned long)tmp_buf, ret_size);
				//VIDEO_PRF("start to transfer the data to see src %x dst %x size 0x%x\n"
				//	, (int)tmp_buf, (int)req_buf, req_size);
				/* Main CPU should use below mempy to transfer the data to private
				memory */
				hld_dev_memcpy(req_buf, tmp_buf, ret_size);
				#endif

				kfree(tmp_buf);
		}

		deca_update_write(dev->deca_dev,ret_size);

		ret += ret_size;

		if(ret_size < count)
		{
			count = req_size = count - ret_size;
			goto START;
		}

	}
	else {
			msleep(10);
			goto START;
			//ret = -EINVAL;
		}

#endif
    return ret;
}

extern UINT32 deca_standby(struct deca_device * device, UINT32 status);

int ali_m36_audio_suspend(struct device *dev, pm_message_t state)
{
    UINT32 pause_dec;
    UINT32 pause_output ;
    struct ali_audio_device *a_dev = dev_get_drvdata(dev);
    struct deca_device *see_deca_dev=NULL;
    see_deca_dev=a_dev->deca_dev;

    pause_dec = 1;
    pause_output= 1;

    deca_decore_ioctl(see_deca_dev,DECA_DECORE_PAUSE_DECODE,(void *)&pause_dec,(void *)&pause_output);

/*
    deca_stop(see_deca_dev, 0, ADEC_STOP_IMM);
    deca_standby(see_deca_dev, 1);
    mdelay(100);
 */

    return 0;
}

int ali_m36_audio_resume(struct device *dev)
{
    UINT32 pause_dec;
    UINT32 pause_output ;
    struct ali_audio_device *a_dev = dev_get_drvdata(dev);
    struct deca_device *see_deca_dev=NULL;
    see_deca_dev=a_dev->deca_dev;

    if(m_audio_pause == 0)
    {
    	pause_dec = 0;
    	pause_output = 0;

    	deca_decore_ioctl(see_deca_dev,DECA_DECORE_PAUSE_DECODE,(void *)&pause_dec,(void *)&pause_output);

    }

/*
    deca_standby(see_deca_dev, 0);
    deca_start(see_deca_dev, 0);
*/
    return 0;
}

static int __devinit pcm_capture_init(void)
{
	struct device_node* node;

	node = of_find_compatible_node(NULL, NULL, "alitech,memory-mapping");
	if(NULL == node)
	{
		pr_err("alitech,memory-mapping node is null\n");
		return -1;
	}
	else
	{
		of_property_read_u32_index(node, "pcm_capture_buff", 0,(u32 *)&ali_m36_audio_dev.pcm_capture_buff);
		of_property_read_u32_index(node, "pcm_capture_buff", 1,(u32 *)&ali_m36_audio_dev.pcm_capture_buff_len);
		if(ali_m36_audio_dev.pcm_capture_buff && (ali_m36_audio_dev.pcm_capture_buff_len >= 1<<12))
		{
			if (!ali_m36_audio_dev.pcm_capture_buff)
			{
				pr_err("pcm_capture_buff address is NULL!\n");
				return -1;
			}

			if ((unsigned int)ali_m36_audio_dev.pcm_capture_buff % 32)
			{
				ali_m36_audio_dev.pcm_capture_buff = (unsigned int *)(((unsigned int)ali_m36_audio_dev.pcm_capture_buff + 31)/32*32);
				ali_m36_audio_dev.pcm_capture_buff_len -= 32;
			}

			if ((1<<20) <= ali_m36_audio_dev.pcm_capture_buff_len)
			{
				ali_m36_audio_dev.pcm_capture_buff_len = (1<<20) - 16;
			}
		}
		else
		{
			ali_m36_audio_dev.pcm_capture_buff = NULL;
			ali_m36_audio_dev.pcm_capture_buff_len = 0;
		}

		return 0;
	}
}

extern int audio_debug_procfs_init(void);
extern int audio_debug_procfs_exit(void);

static int ali_audio_probe(struct platform_device * pdev)
{
	int ret = 0;
	int result = 0;
	dev_t devno;

	/*
	struct deca_feature_config deca_cfg;
	struct snd_feature_config snd_cfg;
	*/
	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, 1, ALI_AUDIO_DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

	memset((void *)&ali_m36_audio_dev, 0, sizeof(ali_m36_audio_dev));

	pr_debug("audio_device_init-->\n");
	mutex_init(&ali_m36_audio_dev.audio_mutex);
	cdev_init(&ali_m36_audio_dev.cdev, &ali_m36_audio_fops);
	ali_m36_audio_dev.cdev.owner=THIS_MODULE;
	ali_m36_audio_dev.cdev.ops=&ali_m36_audio_fops;

	pr_debug("audio_dev_register\n");

	ret=cdev_add(&ali_m36_audio_dev.cdev, devno, 1);
	if(ret)
	{
		pr_err("Alloc audio device failed, err: %d.\n", ret);
		return ret;
	}

	pr_debug("register audio device end.\n");


	ali_m36_audio_class = class_create(THIS_MODULE, "ali_m36_audio_class");

	if (IS_ERR(ali_m36_audio_class))
	{
		result = PTR_ERR(ali_m36_audio_class);

		goto err1;
	}

	ali_m36_audio_dev_node = device_create(ali_m36_audio_class, NULL, devno, &ali_m36_audio_dev, "ali_m36_audio0");
    if (IS_ERR(ali_m36_audio_dev_node))
    {
		result = PTR_ERR(ali_m36_audio_dev_node);

		goto err2;
	}

	ali_m36_audio_class->suspend = ali_m36_audio_suspend;
	ali_m36_audio_class->resume = ali_m36_audio_resume;

	//void ali_m36_audio_test(struct file_operations *fop,struct ali_audio_device *dev);
	//ali_m36_audio_test(&ali_m36_audio_fops, &ali_m36_audio_dev);

    sema_init(&m_audio_sem, 1);

	pr_debug("%s,%d.\n", __FUNCTION__, __LINE__);

//	ali_audio_info_init();

	pr_debug("%s,%d.\n", __FUNCTION__, __LINE__);

	pr_debug("audio_device_init<--:%d\n",ret);

	audio_debug_procfs_init();

//	ali_audio_api_show("%s,%d.ret:%d\n", __FUNCTION__, __LINE__, ret);

	pcm_capture_init();

	return ret;

err2:
	class_destroy(ali_m36_audio_class);
err1:
	cdev_del(&ali_m36_audio_dev.cdev);
	//kfree(priv);
//	ali_audio_api_show("%s,%d.\n", __FUNCTION__, __LINE__);

	return -EINVAL;
}

static int ali_audio_remove(struct platform_device * pdev)
{
    audio_debug_procfs_exit();

	if(ali_m36_audio_dev_node != NULL)
	{
		device_del(ali_m36_audio_dev_node);
	}

	if(ali_m36_audio_class != NULL)
	{
		class_destroy(ali_m36_audio_class);
	}

	cdev_del(&ali_m36_audio_dev.cdev);

	return 0;
//	ali_audio_info_exit();
}

EXPORT_SYMBOL(sys_open);
EXPORT_SYMBOL(sys_read);
EXPORT_SYMBOL(sys_write);
EXPORT_SYMBOL(sys_ioctl);
EXPORT_SYMBOL(sys_poll);
EXPORT_SYMBOL(sys_fork);
EXPORT_SYMBOL(do_fork);
EXPORT_SYMBOL(sys_clone);
EXPORT_SYMBOL(kernel_thread);

static const struct of_device_id ali_audio_of_match[] = {
       { .compatible = "alitech, audio", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_audio_of_match);

static struct platform_driver ali_audio_platform_driver = {
	.probe   = ali_audio_probe, 
	.remove   = ali_audio_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_audio",
			.of_match_table = ali_audio_of_match,
	},
};
module_platform_driver(ali_audio_platform_driver);

MODULE_DESCRIPTION("driver for the Ali M36xx(Dual Core) audio/sound device");
MODULE_AUTHOR("ALi Corp ShangHai SDK Team, Eric Li");
MODULE_LICENSE("GPL");
