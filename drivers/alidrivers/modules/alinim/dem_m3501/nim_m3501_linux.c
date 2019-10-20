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

#include "nim_m3501.h"
#include "../tun_common.h"
#include <linux/device.h>
#include <linux/platform_device.h>
#include <ali_soc.h>

#define TWO_TUNER_SUPPORT
#define MAX_TUNER_SUPPORT_NUM 	4
#define ALI_NIM_DEVICE_NAME 	"ali_nim_m3501"
//#define SUPPORT_PLATFORM_DEVICE

#ifndef SUPPORT_PLATFORM_DEVICE

struct nim_device 				ali_m3501_nim_dev[MAX_TUNER_SUPPORT_NUM];
struct nim_s3501_private 		*ali_m3501_nim_priv[MAX_TUNER_SUPPORT_NUM] = {NULL, NULL,NULL,NULL};
struct device 					*ali_m3501_nim_dev_node[MAX_TUNER_SUPPORT_NUM] = {NULL};

#endif
struct class 					*ali_m3501_nim_class =NULL;

static INT32                    g_is_rest = 1;
//static UINT32                   init_nu = 0;
bool                            class_create_flag = true; 
static char nim_workq[MAX_TUNER_SUPPORT_NUM][5] =
{
    "nim0", "nim1", "nim2", "nim3"
};

static char as_workq[MAX_TUNER_SUPPORT_NUM][12] =
{
    "as_workq0", "as_workq1","as_workq2", "as_workq3"
};

static int ali_m3501_nim_open(struct inode *inode, struct file *file);

static void nim_s3501_task_open(struct work_struct *nim_work)
{
    struct nim_device 				*dev;
    dev = container_of((void *)nim_work, struct nim_device, work);

    S3501_PRINTF(NIM_LOG_DBG,"[kangzh]line=%d,%s enter!\n", __LINE__, __FUNCTION__);

    nim_s3501_task((UINT32)dev, 0);


}

static void nim_s3501_autoscan_open(struct work_struct *work)
{
    NIM_AUTO_SCAN_T                    *pst_auto_scan;
    struct nim_device                 *dev;
    struct nim_s3501_private         *priv;

    dev = container_of((void *)work, struct nim_device, as_work);
    priv = dev->priv;
    pst_auto_scan = &priv->as_info;

    nim_s3501_autoscan(dev, pst_auto_scan);
}

static INT32 nim_s3501_task_init(struct nim_device *dev)
{
    UINT8 dev_idx = 0;
    struct nim_s3501_private 	*priv = dev->priv;

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

	dev_idx = priv->dev_idx;

    dev->workqueue = create_workqueue(nim_workq[dev_idx]);

    if (!(dev->workqueue))
    {
        S3501_PRINTF(NIM_LOG_DBG,"Failed to allocate work queue\n");
        return -1;
    }

    dev->autoscan_work_queue = create_workqueue(as_workq[dev_idx]);
    if (!(dev->autoscan_work_queue))
    {
        S3501_PRINTF(NIM_LOG_DBG,"%s in:Failed to create nim autoscan workequeue!\n",__FUNCTION__);
        destroy_workqueue(dev->workqueue);
        return -1;
    }
    init_waitqueue_head(&priv->as_sync_wait_queue);
    priv->work_alive = 1;
    priv->ul_status.s3501_chanscan_stop_flag = 0;
    INIT_WORK(&dev->work, nim_s3501_task_open);
    INIT_WORK(&dev->as_work, nim_s3501_autoscan_open);
    queue_work(dev->workqueue, &dev->work);

    return SUCCESS;
}

#if 0
INT32 nim_s3501_autoscan_signal_input(struct nim_device *dev, UINT8 s_case)
{
    struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	
    fd_dmx = sys_open((const char __user *)"/dev/ali_m36_dmx_0", O_RDWR, 0);
    if (fd_dmx< 0)
    {
		nim_print(KERN_WARNING "Warning: Unable to open an dmx.\n");
		return -1;
    }	

    switch (s_case)
    {
    case NIM_SIGNAL_INPUT_OPEN:
        sys_ioctl(fd_dmx, ALI_DMX_IO_SET_BYPASS_MODE, (UINT32) (priv->ul_status.adcdata));//Set bypass mode
        break;
    case NIM_SIGNAL_INPUT_CLOSE:
        sys_ioctl(fd_dmx, ALI_DMX_IO_CLS_BYPASS_MODE, (UINT32)(NULL));//Clear bypass mode.
        break;
    }

    if (sys_close(fd_dmx))
    {
        nim_print("%s in:Error closing the dmx_hdl.\n", __FUNCTION__);
    }
    return SUCCESS;
}
#endif

