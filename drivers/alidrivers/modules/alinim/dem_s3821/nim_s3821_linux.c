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

/*
*    File: nim_s3281_linux.c
*    Description: s3281 nim driver for linux api
*/

#include "nim_s3821.h"

#define ALI_NIM_DEVICE_NAME         "ali_nim_s3821"
#define MAX_TUNER_SUPPORT_NUM        2//1


static struct nim_device             ali_m3821_nim_dev;
struct nim_s3821_private             *ali_m3821_nim_priv = NULL;
static struct class                 *ali_m3821_nim_class;
static struct device                 *ali_m3821_nim_dev_node;
static UINT8                         nim_sw_test_tread_status = 0;

static UINT8                        *g_s3821_adc2dma_st_addr = NULL;
static UINT32                        g_s3821_adc2dma_read_offset = 0;
 


static void nim_s3821_task_open(struct work_struct *nim_work)
{
    struct nim_device                 *dev;
    dev = container_of((void *)nim_work, struct nim_device, work);

    S3821_PRINTF(NIM_LOG_DBG,"[kangzh]line=%d,%s enter!\n", __LINE__, __FUNCTION__);


    nim_s3821_main_thread((UINT32)dev, 0);


}

INT32 nim_s3821_task_init(struct nim_device *dev)
{
    if(NULL == dev)
    {
		return -1;
    }
	
    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);
    dev->workqueue = create_workqueue("nim0");
    if (!(dev->workqueue))
    {
        S3821_PRINTF(NIM_LOG_DBG,"Failed to allocate work queue\n");
        return -1;
    }

    INIT_WORK(&dev->work, nim_s3821_task_open);
    queue_work(dev->workqueue, &dev->work);

    return SUCCESS;
}


extern unsigned long __G_ALI_MM_NIM_PARAM_BUF_SIZE ;
extern unsigned long __G_ALI_MM_NIM_PARAM_BUF_ADDR ;


static void nim_s3821_reset(void)
{
        //joey, 20150211, for C3821, OFDM IP reset register is 80h.
		//step 1: do the COFDM whole reset.
	UINT32 tmp_data;

	tmp_data = *(volatile UINT32 *)0xb8000080;
	comm_sleep(10);
	
	tmp_data = tmp_data | (1<<22);
	*(volatile UINT32 *)0xb8000080 = tmp_data;
	comm_sleep(10);
	
	tmp_data = tmp_data & (~(1<<22));
	*(volatile UINT32 *)0xb8000080 = tmp_data;
	comm_sleep(10);
	
}

static void nim_s3821_set_config(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
    struct nim_s3821_private 	*priv = dev->priv;
    TUNER_IO_FUNC               *p_io_func=NULL;

	
    memcpy(&priv->tuner_control.config_data, &nim_cfg->cofdm_data, sizeof(struct COFDM_TUNER_CONFIG_DATA));
    memcpy(&priv->tuner_control.tuner_config, &nim_cfg->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
    memcpy(&priv->tuner_control.ext_dm_config, &nim_cfg->ext_dm_config, sizeof(struct EXT_DM_CONFIG));
    priv->tuner_control.tuner_id = nim_cfg->tuner_id;

	priv->tuner_control.config_data.memory_size=__G_ALI_MM_NIM_PARAM_BUF_SIZE;
	priv->tuner_control.config_data.memory=__G_ALI_MM_NIM_PARAM_BUF_ADDR;

   S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,memory_size=0x%x!\n",__FUNCTION__,__LINE__,priv->tuner_control.config_data.memory_size);
   
    //get tuner IF freq.  added by David.Deng @ 2007.12.12
    priv->g_tuner_if_freq = priv->tuner_control.tuner_config.w_tuner_if_freq;
    //0:ISDBT_TYPE, 1:DVBT_TYPE, 2:DVBT2_TYPE, 3:DVBT2-COMBO, 4...
    priv->cofdm_type = priv->tuner_control.config_data.flag & 0x00000007 ;
	

    if (priv->tuner_control.work_mode == NIM_COFDM_SOC_MODE)
    {
        priv->base_addr = S3821_COFDM_SOC_BASE_ADDR;
    }
    else
    {
        priv->base_addr = priv->tuner_control.ext_dm_config.i2c_base_addr;
    }

    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,flag=%d,i2c_type=%d,i2c_addr=%x\n", __FUNCTION__, __LINE__,
		                              priv->tuner_control.config_data.flag,
		                              priv->tuner_control.tuner_config.i2c_type_id,
		                              priv->tuner_control.tuner_config.c_tuner_base_addr);
	
	p_io_func = tuner_setup(NIM_DVBT,nim_cfg->tuner_id);
	if(NULL != p_io_func)
	{
		priv->tuner_control.nim_tuner_init = p_io_func->pf_init; //(dvbt_tuner_init_callback*)
		priv->tuner_control.nim_tuner_control = p_io_func->pf_control;
		priv->tuner_control.nim_tuner_status = p_io_func->pf_status;
		priv->tuner_control.nim_tuner_command = p_io_func->pf_command;
		priv->tuner_control.nim_tuner_close = p_io_func->pf_close;
		
	}
	
}


