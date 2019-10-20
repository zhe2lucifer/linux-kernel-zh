/*
 * Copyright 2015 Ali Corporation Inc. All Rights Reserved.
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
 
/*****************************************************************************
*	Copyright (C)2015 Ali Corporation. All Rights Reserved.
*
*	File:	This file contains C3501H DVBS2 system API and hardware 
*           open/closed operate in LLD.
*    
*    Author:robin.gan
*
*    Description:    

*    History:'refer to update_history.txt'
*******************************************************************************/
#include "nim_dvbs_c3501h_linux.h"
#include "nim_dvbs_c3501h.h"
#include "../tun_common.h"
#include <linux/platform_device.h>
#include <linux/ali_kumsgq.h>
#include <ali_soc.h>

#define MAX_DEMOD_SUPPORT_NUM 	2
#define ALI_C3501H_DEVICE_NAME 	"ali_nim_c3501h"

struct class *ali_nim_c3501h_class;

//static UINT8 g_nim_dev_id = 0;
//static UINT8 demod_mutex_flag = 0;

bool g_c3501h_class_create_flag = TRUE;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static UINT32 demod_str_reg_buffer[500] = {0}; //save the reg value when str
static int ali_nim_c3501h_suspend(struct platform_device *pdev,pm_message_t state);
static int ali_nim_c3501h_resume(struct platform_device *pdev);
#endif

void nim_usb_log(char* msg);


static char nim_workq[MAX_DEMOD_SUPPORT_NUM][5] =
{
    "nim0"
};

static char as_workq[MAX_DEMOD_SUPPORT_NUM][12] =
{
    "as_workq0"
};

static char plsn_search_workq[] = "c3501_plsn_search";
static char channel_change_workq[] = "c3501h_channel_change_wq";

__u32 nim_c3501h_dvbs_as_cb2_ui(void *p_priv, __u8 lck, __u8 polar, __u16 freq, __u32 sym, __u8 fec, __u8 as_stat)
{
    struct nim_c3501h_private *priv = (struct nim_c3501h_private *)p_priv;
    priv->as_status = 0;

    nim_send_as_msg(priv->nim_kumsgq, lck, polar, freq, sym, fec, as_stat);
    wait_event_interruptible_timeout(priv->as_sync_wait_queue, priv->as_status & 0x01, 0x7fffffff);
    priv->as_status = 0;

    return SUCCESS;
}

/*****************************************************************************
*  void nim_c3501h_task(UINT32 param1, UINT32 param2)
*  Task of nim driver,  do some monitor or config
*
* Arguments:
*  Parameter1:  device struct point
*  Parameter2:  unused
*
* Return Value: INT32
*****************************************************************************/
void nim_c3501h_task(UINT32 param1, UINT32 param2)
{
#ifdef DEBUG_IN_TASK
    UINT32 v_cnt_val=0x00;
#endif
	struct nim_device *dev = (struct nim_device *) param1 ;
	struct nim_c3501h_private *priv = (struct nim_c3501h_private *) dev->priv ;
	UINT8 work_mode = 0;
	//UINT32 mer;
    
	priv->tsk_status.m_sym_rate = 0x00;
	priv->tsk_status.m_code_rate = 0x00;
	priv->tsk_status.m_map_type = 0x00;
	priv->tsk_status.m_work_mode = 0x00;
	priv->tsk_status.m_info_data = 0x00;
    
 	// Has been locked
	if(priv->tsk_status.m_lock_flag == NIM_LOCK_STUS_CLEAR)
	{
		;
	}
	else
	{   // Waitting lock(lock process han been break by chanscan_stop_flag ) and i2c is normal 
		if ((priv->tsk_status.m_lock_flag == NIM_LOCK_STUS_SETTING) && (priv->t_param.t_i2c_err_flag == 0x00))
		{
			nim_c3501h_get_lock(dev, &(priv->tsk_status.m_info_data));
            // Found locked then refresh the tp information
			if (priv->tsk_status.m_info_data && (priv->t_param.t_i2c_err_flag == 0x00))
			{
				nim_c3501h_get_symbol_rate(dev, &(priv->tsk_status.m_sym_rate));
				nim_c3501h_reg_get_code_rate(dev, &(priv->tsk_status.m_code_rate));
				nim_c3501h_reg_get_work_mode(dev, &(priv->tsk_status.m_work_mode));
				nim_c3501h_reg_get_map_type(dev, &(priv->tsk_status.m_map_type));
				nim_c3501h_clear_interrupt(dev);
				priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_CLEAR;
			}
		}
	}            

	#if (NIM_OPTR_CCM == ACM_CCM_FLAG)	
        nim_c3501h_get_per(dev , &(priv->channel_info->per));
	#else
		nim_c3501h_reg_get_work_mode(dev, &work_mode);
		if (1 == work_mode)
       		nim_c3501h_get_fer(dev , (UINT32 *)(&(priv->channel_info->per)));
		else
			nim_c3501h_get_per(dev , (UINT32 *)(&(priv->channel_info->per)));
	#endif
    nim_c3501h_get_snr_db(dev, (INT16 *)(&(priv->channel_info->snr)));
    //nim_c3501h_get_phase_error(dev , &(priv->channel_info->phase_err));    
	//nim_c3501h_get_mer(dev, &mer);
    /*when channel change or autoscan finished,the following function can be executed*/
    if (0 == priv->chanscan_autoscan_flag)
	{
		 mutex_lock(&priv->multi_process_mutex);
		 #ifdef DEBUG_IN_TASK 
	     nim_c3501h_debug_intask(dev);
		 #endif  

		 #ifdef IMPLUSE_NOISE_IMPROVED
	     nim_c3501h_auto_adaptive(dev);    
		 #endif 
	     mutex_unlock(&priv->multi_process_mutex);
	 } 
	if(priv->work_alive)
		queue_delayed_work(dev->workqueue, &dev->delay_work, TIMER_DELAY * HZ / 1000);
}

void nim_c3501h_plsn_task(struct delayed_work *work)
{    
	struct nim_device *dev = NULL;
	struct nim_c3501h_private *priv = NULL;
	UINT32 start = 0;
	
	pr_info("[%s %d]Enter\n", __FUNCTION__, __LINE__);
	dev = container_of((void *)work, struct nim_device, delay_plsn_search_work);

	priv = (struct nim_c3501h_private *) dev->priv;
	
	pr_info("[%s %d]start to search plsn!\n", __FUNCTION__, __LINE__);
	start = jiffies;

	nim_c3501h_set_set_cr_dis_pilot(dev);
			
	priv->plsn.search_plsn_stop = 0;
	priv->plsn.plsn_find = 0;
	priv->plsn.plsn_hw_find = 0;
	priv->plsn.plsn_hw_end = 0;
	priv->plsn.search_lock_state = 0;
	priv->plsn.plsn_num = 0;
	priv->plsn.plsn_try = 0;
	priv->plsn.plsn_try_val = 0;
	memset(priv->plsn.plsn_val, 0, sizeof(priv->plsn.plsn_val));
	priv->plsn.auto_scan_start = 0;

	nim_c3501h_search_plsn_top(dev);

	nim_c3501h_set_set_cr_en_pilot(dev);

	priv->plsn.start_search = 0;
	
	pr_info("[%s %d]end to search plsn, cost %d ms\n", __FUNCTION__, __LINE__, (int)(jiffies - start));
}

static void nim_c3501h_task_open(struct delayed_work *work)
{
    struct nim_device 				*dev;
	
    dev = container_of((void *)work, struct nim_device, delay_work);
    nim_c3501h_task((UINT32)dev, 0);
}