static void nim_m3501_set_config(struct nim_device *dev, struct ali_nim_m3501_cfg *nim_cfg)
{
    struct nim_s3501_private 	*priv = dev->priv;
    TUNER_IO_FUNC               *p_io_func=NULL;

    priv->tuner_config_data.qpsk_config = nim_cfg->qpsk_config;
    priv->tuner_config_data.recv_freq_high = nim_cfg->recv_freq_high;
    priv->tuner_config_data.recv_freq_low = nim_cfg->recv_freq_low;
    priv->ext_dm_config.i2c_base_addr = nim_cfg->demod_i2c_addr;
    priv->ext_dm_config.i2c_type_id = nim_cfg->demod_i2c_id;
    priv->tuner_config.c_tuner_base_addr = nim_cfg->tuner_i2c_addr;
    priv->tuner_config.i2c_type_id = nim_cfg->tuner_i2c_id;
    priv->i2c_type_id = nim_cfg->tuner_i2c_id;
	/*printk("tuner id = %x addr = %x,demod id = %d addr = %x  qpsk = %x low = %d high = %d\n",
		priv->tuner_config.i2c_type_id,priv->tuner_config.c_tuner_base_addr,
		priv->ext_dm_config.i2c_type_id, priv->ext_dm_config.i2c_base_addr,
		priv->tuner_config_data.qpsk_config,priv->tuner_config_data.recv_freq_low,
		priv->tuner_config_data.recv_freq_high
		);*/
    S3501_PRINTF(NIM_LOG_DBG,"[%s] line=%d,i2c_type=%d,i2c_addr=0x%x\n", 
	      __FUNCTION__, __LINE__,
	      priv->tuner_config.i2c_type_id,
	      priv->tuner_config.c_tuner_base_addr);
	
	p_io_func = tuner_setup(NIM_DVBS,nim_cfg->tuner_id);
	priv->is_m3031 = ((M3031 == nim_cfg->tuner_id) ? 1:0); 
	
	if(NULL != p_io_func)
	{
		priv->nim_tuner_init = (dvbs_tuner_init_callback)(p_io_func->pf_init);
		priv->nim_tuner_control = p_io_func->pf_control;
		priv->nim_tuner_status = p_io_func->pf_status;
		priv->nim_tuner_command = p_io_func->pf_command;
		priv->nim_tuner_gain =  p_io_func->pf_gain;
		priv->nim_tuner_close = p_io_func->pf_close;
	}
	
}

static INT32 ali_m3501_tuner_adaption(struct nim_device *dev, struct ali_nim_m3501_cfg *nim_cfg)
{		
	nim_m3501_set_config(dev,nim_cfg);

#if 0
	struct nim_s3501_private *priv = dev->priv;
	UINT8 data = 0;
    if (SUCCESS != nim_i2c_read(priv->tuner_config.i2c_type_id, 
		                        priv->tuner_config.c_tuner_base_addr, 
		                        &data, 
		                        1))
    {	
		S3501_PRINTF(NIM_LOG_DBG,"[%s] line=%d,adaption success!,i2c_type=%d,i2c_addr=0x%x\n", 
			      __FUNCTION__, __LINE__,
			      priv->tuner_config.i2c_type_id,
			      priv->tuner_config.c_tuner_base_addr);

		return -1;
    }
#endif

   return SUCCESS;
	
}