/*****************************************************************************
* INT32 ali_m3281_nim_hw_initialize(struct nim_device *dev)
* Description: S3202 open
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/

static INT32 ali_m3821_nim_hw_initialize(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
    struct nim_s3821_private *priv = dev->priv;
	
    //joey, 20140228. to refine tune the dvbt2 only search function.
    nim_s3821_set_config(dev,nim_cfg);
	
	priv->search_t2_only = 0;
    // tuner power on
	nim_s3821_tuner_ioctl(dev, NIM_TUNER_POWER_CONTROL, FALSE);

	nim_s3821_reset();

    //joey, 20130624, for the init code as a function.
    nim_s3821_init_config(dev);

    nim_flag_create(&priv->flag_lock);
  	nim_flag_set(&priv->flag_lock,NIM_3821_SCAN_END); 

	priv->close_flag =0;
	
    nim_s3821_task_init(dev);
	
    nim_s3821_hw_init(dev);

    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,end\n", __FUNCTION__, __LINE__);

    return SUCCESS;
}

static INT32 ali_m3821_tuner_adaption(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
	struct nim_s3821_private *priv = dev->priv;
	UINT8 data = 0;
	
	nim_s3821_set_config(dev,nim_cfg);

    if (SUCCESS == nim_s3821_read(dev, 
		                        priv->tuner_control.tuner_config.c_tuner_base_addr, 
		                        &data, 
		                        1))
    {	
		S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,adaption success!,i2c_type=%d,i2c_addr=0x%x\n", 
			      __FUNCTION__, __LINE__,
			      priv->tuner_control.tuner_config.i2c_type_id,
			      priv->tuner_control.tuner_config.c_tuner_base_addr);

		return SUCCESS;
    }

   return -1;
	
}

static int ali_m3821_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    struct nim_device *dev = file->private_data;
    struct nim_s3821_private *priv = dev->priv;
    int ret = 0;

    switch(cmd)
    {
	case ALI_NIM_TUNER_SELT_ADAPTION_C:
    	{
	        struct ali_nim_mn88436_cfg nim_param;

	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
	      	ret = ali_m3821_tuner_adaption(dev, &nim_param);

		break;
	}
    	case ALI_NIM_HARDWARE_INIT_T:
	{
	        struct ali_nim_mn88436_cfg nim_param;

	        memset((void*)&nim_param,0,sizeof(struct ali_nim_mn88436_cfg));
	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
				
	        ret = ali_m3821_nim_hw_initialize(dev, &nim_param);

	        break;
	}
	case ALI_NIM_STOP_AUTOSCAN:
		priv->autoscan_stop_flag = parg;
	break;
		
	case ALI_NIM_CHANNEL_CHANGE:
	{
	        NIM_CHANNEL_CHANGE_T nim_param;

	        S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,nim_param.fec=%d\n", __FUNCTION__, __LINE__, nim_param.fec);

		memset((void*)&nim_param,0,sizeof(NIM_CHANNEL_CHANGE_T));
			
	        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
	        if (DVBT2_TYPE == priv->cofdm_type)
	        {
				nim_s3821_dvbt2_channel_change(dev, &nim_param);
	        }
			else if (DVBT2_COMBO == priv->cofdm_type)
	        {
	        //joey, 20140228. to refine tune the dvbt2 only search function.
			if ((USAGE_TYPE_CHANCHG != nim_param.usage_type) && (1 == priv->search_t2_only))
			{
				nim_s3821_dvbt2_channel_change(dev, &nim_param);
			}
			else
			{
	       			nim_s3821_combo_channel_change(dev, &nim_param);
			}
		}
	        else if (DVBT_TYPE == priv->cofdm_type)
	        {
	            nim_s3821_dvbt_isdbt_channel_change(dev, &nim_param);
	        }
	        else if (ISDBT_TYPE == priv->cofdm_type)
	        {
	            nim_s3821_dvbt_isdbt_channel_change(dev, &nim_param);
	        }

			
	        if(copy_to_user((NIM_CHANNEL_CHANGE_T *)parg, &nim_param, sizeof(NIM_CHANNEL_CHANGE_T))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
			
	        break;
	}

	case ALI_NIM_DRIVER_T2_SIGNAL_ONLY:
   	{
		UINT32 t2_flag = 0;
		/*
		if(copy_from_user(&t2_flag, parg, sizeof(int))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
		*/
		/*a value from user space*/
        	t2_flag = parg;

		S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,t2_flag=%d\n", __FUNCTION__, __LINE__, t2_flag);
		
		priv->search_t2_only = t2_flag;	
		
		break;
	}
	case ALI_NIM_GET_LOCK_STATUS:
    	{
		UINT8 lock = 0;
        	nim_s3821_get_lock(dev, &lock);
		ret = lock;
        	break;
    	}

	case ALI_NIM_READ_AGC:
	{	
		UINT8 agc = 0;

		nim_s3821_get_agc(dev, &agc);
		ret = agc;
		break;
	}
	case ALI_NIM_LOG_LEVEL:
	{
        	UINT32 log_level= 0;

		if(copy_from_user(&log_level, parg, sizeof(int))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
       		set_log_level(log_level);
		ret=0;
	
		break;
	}		
	case ALI_NIM_READ_SNR:
	{
		UINT8 snr = 0;
		
		nim_s3821_get_snr(dev, &snr);
		ret=snr;
		
		break;
	}
	case ALI_NIM_READ_CODE_RATE:
	{
        	UINT8 code_rate = 0;

		nim_s3821_get_code_rate(dev, &code_rate);

		ret=code_rate;
		break;
	}
	case ALI_NIM_READ_QPSK_BER:
	{
		UINT32 ber = 0;
        
		nim_s3821_get_ber(dev, &ber);
        	ret = ber;

		break;
	}
	case ALI_NIM_READ_FREQ:
    	{
        	UINT32 freq = 0;
     
        	nim_s3821_get_freq(dev, &freq);
        	ret = freq;
        	break;
    	}
		
    	case ALI_NIM_REG_RW:
    	{
        	UINT8 reg_rw_cmd[16] ={0};

        	if(copy_from_user(reg_rw_cmd, (UINT8 *)parg, 16)>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}			
        	if (1 == reg_rw_cmd[0]) // Register Read
        	{
        		ret = nim_s3821_read(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
			if(copy_to_user((UINT8 *)parg, reg_rw_cmd, 16)>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				return -EFAULT;
			}
							
        }
        else if (2 == reg_rw_cmd[0]) // Register Write
        {
		ret = nim_s3821_write(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
        }
	
        break;
	}
	case ALI_NIM_REG_RW_EXT:
	{
	        struct reg_rw_cmd_t
	        {
	            UINT32 offset_addr;
	            UINT8 reg_rw_cmd[16];
	        };

	        struct reg_rw_cmd_t reg_param;

	        if(copy_from_user(&reg_param, (struct reg_rw_cmd_t *)parg, sizeof(struct reg_rw_cmd_t))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}			
	        if (1 == reg_param.reg_rw_cmd[0]) // Register Read
	        {
			ret = nim_s3821_read(dev,reg_param.offset_addr, &(reg_param.reg_rw_cmd[2]), reg_param.reg_rw_cmd[1]);
			if(copy_to_user(((struct reg_rw_cmd_t *)parg)->reg_rw_cmd, (reg_param.reg_rw_cmd), 16)>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				return -EFAULT;
			}					
	        }
	        else if (2 == reg_param.reg_rw_cmd[0]) // Register Write
	        {
	            	ret = nim_s3821_write(dev,reg_param.offset_addr, &(reg_param.reg_rw_cmd[2]), reg_param.reg_rw_cmd[1]);
	        }
	        break;
	    }
	case ALI_NIM_ADC2MEM_START:
    	{
    	
	        S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,19\n", __FUNCTION__, __LINE__);
	        ret = nim_s3821_adc2dma_func_start(dev, parg);
	       
	        break;
    	}
	case ALI_NIM_ADC2MEM_STOP:
	{
        	S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,20\n", __FUNCTION__, __LINE__);
        	ret = nim_s3821_adc2dma_func_stop(dev, parg);
        	break;
    	}
    	case ALI_NIM_ADC2MEM_SEEK_SET:
    	{
        	S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,21\n", __FUNCTION__, __LINE__);
#if 0
	        g_s3821_adc2dma_st_addr = (UINT8 *)((UINT32)((g_var_adc2dma_addr+0xf) & 0xfffffff0));//plus 15, now is for 16-byte align issue.
	        (*((UINT32 *)param)) = (((g_var_adc2dma_len) >> 13) * 8192);
	        g_s3821_adc2dma_read_offset = 0;
	        ret = SUCCESS;
#endif		
        	break;
    	}
	case ALI_NIM_ADC2MEM_READ_8K:
	{
	        S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,22\n", __FUNCTION__, __LINE__);

	        //S3202_PRINTF("copy to user: 0x%08x, from: 0x%08x\n", parg,  g_rx_buf_start_addr+g_rx_buf_cur);
	        if(copy_to_user((UINT8 *)parg, g_s3821_adc2dma_st_addr + g_s3821_adc2dma_read_offset, 0x2000)>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}				
	        g_s3821_adc2dma_read_offset += 0x2000;

	        break;
	}
	case ALI_NIM_DRIVER_SET_RESET_CALLBACK:
    	
        	S3821_PRINTF(NIM_LOG_DBG,"[%s] line = %d,wakeup!\n", __FUNCTION__, __LINE__);
        	if (priv->tuner_control.nim_tuner_control != NULL)
        	{
            		priv->tuner_control.nim_tuner_control(priv->tuner_id, NIM_TUNER_SET_STANDBY_CMD, 1, 0, 0, 0);
        	}
	
        	break;
	case ALI_NIM_TURNER_SET_STANDBY:
    	
        	S3821_PRINTF(NIM_LOG_DBG,"[%s] line = %d,standby!\n", __FUNCTION__, __LINE__);
	        if (priv->tuner_control.nim_tuner_control != NULL)
	        {
	        	priv->tuner_control.nim_tuner_control(priv->tuner_id, NIM_TUNER_SET_STANDBY_CMD, 0, 0, 0, 0);
	        }
	
        	break;
	case ALI_NIM_DRIVER_GET_DEMOD_LOCK_MODE:
		ret = TRUE;
	break;
	case ALI_NIM_DRIVER_GET_FFT_MODE:
		ret = priv->s3821_cur_channel_info.mode;
		break;  
	case ALI_NIM_DRIVER_GET_MODULATION:
		ret = priv->s3821_cur_channel_info.modulation;
		break;
	case ALI_NIM_GET_GUARD_INTERVAL:
		 ret = priv->s3821_cur_channel_info.guard;
		 break;
	case ALI_NIM_GET_SPECTRUM_INV:
		 ret = priv->s3821_cur_channel_info.spectrum;
		 break;
	case ALI_NIM_TURNER_SET_LT:
	{
			
		S3821_PRINTF(NIM_LOG_DBG,"[%s] line = %d, Loopthrough_flag =%d!\n", __FUNCTION__, __LINE__,parg);

		if (priv->tuner_control.nim_tuner_control != NULL)
		{
			priv->tuner_control.nim_tuner_control(priv->tuner_id, NIM_TUNER_SET_LOOPTHROUGH_CMD, parg, 0, 0, 0);
		}
	}
        break;
    	default:
	{
        	S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,23\n", __FUNCTION__, __LINE__);
        	ret = -ENOIOCTLCMD;
        	break;
    	}
    }

    return ret;
}

static RET_CODE ali_m3821_nim_release(struct inode *inode, struct file *file)
{
    UINT8 data = 0;
    INT32 ret = SUCCESS;

    struct nim_device *dev = file->private_data;
    struct nim_s3821_private *priv = dev->priv;

   // tuner power off
    nim_s3821_tuner_ioctl(dev, NIM_TUNER_POWER_CONTROL, TRUE);

    nim_s3821_read(dev, 0x00, &data, 1);
    data |= 0x80;
    nim_s3821_write(dev, 0x00, &data, 1);

	priv->close_flag =1;
	

    if (dev->workqueue)
    {
        flush_workqueue(dev->workqueue);
        destroy_workqueue(dev->workqueue);
        dev->workqueue = NULL;
    }

    nim_flag_del(&priv->flag_lock);

    return ret;
}



static RET_CODE ali_m3821_nim_open(struct inode *inode, struct file *file)
{
    struct nim_s3821_private *priv = ali_m3821_nim_priv; //priv->priv;

    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);

    //PRINTK_INFO("malloc priv OK: %08x\n", priv);
    ali_m3821_nim_dev.priv = (void *)priv;
    file->private_data = (void *) & ali_m3821_nim_dev;



	
    return RET_SUCCESS;
}