static void nim_c3501h_channel_change_wq(struct delayed_work *work)
{
	struct nim_device 				*dev;
	struct nim_c3501h_private 		*priv;
	
    dev = container_of((void *)work, struct nim_device, delay_channel_change_work);
	priv = dev->priv;
	
	nim_flag_clear(&priv->flag_lock, NIM_FLAG_CHN_CHG_START);
    nim_flag_set(&priv->flag_lock, NIM_FLAG_CHN_CHANGING);
	nim_c3501h_waiting_channel_lock(dev, priv->cur_freq, priv->cur_sym, priv->change_type, priv->isid);
#ifdef FORCE_WORK_MODE_IN_CHANGE_TP
	nim_c3501h_set_work_mode(dev, 0x03);
#endif
	priv->chanscan_autoscan_flag = 0;//channel changel finish
	//demod_mutex_flag = 0;
	nim_flag_clear(&priv->flag_lock, NIM_FLAG_CHN_CHANGING);
	pr_info("\t\t Here is the task for C3505 wait channel lock \n");

	if(priv->search_type == NIM_OPTR_CHL_CHANGE)
    {                   
	#ifdef ANTI_WIMAX_INTF
   	    if(force_adpt_disable == 1)
           {
               //nim_c3501h_cr_new_adaptive_unlock_monitor(dev);
               //nim_c3501h_nframe_step_tso_setting(dev,100,0x00);
		   }
		   else
		   {
		       nim_c3501h_task_tso_setting(dev,100,0x00);
		   }
	#else		 
            //nim_c3501h_cr_new_adaptive_unlock_monitor(dev);
            //nim_c3501h_nframe_step_tso_setting(dev,100,0x00);
	#endif	
     }

}

static void nim_c3501h_autoscan_open(struct delayed_work *work)
{
    NIM_AUTO_SCAN_T					*pstauto_scan;
    struct nim_device 				*dev;
    struct nim_c3501h_private 		*priv;

    dev = container_of((void *)work, struct nim_device, delay_as_work);
    priv = dev->priv;
    pstauto_scan = &priv->as_info;

    nim_c3501h_autoscan(dev, pstauto_scan);
}

static INT32 nim_c3501h_task_init(struct nim_device *dev)
{
    UINT8 dev_idx = 0;
    struct nim_c3501h_private 	*priv = dev->priv;

    pr_info("[%s %d]enter!\n", __FUNCTION__, __LINE__);

	dev_idx = priv->dev_idx;
    dev->workqueue = create_workqueue(nim_workq[dev_idx]);

    if (!(dev->workqueue))
    {
        pr_err("[%s %d]Failed to allocate work queue\n", __FUNCTION__, __LINE__);
        return -1;
    }

    dev->autoscan_work_queue = create_workqueue(as_workq[dev_idx]);
    if (!(dev->autoscan_work_queue))
    {
        pr_err("[%s %d]Failed to create nim autoscan workequeue!\n", __FUNCTION__, __LINE__);
        destroy_workqueue(dev->workqueue);
        return -1;
    }

	dev->plsn_search_workqueue = create_workqueue(plsn_search_workq);
    if (!(dev->plsn_search_workqueue))
    {
        pr_err("[%s %d]Failed to create nim autoscan workequeue!\n", __FUNCTION__, __LINE__);
        destroy_workqueue(dev->workqueue);
		destroy_workqueue(dev->autoscan_work_queue);
        return -1;
    }

	dev->channel_change_workqueue= create_workqueue(channel_change_workq);
    if (!(dev->channel_change_workqueue))
    {
        pr_err("[%s %d]Failed to create nim autoscan workequeue!\n", __FUNCTION__, __LINE__);
        destroy_workqueue(dev->workqueue);
		destroy_workqueue(dev->autoscan_work_queue);
		destroy_workqueue(dev->plsn_search_workqueue);
        return -1;
    }
	
    init_waitqueue_head(&priv->as_sync_wait_queue);
    priv->work_alive = 1;
	priv->ul_status.c3501h_chanscan_stop_flag = 0;

	INIT_DELAYED_WORK(&dev->delay_work, (void *)nim_c3501h_task_open);
	INIT_DELAYED_WORK(&dev->delay_plsn_search_work, (void *)nim_c3501h_plsn_task);
    INIT_DELAYED_WORK(&dev->delay_as_work, (void *)nim_c3501h_autoscan_open);
	INIT_DELAYED_WORK(&dev->delay_channel_change_work, (void *)nim_c3501h_channel_change_wq);
	queue_delayed_work(dev->workqueue, &dev->delay_work, TIMER_DELAY * HZ / 1000);
	
    return SUCCESS;
}