INT32 ali_nim_m3501_hw_initialize(struct nim_device *dev, struct ali_nim_m3501_cfg *nim_cfg)
{
    INT32 	                    ret = 0;	
    struct nim_s3501_private 	*priv = dev->priv;

    S3501_PRINTF(NIM_LOG_DBG,"[kent] line=%d,%s enter!\n", __LINE__, __FUNCTION__);

	if(priv->nim_init)
    	 return RET_SUCCESS;

    nim_m3501_set_config(dev,nim_cfg);
   
   	priv->diseqc_info.diseqc_type = 0;
    priv->diseqc_info.diseqc_port = 0;
    priv->diseqc_info.diseqc_k22 = 0; 

    if ((priv->tuner_config_data.qpsk_config & M3501_POLAR_REVERT) == M3501_POLAR_REVERT) //bit4: polarity revert.
    {
		priv->diseqc_info.diseqc_polar = 2;//LNB_POL_V;
    }	
    else //default usage, not revert.
    {
		priv->diseqc_info.diseqc_polar = 1;//LNB_POL_H;
    }
	/*printk("%s tuner id = %x addr = %x demod id = %x addr = %x\n",__FUNCTION__,
		priv->tuner_config.i2c_type_id,priv->tuner_config.c_tuner_base_addr,
		priv->ext_dm_config.i2c_type_id, priv->ext_dm_config.i2c_base_addr
		);*/
    priv->ul_status.m_enable_dvbs2_hbcd_mode = 0;
    priv->ul_status.m_dvbs2_hbcd_enable_value = 0x7f;
    priv->ul_status.nim_s3501_sema = OSAL_INVALID_ID;
    priv->ul_status.s3501_autoscan_stop_flag = 0;
    priv->ul_status.s3501_chanscan_stop_flag = 0;
    priv->ul_status.old_ber = 0;
    priv->ul_status.old_per = 0;
    priv->ul_status.m_hw_timeout_thr = 0;
    priv->ul_status.old_ldpc_ite_num = 0;
    priv->ul_status.c_rs = 0;
    priv->ul_status.phase_err_check_status = 0;
    priv->ul_status.s3501d_lock_status = NIM_LOCK_STUS_NORMAL;
    priv->ul_status.m_s3501_type = 0x00;
    priv->ul_status.m_setting_freq = 123;
    priv->ul_status.m_err_cnts = 0x00;
    priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_NORMAL;
    priv->tsk_status.m_task_id = 0x00;
    priv->t_param.t_aver_snr = -1;
    priv->t_param.t_last_iter = -1;
    priv->t_param.t_last_snr = -1;
    priv->t_param.t_snr_state = 0;
    priv->t_param.t_snr_thre1 = 256;
    priv->t_param.t_snr_thre2 = 256;
    priv->t_param.t_snr_thre3 = 256;
    priv->t_param.t_dynamic_power_en = 0;
    priv->t_param.t_phase_noise_detected = 0;
    priv->t_param.t_reg_setting_switch = 0x0f;
    priv->t_param.t_i2c_err_flag = 0x00;
    priv->flag_lock.flag_id = OSAL_INVALID_ID;


    priv->blscan_mode = NIM_SCAN_SLOW;

    if (nim_s3501_i2c_open(dev))
    {
        S3501_PRINTF(NIM_LOG_DBG,"%s() return %d\n", __FUNCTION__, __LINE__);
        return S3501_ERR_I2C_NO_ACK;
    }

    // Initial the QPSK Tuner
    if (priv->nim_tuner_init != NULL)
    {
        S3501_PRINTF(NIM_LOG_DBG,"[%s] Initial the Tuner \n", __FUNCTION__);
		
        if (priv->nim_tuner_init(&priv->tuner_index, &(priv->tuner_config)) != SUCCESS)
        {
            S3501_PRINTF(NIM_LOG_DBG,"[%s]Error: Init Tuner Failure!\n", __FUNCTION__);

            if (nim_s3501_i2c_close(dev))
            {
				S3501_PRINTF(NIM_LOG_DBG,"%s ,nim_s3501_i2c_close return %d\n", __FUNCTION__, __LINE__);
				return S3501_ERR_I2C_NO_ACK;
            }	

            return ERR_NO_DEV;
        }
		
		S3501_PRINTF(NIM_LOG_DBG, "[%s %d]priv->tuner_index=%d\n", __FUNCTION__, __LINE__, priv->tuner_index);
        priv->tuner_opened = 1;
    }

    if (nim_s3501_i2c_close(dev))
    {
		S3501_PRINTF(NIM_LOG_DBG,"%s ,nim_s3501_i2c_close return %d\n", __FUNCTION__, __LINE__);
		return S3501_ERR_I2C_NO_ACK;
    }	


    nim_s3501_get_type(dev);

    if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501A && 			// Chip 3501A
            (priv->tuner_config_data.qpsk_config & 0xc0) == M3501_2BIT_MODE)	//TS 2bit mode
    {
        //for M3606+M3501A full nim ssi-2bit patch, auto change to 1bit mode.
        priv->tuner_config_data.qpsk_config &= 0x3f; // set to TS 1 bit mode
    }


    S3501_PRINTF(NIM_LOG_DBG,"[%s]    Enter fuction nim_s3501_open\n", __FUNCTION__);
    nim_s3501_set_acq_workmode(dev, NIM_OPTR_HW_OPEN);

    ret = nim_s3501_hw_check(dev);
    if (ret != SUCCESS)
    {
		S3501_PRINTF(NIM_LOG_DBG,"%s ,nim_s3501_hw_check return %d\n", __FUNCTION__, __LINE__);
		return ret;
    }	
    ret = nim_s3501_hw_init(dev);

    nim_s3501_after_reset_set_param(dev);

    nim_s3501_hbcd_timeout(dev, NIM_OPTR_HW_OPEN);

    nim_s3501_task_init(dev);

#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_create(&priv->flag_lock);
#endif
    S3501_PRINTF(NIM_LOG_DBG,"[%s]    Leave fuction\n", __FUNCTION__);
 		priv->nim_init = TRUE;
    return RET_SUCCESS;
}

/*****************************************************************************
*  Function Name: nim_m3501_get_sig_status(struct nim_device *dev)
*  Description:      get m3501_sig_status struct param data
*  Arguments:      nim_device
*  Return Value:   error return < 0  correct return = 0
*****************************************************************************/
INT32 nim_m3501_get_sig_status(struct nim_device *dev)
{

      struct nim_s3501_private *priv =  NULL;
	  INT32 ret = 0;

      if(NULL == dev)
	  {
	     return RET_FAILURE;
	  }	  
      priv =  dev->priv;
	  
	  if ((ret = nim_s3501_reg_get_code_rate(dev, (UINT8 *)&priv->m3501_sig_status.fec)) != SUCCESS)
	  {
		  S3501_PRINTF(NIM_LOG_DBG,"nim_m3501_get_sig_status: get fec error! %d\n", ret);
		  return RET_FAILURE;
	  }
	  
	  if((ret = nim_s3501_reg_get_roll_off(dev, (UINT8 *)&priv->m3501_sig_status.roll_off)) !=SUCCESS)
	  {
		  S3501_PRINTF(NIM_LOG_DBG,"nim_m3501_get_sig_status: get roll_off error! %d\n", ret);
		  return RET_FAILURE;
	  }

	  nim_s3501_reg_get_map_type(dev,(UINT8 *)&priv->m3501_sig_status.modulation);

      //get polar param in nim_s3501_set_polar function
	  
	  priv->m3501_sig_status.dvb_mode = 5;
      return SUCCESS;
}