static struct file_operations ali_m3821_nim_fops =
{
    .owner		= THIS_MODULE,
    .write		= NULL,
    .unlocked_ioctl	= ali_m3821_nim_ioctl,
    .open		= ali_m3821_nim_open,
    .release	= ali_m3821_nim_release,
};



static int __devinit ali_m3821_nim_init(void)
{
    INT32 ret = 0;
    dev_t devno;

    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);
    ali_m3821_nim_priv = kmalloc(sizeof(struct nim_s3821_private), GFP_KERNEL);
    if (!ali_m3821_nim_priv)
    {
		return -ENOMEM;
    }	
    memset(ali_m3821_nim_priv, 0, sizeof(struct nim_s3821_private));
    mutex_init(&ali_m3821_nim_priv->i2c_mutex);
	ali_m3821_nim_priv->flag_lock.flagid_rwlk = __RW_LOCK_UNLOCKED(ali_m3821_nim_priv->flagid_rwlk);
	
    ret = alloc_chrdev_region(&devno, 0, 1, ALI_NIM_DEVICE_NAME);
    if (ret < 0)
    {
        printk("[NIMTRACE] Alloc device region failed, err: %d.\n", (int)ret);
        return ret;
    }


    cdev_init(&ali_m3821_nim_dev.cdev, &ali_m3821_nim_fops);
    ali_m3821_nim_dev.cdev.owner = THIS_MODULE;
    ali_m3821_nim_dev.cdev.ops = &ali_m3821_nim_fops;
    ret = cdev_add(&ali_m3821_nim_dev.cdev, devno, 1);
    if (ret)
    {
        printk("[NIMTRACE] Alloc NIM device failed, err: %d.\n", (int)ret);
        goto error1;
    }

    S3821_PRINTF(NIM_LOG_DBG,"register NIM device end.\n");

    ali_m3821_nim_class = class_create(THIS_MODULE, "ali_m3821_nim_class");

    if (IS_ERR(ali_m3821_nim_class))
    {
        ret = PTR_ERR(ali_m3821_nim_class);

        goto error2;
    }

    ali_m3821_nim_dev_node = device_create(ali_m3821_nim_class, NULL, devno, &ali_m3821_nim_dev,
                                           "ali_s3821_nim0");
    if (IS_ERR(ali_m3821_nim_dev_node))
    {
        printk(KERN_ERR "device_create() failed!\n");

        ret = PTR_ERR(ali_m3821_nim_dev_node);

        goto error3;
    }

    return ret;

error3:
    class_destroy(ali_m3821_nim_class);
error2:
    cdev_del(&ali_m3821_nim_dev.cdev);
error1:
    mutex_destroy(&ali_m3821_nim_priv->i2c_mutex);
    kfree(ali_m3821_nim_priv);

    return ret;
}

static void __exit ali_m3821_nim_exit(void)
{
    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);
    if (ali_m3821_nim_dev_node != NULL)
    {
		device_del(ali_m3821_nim_dev_node);
    }	

    if (ali_m3821_nim_class != NULL)
    {
		class_destroy(ali_m3821_nim_class);
    }	
    cdev_del(&ali_m3821_nim_dev.cdev);
    mutex_destroy(&ali_m3821_nim_priv->i2c_mutex);
    kfree(ali_m3821_nim_priv);
}



module_init(ali_m3821_nim_init);
module_exit(ali_m3821_nim_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kent");
MODULE_DESCRIPTION("Ali M3821 full NIM driver");