static INT32 ali_nim_c3501h_hw_initialize(struct nim_device *dev, struct ali_nim_m3501_cfg *nim_cfg)
{
    INT32 	ret = 0;
	INT32	channel_freq_err = 0;
	TUNER_IO_FUNC               *p_io_func = NULL;
    struct nim_c3501h_private 	*priv = dev->priv;

    pr_info("[%s %d]enter!\n", __FUNCTION__, __LINE__);

	if(priv->nim_init)
	{	
		pr_err("[%s %d]C3501h have inited! priv->nim_init=%d!\n", __FUNCTION__, __LINE__, priv->nim_init);
    	return RET_SUCCESS;
	}

	//diseqc state inits
	priv->diseqc_info.diseqc_type = 0;
    priv->diseqc_info.diseqc_port = 0;
    priv->diseqc_info.diseqc_k22 = 0;
	if ((priv->tuner_config_data.qpsk_config & C3501H_POLAR_REVERT) == C3501H_POLAR_REVERT) //bit4: polarity revert.
		priv->diseqc_info.diseqc_polar = 2;//LNB_POL_V;
	else //default usage, not revert.
		priv->diseqc_info.diseqc_polar = 1;//LNB_POL_H;

    priv->tuner_config_data.qpsk_config = nim_cfg->qpsk_config;
    priv->tuner_config_data.recv_freq_high = nim_cfg->recv_freq_high;
    priv->tuner_config_data.recv_freq_low = nim_cfg->recv_freq_low;
	priv->tuner_config_data.disqec_polar_position = nim_cfg->disqec_polar_position;
	pr_info("[%s %d]priv->tuner_config_data.disqec_polar_position=%d\n", __FUNCTION__, __LINE__, priv->tuner_config_data.disqec_polar_position);
	
    priv->ext_dm_config.i2c_base_addr = nim_cfg->demod_i2c_addr;
    priv->ext_dm_config.i2c_type_id = nim_cfg->demod_i2c_id;
    priv->tuner_config.c_tuner_base_addr = nim_cfg->tuner_i2c_addr;
    priv->tuner_config.i2c_type_id = nim_cfg->tuner_i2c_id;
    priv->i2c_type_id = nim_cfg->tuner_i2c_id;
	priv->ext_lnb_config.i2c_type_id = nim_cfg->lnb_i2c_id;
	priv->ext_lnb_config.i2c_base_addr = nim_cfg->lnb_i2c_addr;
	priv->ext_lnb_type = nim_cfg->lnb_name;
	
	priv->search_type = NIM_OPTR_CHL_CHANGE;

    priv->ul_status.m_enable_dvbs2_hbcd_mode = 0;
    priv->ul_status.m_dvbs2_hbcd_enable_value = 0x7f;
    priv->ul_status.nim_c3501h_sema = OSAL_INVALID_ID;
    priv->ul_status.c3501h_autoscan_stop_flag = 0;
    priv->ul_status.c3501h_chanscan_stop_flag = 0;
    priv->ul_status.old_ber = 0;
    priv->ul_status.old_per = 0;
    priv->ul_status.m_hw_timeout_thr = 0;
    priv->ul_status.old_ldpc_ite_num = 0;
    priv->ul_status.c_rs = 0;
    priv->ul_status.phase_err_check_status = 0;
    priv->ul_status.lock_status = NIM_LOCK_STUS_NORMAL;
    priv->ul_status.m_c3501h_type = 0x00;
    priv->ul_status.m_setting_freq = 123;
    priv->ul_status.m_err_cnts = 0x00;
    priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_NORMAL;
    priv->tsk_status.m_task_id = 0x00;
	
    priv->t_param.t_reg_setting_switch = 0x0f;
    priv->t_param.t_i2c_err_flag = 0x00;
    priv->flag_lock.flag_id = OSAL_INVALID_ID;

    priv->blscan_mode = NIM_SCAN_SLOW;

	p_io_func = tuner_setup(NIM_DVBS, nim_cfg->tuner_id);
	if(NULL != p_io_func)
	{
		priv->nim_tuner_init = (dvbs_tuner_init_callback)(p_io_func->pf_init);
		priv->nim_tuner_control = p_io_func->pf_control;
		priv->nim_tuner_status = p_io_func->pf_status;
		priv->nim_tuner_command =  p_io_func->pf_command;
		priv->nim_tuner_gain =  p_io_func->pf_gain;
		priv->nim_tuner_close = p_io_func->pf_close;
	}
	else
	{
		pr_err("[%s %d]set tuner error! p_io_func==NULL!\n", __FUNCTION__, __LINE__);
		return ERR_FAILURE;
	}
    
	if(priv->ext_lnb_type != 0)
	{
		pr_info("[%s %d] Start set ext_lnb_type %d\n",__FUNCTION__,__LINE__,(int)priv->ext_lnb_type);
		nim_c3501h_set_ext_lnb(dev);
	}

	nim_c3501h_hw_open(dev);
	
    ret = nim_c3501h_hw_check(dev);
    if (ret != SUCCESS)
    {
        pr_err("[%s %d]hw check error!\n", __FUNCTION__, __LINE__);
        return ret;
    }
	nim_c3501h_reg_get_strap_bond_info(dev);
    nim_c3501h_hw_init(dev);
	nim_c3501h_after_reset_set_param(dev);

    nim_c3501h_task_init(dev);

	
	// Initial the QPSK Tuner
	if (priv->nim_tuner_init != NULL)
	{
		pr_info("[%s %d]Initial the Tuner \n", __FUNCTION__, __LINE__);

		nim_c3501h_i2c_through_open(dev);
		if (priv->nim_tuner_init(&priv->tuner_index, &(priv->tuner_config)) != SUCCESS)
		{
			pr_err("[%s %d]Error: Init Tuner Failure!\n", __FUNCTION__, __LINE__);
			nim_c3501h_i2c_through_close(dev);
			return ERR_NO_DEV;
		}
		pr_info("[%s %d]priv->tuner_index=%d\n", __FUNCTION__, __LINE__, (int)priv->tuner_index);
		priv->tuner_opened = 1;

		if (priv->nim_tuner_command != NULL)
	    {
	    	ret = priv->nim_tuner_command(priv->tuner_index, NIM_TUNER_M3031_ID, &channel_freq_err);
			if (SUCCESS == ret)
			{
				priv->tuner_type = IS_M3031;
				pr_info("[%s %d]M3031 Tuner!\n", __FUNCTION__, __LINE__);
			}
			else
			{
				priv->tuner_type = NOT_M3031;
				pr_info("[%s %d]NOT M3031 Tuner!\n", __FUNCTION__, __LINE__);
			}
		}	
		else
		{
			priv->tuner_type = NOT_M3031;
			pr_info("[%s %d]NOT M3031 Tuner!\n", __FUNCTION__, __LINE__);
		}

		nim_c3501h_i2c_through_close(dev);
	}
	else
	{
		pr_err("[%s %d]priv->nim_tuner_init is NULL!\n", __FUNCTION__, __LINE__);
		return ERR_FAILURE;
	}
	
#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_create(&priv->flag_lock);
#endif
    pr_info("[%s %d]end!\n", __FUNCTION__, __LINE__);
	priv->nim_init = TRUE;

	return RET_SUCCESS;
}