static int ali_m3501_nim_release(struct inode *inode, struct file *file)
{
    struct nim_device *dev = file->private_data;
    struct nim_s3501_private *priv = dev->priv;

	if (priv->nim_used != 0)
		{
			priv->nim_used--;

			if(priv->nim_used)
			{
				S3501_PRINTF("[%s %d]enter,priv->nim_used=%d!\n", __FUNCTION__, __LINE__, priv->nim_used);
				return RET_SUCCESS;
			}
		}

	/*
	priv->nim_used--;

	if(priv->nim_used)
		return RET_SUCCESS;*/

    if (priv->tuner_opened && priv->nim_tuner_close)
    {
		priv->nim_tuner_close(priv->tuner_index);
    }	
    nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X90);
    nim_s3501_set_acq_workmode(dev, NIM_OPTR_HW_CLOSE);

    //g_work_alive = 0;
    priv->work_alive = 0;
    priv->ul_status.s3501_chanscan_stop_flag = 1;
	priv->ul_status.s3501_autoscan_stop_flag =1;
	//wake_up as_sync_wait_queue to make sure when auto_scan_work_queue in wait mode can nomal stop
	if(!priv->as_status)
	{
		priv->as_status |= 0x01;
        //Determines whether the priv->as_sync_wait_queue is initialized            
        if(NULL != priv->as_sync_wait_queue.task_list.next)                     
			wake_up_interruptible(&priv->as_sync_wait_queue);
	}
    if (dev->autoscan_work_queue)
    {
        flush_workqueue(dev->autoscan_work_queue);
        destroy_workqueue(dev->autoscan_work_queue);
        dev->autoscan_work_queue = NULL;
    }

    if (dev->workqueue)
    {
        flush_workqueue(dev->workqueue);
        destroy_workqueue(dev->workqueue);
        dev->workqueue = NULL;
    }

#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_del(&priv->flag_lock);
#endif
    priv->nim_init = FALSE;
	ali_destroy_kumsgq(priv->nim_kumsgq);
	priv->nim_kumsgq= NULL;
    return RET_SUCCESS;
}