static long ali_nim_c3501h_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    INT32 ret = 0;

    struct nim_device 			*dev = file->private_data;
    struct nim_c3501h_private 	*priv = dev->priv;
    unsigned long 				arg = (unsigned long) parg;
	UINT32 plsn_value;
	UINT32 temp_value;
	UINT8 work_mode = 0;
	UINT8 pin_num;
	UINT8 value;

	mutex_lock(&priv->multi_process_mutex);
    switch (cmd)
    {
	    case ALI_NIM_HARDWARE_INIT_S:
	    {
	        struct ali_nim_m3501_cfg nim_param;
			
	        if(copy_from_user(&nim_param, (struct ali_nim_m3501_cfg*)parg, sizeof(struct ali_nim_m3501_cfg))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}			
	        ret = ali_nim_c3501h_hw_initialize(dev, &nim_param);
	        break;
	    }
		
	    case ALI_NIM_SET_POLAR:
	    {
	        UINT8 polar_param = 0;
			
	        get_user(polar_param, (unsigned char *)parg);
	        ret = nim_c3501h_set_polar(dev, polar_param);
	        break;
	    }
		
	    case ALI_NIM_CHANNEL_CHANGE:
	    {
	        NIM_CHANNEL_CHANGE_T nim_param;		
			struct nim_dvbs_tsn * temp_user_addr = NULL;
			struct nim_dvbs_isid * temp_user_isid_addr = NULL;
			memset(&nim_param, 0, sizeof(NIM_CHANNEL_CHANGE_T));
			
			//copy data from user space to kernel space, the pointer in struct is still user space address
	        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}			

			temp_user_addr = nim_param.tsn;//nim_param.tsn is still user space address
			if (temp_user_addr != NULL)
			{				
				nim_param.tsn = kmalloc(sizeof(struct nim_dvbs_tsn), GFP_KERNEL);
				if (NULL == nim_param.tsn)
				{				
					pr_err("[ %s %d ]malloc nim_param.tsn error\n", __FUNCTION__, __LINE__);
					mutex_unlock(&priv->multi_process_mutex);
					return (-ENOMEM);
				}			
				
				//temp_nim_param.tsn is still user space address, so use copy_from_user
				if(copy_from_user(nim_param.tsn, temp_user_addr, sizeof(struct nim_dvbs_tsn))>0)
				{
					if (nim_param.tsn != NULL)
					{
						kfree(nim_param.tsn);
					}
					pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
			}
			temp_user_isid_addr = nim_param.isid;//nim_param.tsn is still user space address
			if (temp_user_isid_addr != NULL)
			{				
				nim_param.isid = kmalloc(sizeof(struct nim_dvbs_isid), GFP_KERNEL);
				if (NULL == nim_param.isid)
				{				
					pr_err("[ %s %d ]malloc nim_param.isid error\n", __FUNCTION__, __LINE__);
					mutex_unlock(&priv->multi_process_mutex);
					return (-ENOMEM);
				}			
				
				//temp_nim_param.isid is still user space address, so use copy_from_user
				if(copy_from_user(nim_param.isid, temp_user_isid_addr, sizeof(struct nim_dvbs_isid))>0)
				{
					if (nim_param.isid != NULL)
					{
						kfree(nim_param.isid);
					}
					pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
			}
			//demod_mutex_flag = 1;
			priv->chanscan_autoscan_flag = 1;//Enter channelchange function
	        ret = nim_c3501h_channel_change(dev, &nim_param);
			if (nim_param.tsn != NULL)
				kfree(nim_param.tsn);
			if(nim_param.isid != NULL)
				kfree(nim_param.isid);
		#ifndef CHANNEL_CHANGE_ASYNC
             //demod_mutex_flag = 0;
             priv->chanscan_autoscan_flag = 0;//Leave channelchange function
        #endif
			if (SUCCESS != ret)
			 	priv->chanscan_autoscan_flag = 0;//Leave channelchange function
			break;
	    }
		
		case ALI_NIM_LOG_LEVEL:
		{
	        UINT32 log_level= 0;

			if(copy_from_user(&log_level, (UINT32 *)parg, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
	        set_log_level(log_level);
			ret=0;
			
			break;
		}		
		
	    case ALI_NIM_GET_LOCK_STATUS:
	    {
	        UINT8 lock = 0;

		#ifndef FC_SEARCH_RANGE_LIMITED
			nim_c3501h_get_lock(dev, &lock); 
        #else
        	nim_c3501h_get_lock_check_limited(dev, &lock); 
        #endif
	        ret = lock;

	        break;
	    }
		
	    case ALI_NIM_READ_QPSK_BER:
	    {
	        UINT32 ber = 0;
			
	        nim_c3501h_get_ber(dev, &ber);
	        ret = ber;
	        break;
	    }
		
	    case ALI_NIM_READ_RSUB:
	    {
	        UINT32 per = 0;

		#ifdef DEBUG_IN_TASK
            per = &(priv->channel_info->per);
        #else
            nim_c3501h_reg_get_work_mode(dev, &(priv->channel_info->work_mode));
            if (1 == priv->channel_info->work_mode)
            {
                nim_c3501h_get_fer(dev, &per); 
            }
            else
            {
            	nim_c3501h_get_per(dev, &per); 
            }
        #endif
			
			ret = per;
	        break;
	    }
		
	    case ALI_NIM_READ_AGC:
	    {
	        UINT8 agc = 0;

	        nim_c3501h_get_agc(dev, &agc);
	        ret = agc;
	        break;
	    }
		
		case ALI_NIM_GET_RF_LEVEL : //uint 0.1dbm range:s(0--14) s2(0-25);precison:(-+1dbm)
		{
			INT8 rf_value = 0;
			INT32 tmp = 0;
			
			nim_c3501h_get_agc_dbm(dev,&rf_value);
			if(rf_value < 0)
			{
				rf_value = 0 - rf_value;
			}
			tmp = rf_value * 10;//dbm transfer 0.1dbm
			ret = tmp;
			break;
		}
		
	    case ALI_NIM_READ_SNR:
	    {
	        UINT8 snr = 0;
			
	        nim_c3501h_get_snr(dev, &snr);
	        ret = snr;
	        break;
	    }
		
		case ALI_NIM_GET_CN_VALUE: //uint 0.01db,range :0-25;precision: 0-20:(+-1db),>25:(+-2db)
		{	
			INT16 cn_value = 0;
			
			nim_c3501h_get_snr_db(dev,&cn_value);//snr uint is 0.01 db
			ret = cn_value;
			break;
		}
		
		case ALI_NIM_GET_MER: //uint 0.1db
		{
			UINT16 mer = 0;
			
			//nim_c3501h_get_mer(dev, &mer);
			nim_c3501h_get_snr_db(dev, &mer);
			ret = mer;
			break;
		}
		
	    case ALI_NIM_READ_SYMBOL_RATE:
	    {
	        UINT32 sym = 0;
			
	        nim_c3501h_get_symbol_rate(dev, &sym);
	        ret = sym;
	        break;
	    }
		
	    case ALI_NIM_READ_FREQ:
	    {
	        UINT32 freq = 0;
			
			ret = nim_c3501h_get_cur_freq(dev ,&freq);
	        break;
	    }
		
	    case ALI_NIM_READ_CODE_RATE:
	    {
	        UINT8 fec = 0;

	        nim_c3501h_reg_get_code_rate(dev, &fec);
	        ret = fec;

	        break;
	    }
		
	    case ALI_NIM_AUTO_SCAN:          /* Do AutoScan Procedure */
	    {
			NIM_AUTO_SCAN_T as_load;
	        flush_workqueue(dev->autoscan_work_queue);
	        
	        if(copy_from_user(&as_load, (NIM_AUTO_SCAN_T *)parg, sizeof(NIM_AUTO_SCAN_T))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}			
	        priv->yet_return = FALSE;
	        priv->as_info.sfreq = as_load.sfreq;
	        priv->as_info.efreq = as_load.efreq;
	        priv->as_info.unicable = as_load.unicable;
	        priv->as_info.fub = as_load.fub;
	        priv->as_info.callback = nim_c3501h_dvbs_as_cb2_ui;
			priv->autoscan_control = NORMAL_MODE;

	        //demod_mutex_flag = 1;
			priv->chanscan_autoscan_flag = 1;//Enter autoscan function
	        ret = queue_delayed_work(dev->autoscan_work_queue, &dev->delay_as_work, 0);
			//demod_mutex_flag = 0;
			priv->chanscan_autoscan_flag = 0;//leave autoscan funtion
	        break;
	    }
		
	    case ALI_NIM_STOP_AUTOSCAN:
		{
	        priv->ul_status.c3501h_autoscan_stop_flag = arg;
			pr_info("[%s %d]priv->ul_status.c3501h_autoscan_stop_flag=%d\n",
						__FUNCTION__, __LINE__, (int)(priv->ul_status.c3501h_autoscan_stop_flag));
	        break;
	    }
		
	    case ALI_NIM_DISEQC_OPERATE:
	    {
	        struct ali_nim_diseqc_cmd dis_cmd;

	        if(copy_from_user(&dis_cmd, (struct ali_nim_diseqc_cmd *)parg, sizeof(struct ali_nim_diseqc_cmd))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}				
				
	        switch(dis_cmd.diseqc_type)
	        {
		        case 1:
				{
		            ret = nim_c3501h_diseqc_operate(dev, dis_cmd.mode, dis_cmd.cmd, dis_cmd.cmd_size);
		            break;
		        }
				
		        case 2:
		        {
					ret = nim_c3501h_diseqc2x_operate(dev, dis_cmd.mode, \
	                                             dis_cmd.cmd, dis_cmd.cmd_size, dis_cmd.ret_bytes, &dis_cmd.ret_len);
		            if(copy_to_user((struct ali_nim_diseqc_cmd *)parg, &dis_cmd, sizeof(struct ali_nim_diseqc_cmd))>0)
					{
						pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
						mutex_unlock(&priv->multi_process_mutex);
						return -EFAULT;
					}
		            break;
		        }
				
		        default:
				{
		            ret = -ENOIOCTLCMD;
		            break;
		        }
	        }
	        break;
	    }
	 
		case ALI_NIM_GET_KUMSGQ:
		{
			int flags = -1;	
			if(copy_from_user(&flags, (int *)parg, sizeof(int))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			ret  = ali_kumsgq_newfd(priv->nim_kumsgq, flags);
			if(ret > 0)
			{
				pr_info("[%s %d]create kumsgq fd success!\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return ret;	
			}
			break;
		}	

	    case ALI_NIM_AS_SYNC_WITH_LIB:
	    {
	        priv->as_status |= 0x01;
	        wake_up_interruptible(&priv->as_sync_wait_queue);
	        break;
	    }
		
	    case ALI_NIM_DRIVER_GET_CUR_FREQ:
		{
			break;
	    }
		
	    case ALI_NIM_TURNER_SET_STANDBY:
		{
			nim_c3501h_i2c_through_open(dev);
	        if (priv->nim_tuner_command != NULL)
	        {
	            priv->nim_tuner_command(priv->tuner_index, NIM_TURNER_SET_STANDBY, 0);
	        }
			nim_c3501h_i2c_through_close(dev);
	        break;
	    }
		
	    case ALI_NIM_DRIVER_GET_ID:
		{
	        ret = priv->ul_status.m_c3501h_type;

	        break;
	    }
		
	    case ALI_NIM_REG_RW:
	    {
		        UINT8 reg_rw_cmd[16] ={0};	            
		        if(copy_from_user(reg_rw_cmd, (UINT8 *)parg, 16)>0)
				{
					pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
				
		        if (1 == reg_rw_cmd[0]) // Register Read
		        {
		            ret = nim_reg_read(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
		            if(copy_to_user((UINT8 *)parg, reg_rw_cmd, 16)>0)
					{
						pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
						mutex_unlock(&priv->multi_process_mutex);
						return -EFAULT;
					}
									
		        }
		        else if (2 == reg_rw_cmd[0]) // Register Write
		        {					
		            ret = nim_reg_write(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
		        }
			break;	
	    }
		
	    case ALI_NIM_SET_LNB_POWER:
			break;
			
	    case ALI_NIM_GET_LNB_POWER:
			break;
			
	    case ALI_NIM_SET_LNB_FREQS:
			break;
			
		case ALI_NIM_GET_LNB_FREQS:
			break;
			
		case ALI_NIM_GET_LNB_OCP:
		{
			if (priv->ext_lnb_command)
			{
				ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_GET_OCP, 0);	
			}
			break;
		}
		
		case ALI_NIM_SET_LNB_POWER_ONOFF:
		{
			UINT8 onoff = 0;
			get_user(onoff, (unsigned char *)parg);
			if (priv->ext_lnb_command)
			{
				ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_POWER_ONOFF, onoff);
			}
			break;
		}
					
		case ALI_NIM_LNB_CURRENT_LIMIT_CONTROL:
		{
			UINT8 onoff = 0;
			get_user(onoff, (unsigned char *)parg);
			if (priv->ext_lnb_command)
			{
				ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_CURRENT_LIMIT_CONTROL, onoff);
			}
			break;
		}
					
		case ALI_NIM_SET_LOOPTHRU:
			break;
			
		case ALI_NIM_GET_LOOPTHRU:
	        break;
			
		case ALI_NIM_ACM_MODE_GET_TP_INFO:
		{
			nim_c3501h_get_tp_info(dev);
			break;
		}
		
		case ALI_NIM_ACM_MODE_SET_ISID:
		{
			struct nim_dvbs_isid isid;
	    	if(copy_from_user(&isid, (struct nim_dvbs_isid *)parg, sizeof(struct nim_dvbs_isid))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			nim_c3501h_set_isid(dev,&isid);
			break;
		}
				
		case ALI_NIM_ACM_MODE_GET_ISID:
		{
			if (priv->ul_status.c3501h_chanscan_stop_flag || priv->ul_status.c3501h_autoscan_stop_flag)
			{
				pr_info("[%s %d]don't allow get isid!\n",__FUNCTION__, __LINE__);
				priv->isid->get_finish_flag = 0;			
			}
			
			if(copy_to_user((struct nim_dvbs_isid *)parg, priv->isid, sizeof(struct nim_dvbs_isid))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			break;
		}		
			
		case ALI_NIM_DRIVER_CAP_DATA:
		{
			struct nim_dvbs_data_cap data_cap;
			if(copy_from_user(&data_cap, (struct nim_dvbs_data_cap *)parg, sizeof(struct nim_dvbs_data_cap))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			nim_c3501h_ts_cap_data_entity(dev, (UINT32 *)data_cap.dram_base_addr, data_cap.cap_len, data_cap.cap_src);
			break;
		}

		case ALI_NIM_DRIVER_GET_SYMBOL:
		{
			struct nim_get_symbol symbol;
			if(copy_from_user(&symbol, (struct nim_get_symbol *)parg, sizeof(struct nim_get_symbol))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			
			//if ((symbol.p_cr_out_i != NULL) && (symbol.p_cr_out_q != NULL))
			//{
				//nim_c3501h_get_symbol(dev, &symbol);				
			//}
			break;
		}

		case ALI_NIM_DRIVER_SET_PLSN:
		{
			if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			priv->plsn.plsn_try = 0;
			priv->plsn.plsn_now = plsn_value;
	        nim_c3501h_set_plsn(dev);
	        break;
		}
		
		case ALI_NIM_DRIVER_GET_PLSN:
		{
			if(copy_to_user((UINT32 *)parg, &priv->plsn.plsn_report, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			pr_info("[%s %d]*(UINT32 *)parg=%d, priv->plsn.plsn_report=%d\n", __FUNCTION__, __LINE__, *(int *)parg, (int)priv->plsn.plsn_report);
			break;
		}
		
		case ALI_NIM_DRIVER_PLSN_GOLD_TO_ROOT:
		{
			if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			temp_value = nim_c3501h_plsn_gold_to_root(plsn_value);
			if(copy_to_user((UINT32 *)parg, &temp_value, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			
			break;
		}
		
		case ALI_NIM_DRIVER_PLSN_ROOT_TO_GOLD:
		{
			if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			temp_value = nim_c3501h_plsn_root_to_gold(plsn_value);

			if(copy_to_user((UINT32 *)parg, &temp_value, sizeof(UINT32))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			
			break;
		}
		
		case ALI_NIM_DRIVER_GENERATE_TABLE:
			break;
		
		case ALI_NIM_DRIVER_RELEASE_TABLE:
			break;
		
		case ALI_NIM_DRIVER_GET_PLSN_FINISH_FLAG:
		{
			if(copy_to_user((UINT8 *)parg, &priv->plsn.plsn_finish_flag, sizeof(UINT8))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			break;
		}		
		
		case ALI_NIM_GET_WORK_MODE:
	    {
	        nim_c3501h_reg_get_work_mode(dev,&work_mode);
			if(copy_to_user((UINT8 *)parg, &work_mode, sizeof(work_mode))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}

	        break;
	    }

		case ALI_NIM_DRIVER_SET_TSN:
		{
			if(copy_from_user(priv->tsn, (struct nim_dvbs_tsn *)parg, sizeof(struct nim_dvbs_tsn))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			nim_c3501h_set_tsn(dev, priv->tsn);
			break;
		}
		
		case ALI_NIM_DRIVER_GET_TSN:
		{
			nim_c3501h_get_tsn(dev, priv->tsn);			
			if(copy_to_user((struct nim_dvbs_tsn *)parg, &priv->tsn, sizeof(struct nim_dvbs_tsn))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			break;
		}
		
		case ALI_NIM_DRIVER_GET_WIDEBAND_MODE:
		{
			mutex_lock(&priv->tsn_mutex);
			if(copy_to_user((UINT8 *)parg, &priv->tsn->is_wideband, sizeof(UINT8))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->tsn_mutex);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			TSN_PRINTF("[%s %d]is_wideband=%d\n", __FUNCTION__, __LINE__, *(UINT8 *)parg);
			mutex_unlock(&priv->tsn_mutex);
			break;
		}	
		
	    case ALI_NIM_DRIVER_GET_TSN_FINISH_FLAG:
		{
			mutex_lock(&priv->tsn_mutex);
			if(copy_to_user((UINT8 *)parg, &priv->tsn->get_finish_flag, sizeof(UINT8))>0)
			{
				pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->tsn_mutex);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			TSN_PRINTF("[%s %d]tsn get_finish_flag=%d\n", __FUNCTION__, __LINE__, *(UINT8 *)parg);
			mutex_unlock(&priv->tsn_mutex);
			break;
	    }

		case ALI_NIM_DRIVER_GPIO_OUTPUT:
		{
			pin_num = ((struct DEMOD_GPIO_STRUCT *)(parg))->position;
			value = ((struct DEMOD_GPIO_STRUCT *)(parg))->value;
			NIM_PRINTF("[%s %d]pin_num=%d, value=%d\n", __FUNCTION__, __LINE__, pin_num, value);
			nim_c3501h_gpio_output(dev, pin_num, value);
			break;
		}
		
		case ALI_NIM_DRIVER_ENABLE_GPIO_INPUT:
		{
			pin_num = ((struct DEMOD_GPIO_STRUCT *)(parg))->position;
			nim_c3501h_enable_gpio_input(dev, pin_num);
			break;
		}
		
		case ALI_NIM_DRIVER_GET_GPIO_INPUT:
		{
			pin_num = ((struct DEMOD_GPIO_STRUCT *)(parg))->position;
			nim_c3501h_get_gpio_input(dev, pin_num, &value);
			((struct DEMOD_GPIO_STRUCT *)(parg))->value = value;
			break;
		}
		
	    default:
		{
	        ret = -ENOIOCTLCMD;
			pr_err("[%s %d]error, cmd=0x%x\n", __FUNCTION__, __LINE__, cmd);
	        break;
	    }
    }
	mutex_unlock(&priv->multi_process_mutex);
    return ret;
}

/*****************************************************************************
* INT32 ali_m3503_nim_open(struct nim_s3501_private *priv)
* Description: S3501 open
*
* Arguments:
*  Parameter1: struct nim_s3501_private *priv
*
* Return Value: INT32
*****************************************************************************/
static int ali_nim_c3501h_open(struct inode *inode, struct file *file)
{
    UINT8 dev_idx = 0;
	struct nim_device    * dev = NULL;
	struct nim_c3501h_private * priv = NULL;

	pr_info("[%s %d]Enter!\n", __FUNCTION__, __LINE__);
	dev = container_of(inode->i_cdev, struct nim_device, cdev);
	if (NULL == dev)
	{
		pr_err("[%s %d]get dev error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	
	priv = dev->priv;
	if (NULL == priv)
	{
		pr_err("[%s %d]get priv error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

    pr_info("[%s]line=%d,enter,dev_idx=%d!\n", __FUNCTION__, __LINE__, dev_idx);

	priv->nim_kumsgq = ali_new_kumsgq();
	pr_info("[%s %d]dev_idx=%d nim_kumsgq=0x%x \r\n", __FUNCTION__, __LINE__, dev_idx, (unsigned int)priv->nim_kumsgq);
	if (!priv->nim_kumsgq)
	{
		pr_err("[%s %d]create kumsgq fail!\n", __FUNCTION__, __LINE__);
		goto out0;
    }

	priv->nim_used++;
    file->private_data = (void *)dev;
    pr_info("[%s %d]end!\n", __FUNCTION__, __LINE__);
    return RET_SUCCESS;
	
out0:
	dev_idx = MINOR(inode->i_rdev);
	WARN(1,"False to new 3501h nim[%d] kumsgq!!!!!!", dev_idx);
	return -EFAULT;
}


static int ali_nim_c3501h_release(struct inode *inode, struct file *file)
{
    struct nim_device 		  *dev = file->private_data;
    struct nim_c3501h_private  *priv = dev->priv;

    pr_info("[%s %d],enter!\n", __FUNCTION__, __LINE__);
	
	if (priv->nim_used != 0)
	{
		priv->nim_used--;
		if(priv->nim_used)
		{
			pr_err("[%s %d]enter,priv->nim_used=%d!\n", __FUNCTION__, __LINE__, (int)priv->nim_used);
			return RET_SUCCESS;
		}
	}

	nim_c3501h_i2c_through_open(dev);
    if (priv->tuner_opened && priv->nim_tuner_close)
    {
		priv->nim_tuner_close(priv->tuner_index);	
    }
	nim_c3501h_i2c_through_close(dev);
	
	if (priv->ext_lnb_command)
		priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_RELEASE_CHIP, 0);

	nim_c3501h_reset_fsm(dev);                                                                                  

    nim_c3501h_hw_close(dev);
	
    priv->work_alive = 0;
    priv->ul_status.c3501h_chanscan_stop_flag = 1;
	priv->ul_status.c3501h_autoscan_stop_flag = 1;	

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
		cancel_delayed_work(&dev->delay_as_work);
        flush_workqueue(dev->autoscan_work_queue);
        destroy_workqueue(dev->autoscan_work_queue);
    }

    if (dev->workqueue)
    {
		cancel_delayed_work(&dev->delay_work);
        flush_workqueue(dev->workqueue);
        destroy_workqueue(dev->workqueue);
    }

	if (dev->plsn_search_workqueue)
    {
		cancel_delayed_work(&dev->delay_plsn_search_work);
        flush_workqueue(dev->plsn_search_workqueue);
        destroy_workqueue(dev->plsn_search_workqueue);
    }
	
	if(dev->channel_change_workqueue)
	{
		cancel_delayed_work(&dev->delay_channel_change_work);
		flush_workqueue(dev->channel_change_workqueue);
		destroy_workqueue(dev->channel_change_workqueue);
	}
#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_del(&priv->flag_lock);
#endif
    pr_info("[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
	priv->nim_init = FALSE;

	ali_destroy_kumsgq(priv->nim_kumsgq);
	priv->nim_kumsgq = NULL;

    return RET_SUCCESS;
}


static struct file_operations ali_nim_c3501h_fops =
{
    .owner						= THIS_MODULE,
    .write                      = NULL,
    .unlocked_ioctl             = ali_nim_c3501h_ioctl,
    .open                       = ali_nim_c3501h_open,
    .release                    = ali_nim_c3501h_release,
};

static int ali_nim_c3501h_malloc_memory(struct nim_device *dev)
{
	struct nim_dvbs_channel_info *channel_info_mem = NULL;
	struct nim_dvbs_bb_header_info *bb_header_info_mem = NULL;
	struct nim_dvbs_isid *isid_mem = NULL;
	struct nim_dvbs_tsn *p_tsn = NULL;
	struct nim_c3501h_private 	*priv = NULL;

	pr_info("[%s %d]enter\n", __FUNCTION__, __LINE__);
	if (NULL == dev)
	{
		pr_err("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	priv = dev->priv;
	
	priv->ul_status.adc_data_malloc_addr = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE * 2);//256K
	if(NULL == priv->ul_status.adc_data_malloc_addr)			
	{
	    pr_err("[%s %d]malloc priv->ul_status.adc_data_malloc_addr fail!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
	comm_memset(priv->ul_status.adc_data_malloc_addr, 0, BYPASS_BUF_SIZE * 2 * sizeof(UINT8));
	
	channel_info_mem = comm_malloc(sizeof(struct nim_dvbs_channel_info));
	if (!channel_info_mem)
	{
		comm_free(priv->ul_status.adc_data_malloc_addr);		
	    pr_err("[%s %d]malloc channel_info_mem failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(channel_info_mem, 0, sizeof(struct nim_dvbs_channel_info));
	priv->channel_info = channel_info_mem;

	bb_header_info_mem = comm_malloc(sizeof(struct nim_dvbs_bb_header_info));
	if (!bb_header_info_mem)
	{
		comm_free(priv->channel_info);
		comm_free(priv->ul_status.adc_data_malloc_addr);
	    pr_err("[%s %d]malloc bb_header_info_mem failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(bb_header_info_mem, 0, sizeof(struct nim_dvbs_bb_header_info));
	priv->bb_header_info = bb_header_info_mem;

	isid_mem = comm_malloc(sizeof(struct nim_dvbs_isid));
	if (!isid_mem)
	{
		comm_free(priv->bb_header_info);
		comm_free(priv->channel_info);
		comm_free(priv->ul_status.adc_data_malloc_addr);
	    pr_err("[%s %d]malloc isid_mem failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(isid_mem, 0, sizeof(struct nim_dvbs_isid));
	priv->isid = isid_mem;

	p_tsn = comm_malloc(sizeof(struct nim_dvbs_tsn));
	if (!p_tsn)
	{
		comm_free(priv->isid);
		comm_free(priv->bb_header_info);
		comm_free(priv->channel_info);
		comm_free(priv->ul_status.adc_data_malloc_addr);
	    pr_err("[%s %d]malloc p_tsn failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(p_tsn, 0, sizeof(struct nim_dvbs_tsn));
	priv->tsn= p_tsn;

	//To solve malloc failure when autoscan
	priv->ul_status.adc_data_malloc_addr = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE * 2);//256K
	if(NULL == priv->ul_status.adc_data_malloc_addr)			
	{
		comm_free(priv->tsn);
		comm_free(priv->isid);
		comm_free(priv->bb_header_info);
		comm_free(priv->channel_info);
		comm_free(priv->ul_status.adc_data_malloc_addr);
		pr_err("[%s %d]malloc adc data failed!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
	

	//To solve malloc failure when autoscan
	if(NULL == channel_spectrum)
	{
	    channel_spectrum = (INT32 *) comm_malloc(FS_MAXNUM * 4);//58K
	    if(channel_spectrum == NULL)
	    {
	    	comm_free(priv->ul_status.adc_data_malloc_addr);
	    	comm_free(priv->tsn);
			comm_free(priv->isid);
			comm_free(priv->bb_header_info);
			comm_free(priv->channel_info);
			comm_free(priv->ul_status.adc_data_malloc_addr);
	        pr_err("[%s %d]malloc channel spectrum failed!\n", __FUNCTION__, __LINE__);
	        return ERR_NO_MEM;
	    }
	}
	
	if(NULL == channel_spectrum_tmp)
	{
		channel_spectrum_tmp = (INT32 *) comm_malloc(FS_MAXNUM * 4);//58K
	   	if(channel_spectrum_tmp == NULL)
	   	{
	   		comm_free(channel_spectrum);
	   		comm_free(priv->ul_status.adc_data_malloc_addr);
	    	comm_free(priv->tsn);
			comm_free(priv->isid);
			comm_free(priv->bb_header_info);
			comm_free(priv->channel_info);
			comm_free(priv->ul_status.adc_data_malloc_addr);
			pr_err("[%s %d]malloc channel_spectrum_tmp failed!\n", __FUNCTION__, __LINE__);
			return ERR_NO_MEM;
	   	}
	}

	pr_info("[%s %d]over\n", __FUNCTION__, __LINE__);
	return SUCCESS;
}

static int ali_nim_c3501h_free_memory(struct nim_device *dev)
{
	struct nim_c3501h_private 	*priv = NULL;

	pr_info("[%s %d]enter\n", __FUNCTION__, __LINE__);
	if (NULL == dev)
	{
		pr_err("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	priv = dev->priv;

	if (channel_spectrum_tmp != NULL)
	{
		comm_free(channel_spectrum_tmp);
	}
	
	if (channel_spectrum != NULL)
	{
		comm_free(channel_spectrum);
	}

	if (priv->ul_status.adc_data_malloc_addr != NULL)
	{
		comm_free(priv->ul_status.adc_data_malloc_addr);
	}

	if (priv->tsn != NULL)
	{
		comm_free(priv->tsn);
	}

	if (priv->isid != NULL)
	{
		comm_free(priv->isid);
	}

	if (priv->bb_header_info != NULL)
	{
		comm_free(priv->bb_header_info);
	}

	if (priv->channel_info != NULL)
	{
		comm_free(priv->channel_info);
	}

	if (priv->ul_status.adc_data_malloc_addr != NULL)
	{
		comm_free(priv->ul_status.adc_data_malloc_addr);
	}

	pr_info("[%s %d]over\n", __FUNCTION__, __LINE__);
	return SUCCESS;
}

static int ali_nim_c3501h_probe(struct platform_device * pdev)
{
    INT32 ret;
	static int dev_num = 0;
    dev_t devno;
	struct nim_device 				*ali_nim_c3501h_dev = NULL;
	struct nim_c3501h_private 		*ali_nim_c3501h_priv = NULL;
	struct device 					*ali_nim_c3501h_dev_node = NULL;
	
    pr_info("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);
	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, 4, ALI_C3501H_DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for ali_nim_c3501h device\n");
		return ret;
	}
	if (g_c3501h_class_create_flag)
	{
	    ali_nim_c3501h_class = class_create(THIS_MODULE, "ali_nim_c3501h_class");
	    if (IS_ERR(ali_nim_c3501h_class))
	    {
	        pr_err("[%s %d]class_create error!\n", __FUNCTION__, __LINE__);
	        ret = PTR_ERR(ali_nim_c3501h_class);
	        return ret;
	    }

		g_c3501h_class_create_flag = FALSE;
	}


	ali_nim_c3501h_dev = comm_malloc(sizeof(struct nim_device));
    if (!ali_nim_c3501h_dev)
    {
    	pr_err("[%s %d]kmalloc ali_nim_c3501h_dev fail\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
    }
	comm_memset(ali_nim_c3501h_dev, 0, sizeof(struct nim_device));

	ali_nim_c3501h_priv = comm_malloc(sizeof(struct nim_c3501h_private));
	if (!ali_nim_c3501h_priv)
	{
		comm_free(ali_nim_c3501h_dev);
	    pr_err("[%s %d]ali_nim_c3501h_priv kmalloc failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(ali_nim_c3501h_priv, 0, sizeof(struct nim_c3501h_private));	

	cdev_init(&ali_nim_c3501h_dev->cdev, &ali_nim_c3501h_fops);
	
	ali_nim_c3501h_dev->cdev.owner = THIS_MODULE;
	ali_nim_c3501h_dev->cdev.ops = &ali_nim_c3501h_fops;
	ali_nim_c3501h_dev->priv = (void *)ali_nim_c3501h_priv;
	ali_nim_c3501h_dev->class = ali_nim_c3501h_class;
	
	ali_nim_c3501h_priv->dev_idx = dev_num;
	ali_nim_c3501h_priv->nim_init = FALSE;
	ali_nim_c3501h_priv->nim_used= 0;

	ret = ali_nim_c3501h_malloc_memory(ali_nim_c3501h_dev);
	if (ret != SUCCESS)
	{
		kfree(ali_nim_c3501h_priv);
		kfree(ali_nim_c3501h_dev);
		pr_err("[%s %d]malloc c3501h memory failed!\n", __FUNCTION__, __LINE__);
		return ret;
	}	

	mutex_init(&ali_nim_c3501h_priv->i2c_mutex);
	mutex_init(&ali_nim_c3501h_priv->multi_process_mutex);
	mutex_init(&ali_nim_c3501h_priv->tuner_open_mutex);
	mutex_init(&ali_nim_c3501h_priv->plsn_mutex);
	mutex_init(&ali_nim_c3501h_priv->tsn_mutex);
	//ali_nim_c3501h_priv->flag_lock.flagid_rwlk = __RW_LOCK_UNLOCKED(ali_nim_c3501h_priv->flagid_rwlk);
	
	ret = cdev_add(&ali_nim_c3501h_dev->cdev, devno, 1);
	if(ret)
	{	   
		mutex_destroy(&ali_nim_c3501h_priv->i2c_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->multi_process_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->tuner_open_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->plsn_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->tsn_mutex);
		ali_nim_c3501h_free_memory(ali_nim_c3501h_dev);
	    kfree(ali_nim_c3501h_priv);
		kfree(ali_nim_c3501h_dev);
		pr_err("[%s %d]cdev_add failed, ret = %d.\n", __FUNCTION__, __LINE__, (int)ret);
		return ret;
	}

	ali_nim_c3501h_dev_node = device_create(ali_nim_c3501h_class, NULL, devno,
	                            ali_nim_c3501h_dev, "ali_nim_c3501h_%d", dev_num);
	if(IS_ERR(ali_nim_c3501h_dev_node))
	{	    
	    ret = PTR_ERR(ali_nim_c3501h_dev_node);
	    cdev_del(&ali_nim_c3501h_dev->cdev);
		mutex_destroy(&ali_nim_c3501h_priv->i2c_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->multi_process_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->tuner_open_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->plsn_mutex);
		mutex_destroy(&ali_nim_c3501h_priv->tsn_mutex);
		ali_nim_c3501h_free_memory(ali_nim_c3501h_dev);
	    kfree(ali_nim_c3501h_priv);
		kfree(ali_nim_c3501h_dev);
		pr_err("[%s %d]device_create() failed, ret = %d\n", __FUNCTION__, __LINE__, (int)ret);
		return ret;
	}	

	ali_nim_c3501h_dev->ali_nim_dev_node = ali_nim_c3501h_dev_node;
	
	dev_num++;
	//platform_set_drvdata(pdev, (void *)ali_nim_c3501h_dev); //add device private data to platform_device data
	pdev->dev.platform_data = ali_nim_c3501h_dev;
	pr_info("[%s %d]end!\n", __FUNCTION__, __LINE__);
	return SUCCESS;
}

static int ali_nim_c3501h_remove(struct platform_device *pdev)
{
    struct nim_device *dev = NULL;
	struct nim_c3501h_private *priv = NULL;

	pr_info("[%s %d]enter!\n", __FUNCTION__, __LINE__);
	
	dev = pdev->dev.platform_data;
	if (dev == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}

	priv = dev->priv;
	if (priv == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}    

	ali_nim_c3501h_free_memory(dev);
	    
    mutex_destroy(&priv->i2c_mutex);
	mutex_destroy(&priv->multi_process_mutex);
	mutex_destroy(&priv->tuner_open_mutex);
	mutex_destroy(&priv->plsn_mutex);		
	mutex_destroy(&priv->tsn_mutex);
	
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
		cancel_delayed_work(&dev->delay_as_work);
        flush_workqueue(dev->autoscan_work_queue);
        destroy_workqueue(dev->autoscan_work_queue);
    }

    if (dev->workqueue)
    {
		cancel_delayed_work(&dev->delay_work);
        flush_workqueue(dev->workqueue);
        destroy_workqueue(dev->workqueue);
    }

	if (dev->plsn_search_workqueue)
    {
		cancel_delayed_work(&dev->delay_plsn_search_work);
        flush_workqueue(dev->plsn_search_workqueue);
        destroy_workqueue(dev->plsn_search_workqueue);
    }
	
	if(dev->channel_change_workqueue)
	{
		cancel_delayed_work(&dev->delay_channel_change_work);
		flush_workqueue(dev->channel_change_workqueue);
		destroy_workqueue(dev->channel_change_workqueue);
	}
	
    kfree(priv);
	cdev_del(&dev->cdev);
	
	if(ali_nim_c3501h_class != NULL)
	{
        class_destroy(ali_nim_c3501h_class);
	}

	if (dev->ali_nim_dev_node != NULL)
	{
		device_del(dev->ali_nim_dev_node);
	}  

    pr_info("[%s %d]end!\n", __FUNCTION__, __LINE__);

	return 0;
}

static int ali_nim_c3501h_suspend(struct platform_device *pdev,pm_message_t state)
{
	UINT16 c3501h_reg_num = 0x0;
	UINT8 reg_value = 0x0;
	struct nim_device *dev = NULL;
	struct nim_c3501h_private *priv = NULL;

	pr_info("[%s %d]enter\n", __FUNCTION__, __LINE__);
	dev = pdev->dev.platform_data;
	if (dev == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}

	priv = dev->priv;
	if (priv == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}
	
	if(!priv->nim_init)
	{
		return SUCCESS;
	}

	priv->nim_init = FALSE;
	
	//sleep demod
	for(c3501h_reg_num = 0;c3501h_reg_num < 0x1c0; c3501h_reg_num++)
	{
		if(nim_reg_read(dev, c3501h_reg_num, &reg_value, 1) < 0)
		{
		#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
			pr_info("Function ali_nim_suspend read error");
		#endif
		}
		//pr_info("[suspend] reg :0x%x = 0x%x\n",c3501h_reg_num,reg_value);
		demod_str_reg_buffer[c3501h_reg_num]=reg_value;
		reg_value = 0x0;
	}

	nim_c3501h_i2c_through_open(dev);
	if (priv->nim_tuner_command !=  NULL)
	{	
		priv->nim_tuner_command(priv->tuner_index, NIM_TURNER_SET_STANDBY, 0);
	}
	
	if (priv->tuner_opened && priv->nim_tuner_close)
    {
		priv->nim_tuner_close(priv->tuner_index);	
    }
	nim_c3501h_i2c_through_close(dev);
	
	if (priv->ext_lnb_command)
		priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_RELEASE_CHIP, 0);

	nim_c3501h_reset_fsm(dev);                                                                                  

    nim_c3501h_hw_close(dev);
	
    priv->work_alive = 0;
    priv->ul_status.c3501h_chanscan_stop_flag = 1;
	priv->ul_status.c3501h_autoscan_stop_flag = 1;	

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
		cancel_delayed_work(&dev->delay_as_work);
        flush_workqueue(dev->autoscan_work_queue);
        destroy_workqueue(dev->autoscan_work_queue);
    }

    if (dev->workqueue)
    {
		cancel_delayed_work(&dev->delay_work);
        flush_workqueue(dev->workqueue);
        destroy_workqueue(dev->workqueue);
    }

	if (dev->plsn_search_workqueue)
    {
		cancel_delayed_work(&dev->delay_plsn_search_work);
        flush_workqueue(dev->plsn_search_workqueue);
        destroy_workqueue(dev->plsn_search_workqueue);
    }
	
	if(dev->channel_change_workqueue)
	{
		cancel_delayed_work(&dev->delay_channel_change_work);
		flush_workqueue(dev->channel_change_workqueue);
		destroy_workqueue(dev->channel_change_workqueue);
	}
	
#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_del(&priv->flag_lock);
#endif

	ali_destroy_kumsgq(priv->nim_kumsgq);
	priv->nim_kumsgq = NULL;

	return SUCCESS;
}

static int ali_nim_c3501h_resume(struct platform_device *pdev)
{
	UINT16 c3501h_reg_num = 0x0;
	UINT8 reg_value = 0x0;
	struct nim_device *dev = NULL;
	struct nim_c3501h_private *priv = NULL;

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		pr_info("Function ali_nim_resume....===");
#endif

	dev = pdev->dev.platform_data;
	if (dev == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}

	priv = dev->priv;
	if (priv == NULL)
	{
		pr_err("[%s %d]dev == NULL\n", __FUNCTION__, __LINE__);
		//return -EINVAL;
		return SUCCESS;
	}

	if(!priv->nim_init)
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		pr_info("c3501h ali_nim_suspend have not init");
	#endif
		return SUCCESS;
	}

	//wakeup demod
	for(c3501h_reg_num=0;c3501h_reg_num<0x1c0;c3501h_reg_num++)
	{		
		reg_value = demod_str_reg_buffer[c3501h_reg_num];
		if(nim_reg_write(dev, c3501h_reg_num, &reg_value, 1) < 0)
		{
		#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
			pr_info("Function ali_nim_resume write error");
		#endif
		}
		reg_value = 0x0;
	}

	if(pdev->id ==1)
	{
		return SUCCESS;
	}

	nim_c3501h_i2c_through_open(dev);
	//wakeup tuner
	if (priv->nim_tuner_command != NULL)
	{
		priv->nim_tuner_command(priv->tuner_index, NIM_TUNER_WAKE_UP, 0);
	}
	else
	{
		pr_err("[%s %d]function: nim_tuner_control don't find\n", __FUNCTION__, __LINE__);
		nim_c3501h_i2c_through_close(dev);
		return -EFAULT;
	}
	nim_c3501h_i2c_through_close(dev);
	
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	pr_info("nim tuner wakeup...");
#endif
	
	return SUCCESS;
}

static	const	struct	of_device_id	ali_nim_c3501h_match[]	= {
				{	.compatible	= "alitech, nim_c3501h",	},
				{},
};

MODULE_DEVICE_TABLE(of, ali_nim_c3501h_match);

static struct platform_driver nim_platform_driver = {
	.probe   = ali_nim_c3501h_probe, 
	.remove   = ali_nim_c3501h_remove,
	.suspend  = ali_nim_c3501h_suspend,
	.resume   = ali_nim_c3501h_resume,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = ALI_C3501H_DEVICE_NAME,
			.of_match_table	= ali_nim_c3501h_match,
	},
};

module_platform_driver(nim_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robin.Gan");
MODULE_DESCRIPTION("Ali C3501H Demod driver");