static long ali_m3501_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    struct nim_device *dev = file->private_data;
    struct nim_s3501_private *priv = dev->priv;
    unsigned long arg = (unsigned long) parg;
    int ret = 0;
    INT32 curfreq = 0;
	UINT8 work_mode;
	mutex_lock(&priv->multi_process_mutex);
    switch (cmd)
    {
	 case ALI_NIM_TUNER_SELT_ADAPTION_S:	
     {
	 	struct ali_nim_m3501_cfg nim_param;

        if(copy_from_user(&nim_param, (struct ali_nim_m3501_cfg *)parg, sizeof(struct ali_nim_m3501_cfg))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
        ret = ali_m3501_tuner_adaption(dev, &nim_param);

		break;
	 }	
    case ALI_NIM_HARDWARE_INIT_S:
    {
        struct ali_nim_m3501_cfg nim_param;

        if(copy_from_user(&nim_param, (struct ali_nim_m3501_cfg *)parg, sizeof(struct ali_nim_m3501_cfg))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
			
        ret = ali_nim_m3501_hw_initialize(dev, &nim_param);
        break;
    }
    case ALI_NIM_SET_POLAR:
    {
        UINT32 polar_param= 0;
        //arm no support get_user. 
	if(copy_from_user(&polar_param, (UINT32 *)parg, sizeof(UINT32))>0)
	{
		printk("%s error line%d\n", __FUNCTION__, __LINE__);
		// Invalid user space address
		mutex_unlock(&priv->multi_process_mutex);
		return -EFAULT;
	}
        ret = nim_s3501_set_polar(dev, polar_param);
        break;
    }
    case ALI_NIM_LOG_LEVEL:
    {
        UINT32 log_level= 0;

	if(copy_from_user(&log_level, (UINT32 *)parg, sizeof(UINT32))>0)
	{
		printk("%s error line%d\n", __FUNCTION__, __LINE__);
		// Invalid user space address
		mutex_unlock(&priv->multi_process_mutex);
		return -EFAULT;
	}
        set_log_level(log_level);
	ret=0;
	
	break;
    }		
    case ALI_NIM_CHANNEL_CHANGE:
    {
        NIM_CHANNEL_CHANGE_T nim_param;
     
        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
        ret =  nim_s3501_channel_change(dev, nim_param.freq, nim_param.sym, nim_param.fec);
		break;
    }
    case ALI_NIM_GET_LOCK_STATUS:
    {
        UINT8 lock = 0;
     
        nim_s3501_get_lock(dev, &lock);
        ret = lock;
        break;
    }
    case ALI_NIM_READ_QPSK_BER:
    {
        UINT32 ber = 0;
       
        nim_s3501_get_ber(dev, &ber);
        ret = ber;
        break;
    }
    case ALI_NIM_READ_RSUB:
    {
        UINT32 per = 0;
       
        nim_s3501_get_per(dev, &per);
        ret = per;
        break;
    }
    case ALI_NIM_READ_AGC:
    {
        UINT8 agc = 0;
       
        nim_s3501_get_agc(dev, &agc);
        ret = agc;
        break;
    }
	case ALI_NIM_GET_RF_LEVEL: //uint 0.1dbm,minnum:-80dbm
	{
		INT8 rf_value = 0;
		INT32 tmp = 0;
		nim_s3501_get_agc_dbm(dev,&rf_value);
		if(rf_value < 0)
		{
			rf_value = 0 - rf_value;
		}
		tmp = rf_value * 10;//transfer dbm to 0.1dbm
		ret = tmp;
		break;
	}
	/*uint 0.01db
	   maximum:
	   QPSK:14db
	   8PSK:22db
	   16APSK:22db
	*/
	case ALI_NIM_GET_CN_VALUE:
	{
		INT32 snr_db = 0;
		nim_s3501_get_snr_db(dev, &snr_db);
		ret = snr_db;
		break;
	}
    case ALI_NIM_READ_SNR:
    {
        UINT8 snr = 0;
        
        nim_s3501_get_snr(dev, &snr);
        ret = snr;
        break;
    }
    case ALI_NIM_READ_SYMBOL_RATE:
    {
        UINT32 sym = 0;
        
        nim_s3501_reg_get_symbol_rate(dev, &sym);
        ret = sym;
        break;
    }
    case ALI_NIM_READ_FREQ:
    {
        //UINT32 freq = 0;
     
        //nim_s3501_reg_get_freq(dev, &freq);
        //ret = freq;
        
		ret = nim_s3501_get_curfreq(dev);
        break;
    }
    case ALI_NIM_READ_CODE_RATE:
    {
        UINT8 fec = 0;
     
        nim_s3501_reg_get_code_rate(dev, &fec);
        ret = fec;
        break;
    }
    case ALI_NIM_AUTO_SCAN:          /* Do AutoScan Procedure */
    {
	NIM_AUTO_SCAN_T as_load;
		
        flush_workqueue(dev->autoscan_work_queue);
        if(copy_from_user(&as_load, (NIM_AUTO_SCAN_T *)parg, sizeof(NIM_AUTO_SCAN_T))>0)
	{
		printk("%s error line%d\n", __FUNCTION__, __LINE__);
		// Invalid user space address
		mutex_unlock(&priv->multi_process_mutex);
		return -EFAULT;
	}
			
        priv->yet_return = FALSE;

        priv->as_info.sfreq = as_load.sfreq;
        priv->as_info.efreq = as_load.efreq;
        priv->as_info.unicable = as_load.unicable;
        priv->as_info.fub = as_load.fub;
        priv->as_info.callback = dvbs_as_cb2_ui;
        ret = queue_work(dev->autoscan_work_queue, &dev->as_work);
        break;
    }
    case ALI_NIM_STOP_AUTOSCAN:

        priv->ul_status.s3501_autoscan_stop_flag = arg;
        break;
    case ALI_NIM_DISEQC_OPERATE:
    {
        struct ali_nim_diseqc_cmd dis_cmd;

        if(copy_from_user(&dis_cmd, (struct ali_nim_diseqc_cmd *)parg, sizeof(struct ali_nim_diseqc_cmd))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
			
        switch(dis_cmd.diseqc_type)
        {
	        case 1:
	            ret = nim_s3501_di_seq_c_operate(dev, dis_cmd.mode, dis_cmd.cmd, dis_cmd.cmd_size);
	            break;
	        case 2:
	            ret = nim_s3501_di_seq_c2x_operate(dev, dis_cmd.mode, \
	                                             dis_cmd.cmd, dis_cmd.cmd_size, dis_cmd.ret_bytes, &dis_cmd.ret_len);
	            if(copy_to_user((struct ali_nim_diseqc_cmd *)parg, &dis_cmd, sizeof(struct ali_nim_diseqc_cmd))>0)
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
								
	            break;
	        default:
	            ret = -ENOIOCTLCMD;
	            break;
        }
        break;
    }

	case ALI_NIM_GET_KUMSGQ:
	{
		int flags = -1;
		int ret = -1;
		if(copy_from_user(&flags, (int *)parg, sizeof(int)))
		{
			printk("Err: copy_from_user\n");
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		ret  = ali_kumsgq_newfd(priv->nim_kumsgq, flags);
		if(ret> 0)
		{
			mutex_unlock(&priv->multi_process_mutex);
			return ret;	
		}	
	}
	break;

    case ALI_NIM_AS_SYNC_WITH_LIB:
    {
        priv->as_status |= 0x01;
        wake_up_interruptible(&priv->as_sync_wait_queue);
        break;
    }
    case ALI_NIM_DRIVER_GET_CUR_FREQ:

        switch (arg)
        {
        case NIM_FREQ_RETURN_SET:
			mutex_unlock(&priv->multi_process_mutex);
            return priv->ul_status.m_setting_freq;
        case NIM_FREQ_RETURN_REAL:
        default:
            curfreq = (INT32) nim_s3501_get_curfreq(dev);
			mutex_unlock(&priv->multi_process_mutex);
            return curfreq;
        }
    case ALI_NIM_TURNER_SET_STANDBY:

        if (nim_s3501_i2c_open(dev))
        {
        	mutex_unlock(&priv->multi_process_mutex);
            return S3501_ERR_I2C_NO_ACK;
        }
        if (priv->nim_tuner_control != NULL)
        {
            priv->nim_tuner_control(priv->tuner_index, NIM_TUNER_SET_STANDBY_CMD, 0);
        }
        if (nim_s3501_i2c_close(dev))
        {
        	mutex_unlock(&priv->multi_process_mutex);
            return S3501_ERR_I2C_NO_ACK;
        }
        break;
    case ALI_NIM_DRIVER_GET_ID:

        ret = priv->ul_status.m_s3501_type;
        break;
    case ALI_NIM_REG_RW:
        {
	        UINT8 reg_rw_cmd[16] ={0};
            //printk("[%s] line=%d,ret=%d\n",__FUNCTION__,__LINE__,ret);
		if(copy_from_user(reg_rw_cmd, (UINT8 *)parg, 16)>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}			
		if (1 == reg_rw_cmd[0]) // Register Read
		{
		    ret = nim_reg_read(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
		    if(copy_to_user((UINT8 *)parg, reg_rw_cmd, 16)>0)
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
								
		}
		else if (2 == reg_rw_cmd[0]) // Register Write
		{
				// printk("[%s] line=%d,reg_rw_cmd[3]=0x%x,len=%d\n",__FUNCTION__,__LINE__,reg_rw_cmd[3],reg_rw_cmd[2]);
		    ret = nim_reg_write(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
		}
        }
		break;
    case ALI_NIM_SET_LNB_POWER:
		break;
    case ALI_NIM_GET_LNB_POWER:
		break;
    case ALI_NIM_SET_LNB_FREQS:
		break;
	case ALI_NIM_GET_LNB_FREQS:
		break;
	case ALI_NIM_SET_LOOPTHRU:
		break;
	case ALI_NIM_GET_LOOPTHRU:
        	break;   
	case ALI_NIM_GET_SIG_STATUS://add by dennis on 2014-08-01
		{
			//printk("dennis add cmd = ALI_NIM_GET_SIG_STATUS printf!\n");
			nim_m3501_get_sig_status(dev);
			if(copy_to_user((UINT8 *)parg, &priv->m3501_sig_status, sizeof(priv->m3501_sig_status))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
		}
	  	break;
    case ALI_NIM_GET_QUALITY_INFO://add by dennis on 2014-08-04
		{
			//printk("dennis add cmd = ALI_NIM_GET_QUALITY_INFO printf!\n");

			struct nim_s3501_private *priv =  dev->priv;


			priv->m3501_quality_info.iA = 0;
			priv->m3501_quality_info.iB = 0;
			
			if(copy_to_user((UINT8 *)parg, &priv->m3501_quality_info, sizeof(priv->m3501_quality_info))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
	         }
		break;
	case ALI_NIM_DRIVER_SET_RESET_CALLBACK:

		priv->ul_status.m_pfn_reset_s3501 = (pfn_nim_reset_callback) parg;

		break;
	case ALI_NIM_DRIVER_SET_DEMOD_RESET:
		ret = nim_s3501_demod_restart(dev);
		break;
	case ALI_NIM_GET_WORK_MODE:
    {
        nim_s3501_reg_get_work_mode(dev,&work_mode);
		if(copy_to_user((UINT8 *)parg, &work_mode, sizeof(work_mode))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}

        break;
    }
    default:
		
        ret = -ENOIOCTLCMD;
		printk("[%s] line=%d,ret=%d",__FUNCTION__,__LINE__,ret);
        break;
    }
	mutex_unlock(&priv->multi_process_mutex);
    return ret;
}


int match(struct gpio_chip *chip, void *data)
{
    if (0 == strcmp(chip->label, data))
    {
        return 1;
    }
	return SUCCESS;
}

/*****************************************************************************
* INT32 ali_m3501_nim_open(struct nim_s3501_private *priv)
* Description: S3501 open
*
* Arguments:
*  Parameter1: struct nim_s3501_private *priv
*
* Return Value: INT32
*****************************************************************************/

static void nim_m3501_hwreset(void)
{
	struct gpio_chip *gpio_chip;	
    
    
#ifdef CONFIG_ARM
	int gpio_index = 77;	//gpio[77]
	int gpio_index2 = 7;	//gpio[7]
#else
	int gpio_index3 = 5;  //11;
#endif

	gpio_chip = gpiochip_find("m36", match);
#ifdef CONFIG_ARM

     //set gpio to high
	 gpio_chip->direction_output(gpio_chip,gpio_index,1);
	 gpio_chip->direction_output(gpio_chip,gpio_index2,1);
	 msleep(10);
	 //set gpio to low
	 gpio_chip->direction_output(gpio_chip,gpio_index,0);
	 gpio_chip->direction_output(gpio_chip,gpio_index2,0);

	 msleep(10);	 
	 //set gpio to high
	 gpio_chip->direction_output(gpio_chip,gpio_index,1);
	 gpio_chip->direction_output(gpio_chip,gpio_index2,1);
 
#else	 
     //set gpio to high
     gpio_chip->direction_output(gpio_chip,gpio_index3,1);
	 msleep(10);
     //set gpio to low
	 gpio_chip->direction_output(gpio_chip,gpio_index3,0);
	 msleep(10);	 
	 //set gpio to high
	 gpio_chip->direction_output(gpio_chip,gpio_index3,1);
#endif

}



static struct file_operations ali_m3501_nim_fops =
{
    .owner						= THIS_MODULE,
    .write						= NULL,
    .unlocked_ioctl				= ali_m3501_nim_ioctl,
    .open						= ali_m3501_nim_open,
    .release					= ali_m3501_nim_release,
};

static int ali_m3501_nim_open(struct inode *inode, struct file *file)
{

    UINT8 dev_idx = 0;
    
	if(!g_is_rest)
	{
		nim_m3501_hwreset();
		g_is_rest = 1;
	}

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter,dev_idx=%d!\n", __FUNCTION__, __LINE__, dev_idx);

	dev_idx = MINOR(inode->i_rdev);
	if(dev_idx  >= MAX_TUNER_SUPPORT_NUM)
    {
		S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,dev_idx set error!\n", __FUNCTION__, __LINE__);
		return ERR_FAILED;
	}
	//ali_m3501_nim_dev[dev_idx].priv->nim_used++;
	ali_m3501_nim_priv[dev_idx]->nim_used++;
    file->private_data = (void *)&ali_m3501_nim_dev[dev_idx];
	
	ali_m3501_nim_priv[dev_idx]->nim_kumsgq = ali_new_kumsgq();
	
	printk(KERN_EMERG "### dev_idx=%d nim_kumsgq=0x%x \r\n", dev_idx, (unsigned int)ali_m3501_nim_priv[dev_idx]->nim_kumsgq);
	if (!ali_m3501_nim_priv[dev_idx]->nim_kumsgq)
	{
		goto out0;
    }

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
    return RET_SUCCESS;
out0:
	WARN(1,"False to new nim[%d] kumsgq!!!!!!",dev_idx);
	return -EFAULT;
}

static int ali_m3501_nim_probe(struct platform_device *pdev)
{
    INT32 ret = 0;
	UINT8 i = 0;
    dev_t devno;

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);
	
	i2c_gpio_attach(2);  

	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, MAX_TUNER_SUPPORT_NUM, ALI_NIM_DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

    ali_m3501_nim_class = class_create(THIS_MODULE, "ali_m3501_nim_class");
    if (IS_ERR(ali_m3501_nim_class))
    {
        S3501_PRINTF(NIM_LOG_DBG,"[kangzh]line=%d,%s class_create error,back!\n", __LINE__, __FUNCTION__);
        ret = PTR_ERR(ali_m3501_nim_class);
        return ret;
    }

    for(i = 0; i < MAX_TUNER_SUPPORT_NUM; i++)
    {
        ali_m3501_nim_priv[i] = kmalloc(sizeof(struct nim_s3501_private), GFP_KERNEL);
        if (!ali_m3501_nim_priv[i])
        {
            S3501_PRINTF(NIM_LOG_DBG,"kmalloc failed!\n");
            return -ENOMEM;
        }
        comm_memset(ali_m3501_nim_priv[i], 0, sizeof(struct nim_s3501_private));
        mutex_init(&ali_m3501_nim_priv[i]->i2c_mutex);
        mutex_init(&ali_m3501_nim_priv[i]->tuner_open_mutex);
		mutex_init(&ali_m3501_nim_priv[i]->multi_process_mutex);
        ali_m3501_nim_priv[i]->flag_lock.flagid_rwlk = __RW_LOCK_UNLOCKED(ali_m3501_nim_priv[i]->flagid_rwlk);

        cdev_init(&ali_m3501_nim_dev[i].cdev, &ali_m3501_nim_fops);
        ali_m3501_nim_dev[i].cdev.owner = THIS_MODULE;
        ali_m3501_nim_dev[i].cdev.ops = &ali_m3501_nim_fops;

        ali_m3501_nim_dev[i].priv = (void *)ali_m3501_nim_priv[i];
        ali_m3501_nim_priv[i]->dev_idx = i;
		
        ret = cdev_add(&ali_m3501_nim_dev[i].cdev, devno + i, 1);
        if(ret)
        {
            S3501_PRINTF(NIM_LOG_DBG,"Alloc NIM device failed, err: %d.\n", (int)ret);
            mutex_destroy(&ali_m3501_nim_priv[i]->i2c_mutex);
            mutex_destroy(&ali_m3501_nim_priv[i]->tuner_open_mutex);
			mutex_destroy(&ali_m3501_nim_priv[i]->multi_process_mutex);
            kfree(ali_m3501_nim_priv[i]);
        }

        ali_m3501_nim_dev_node[i] = device_create(ali_m3501_nim_class, NULL, MKDEV(MAJOR(devno), i),
                                    &ali_m3501_nim_dev[i], "ali_m3501_nim%d", i);
        if(IS_ERR(ali_m3501_nim_dev_node[i]))
        {
            S3501_PRINTF(NIM_LOG_DBG,"device_create() failed!\n");
            ret = PTR_ERR(ali_m3501_nim_dev_node[i]);
            cdev_del(&ali_m3501_nim_dev[i].cdev);
            mutex_destroy(&ali_m3501_nim_priv[i]->i2c_mutex);
            mutex_destroy(&ali_m3501_nim_priv[i]->tuner_open_mutex);
			mutex_destroy(&ali_m3501_nim_priv[i]->multi_process_mutex);
            kfree(ali_m3501_nim_priv[i]);
        }
	ali_m3501_nim_priv[i]->nim_init = FALSE;
	//To solve malloc failure when autoscan
	ali_m3501_nim_priv[i]->ul_status.adcdata_malloc_addr = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE * 2);//256K
	if(NULL == ali_m3501_nim_priv[i]->ul_status.adcdata_malloc_addr)			
	{
	    comm_free(ali_m3501_nim_priv[i]->ul_status.adcdata_malloc_addr);
	    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,ENOMEM!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
    }
	
	//To solve malloc failure when autoscan
	if(NULL == channel_spectrum){
		channel_spectrum = (INT32 *) comm_malloc(FS_MAXNUM * 4);//58K
		if(channel_spectrum == NULL)
		{
			S3501_PRINTF(NIM_LOG_DBG,"\n channel_spectrum--> no enough memory!\n");
			 comm_free(channel_spectrum);
			return ERR_NO_MEM;
		}
	}
	if(NULL == channel_spectrum_tmp)
	{
		channel_spectrum_tmp = (INT32 *) comm_malloc(FS_MAXNUM * 4);//58K
		if(channel_spectrum_tmp == NULL)
		{
		   S3501_PRINTF(NIM_LOG_DBG,"\n channel_spectrum_tmp--> no enough memory!\n");
		   comm_free(channel_spectrum);
		   comm_free(channel_spectrum_tmp);
		   return ERR_NO_MEM;
		}
	}

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
    return ret;
}

static int ali_m3501_nim_remove(struct platform_device *pdev)
{
    UINT8 i = 0;

    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

    if(ali_m3501_nim_class != NULL)
    {
		class_destroy(ali_m3501_nim_class);
    }	

    for (i = 0; i < MAX_TUNER_SUPPORT_NUM; i++)
    {
        if(ali_m3501_nim_dev_node[i] != NULL)
        {
			device_del(ali_m3501_nim_dev_node[i]);
        }	


        cdev_del(&ali_m3501_nim_dev[i].cdev);
        mutex_destroy(&ali_m3501_nim_priv[i]->i2c_mutex);
        mutex_destroy(&ali_m3501_nim_priv[i]->tuner_open_mutex);
		mutex_destroy(&ali_m3501_nim_priv[i]->multi_process_mutex);

        if (ali_m3501_nim_dev[i].autoscan_work_queue)
        {
			destroy_workqueue(ali_m3501_nim_dev[i].autoscan_work_queue);
        }	
        if (ali_m3501_nim_dev[i].workqueue)
        {
			destroy_workqueue(ali_m3501_nim_dev[i].workqueue);
        }	

        kfree(ali_m3501_nim_priv[i]);
    }
    S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,end!\n", __FUNCTION__, __LINE__);

	return 0;
}

static int ali_m3501_nim_suspend(struct platform_device *pdev, pm_message_t state)
{
	return SUCCESS;
}
static int ali_m3501_nim_resume(struct platform_device *pdev)
{
	return SUCCESS;
}

static const struct of_device_id ali_nim_m3501_of_match[] = {
	{.compatible= "alitech, nim_m3501", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_nim_m3501_of_match);

static struct platform_driver nim_platform_driver ={
	.probe                      = ali_m3501_nim_probe,
	.remove                     = ali_m3501_nim_remove,
	.suspend                    = ali_m3501_nim_suspend,
	.resume                     = ali_m3501_nim_resume,
	.driver = {
		.name           = ALI_NIM_DEVICE_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = ali_nim_m3501_of_match,
	},
};

module_platform_driver(nim_platform_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kent Kang");
MODULE_DESCRIPTION("Ali M3501 full NIM driver");
