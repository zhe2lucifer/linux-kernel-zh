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
*	File:	This file contains C3505 DVBS2 system API and hardware 
*           open/closed operate in LLD.
*
*    Description:    
*    History:'refer to update_history.txt'
*******************************************************************************/

#include "nim_dvbs_c3505.h"
#include "../tun_common.h"
#include <linux/platform_device.h>
#include <linux/ali_kumsgq.h>
#include <ali_soc.h>

#define MAX_TUNER_SUPPORT_NUM 	2
#define MAX_DEMOD_SUPPORT_NUM 	2
#define ALI_NIM_DEVICE_NAME 	"ali_nim_c3505"

//struct nim_device 				ali_c3505_nim_dev[MAX_TUNER_SUPPORT_NUM];
//struct nim_c3505_private 		*ali_c3505_nim_priv[MAX_TUNER_SUPPORT_NUM] = {NULL};
bool g_c3505_class_create_flag = TRUE;
struct class 					*ali_c3505_nim_class;
struct device 					*ali_c3505_nim_dev_node[MAX_TUNER_SUPPORT_NUM];
UINT8  *dram_base_t 			= NULL;

//static UINT8 demod_mutex_flag = 0;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
UINT32 demod_str_reg_buffer[500] = {0}; //save the reg value when str
static int ali_nim_suspend(struct platform_device *pdev,pm_message_t state);
static int ali_nim_resume(struct platform_device *pdev);
#endif

void nim_usb_log(char* msg);


static char nim_workq[MAX_TUNER_SUPPORT_NUM][5] =
{
    "nim0"
};

static char as_workq[MAX_TUNER_SUPPORT_NUM][12] =
{
    "as_workq0"
};

static char plsn_gen_tab_workq[] = "plsn_gen_tab";
static char plsn_search_workq[] = "plsn_search";
static char channel_change_workq[] = "channel_change_wq";


__u32 nim_c3505_dvbs_as_cb2_ui(void *p_priv, __u8 lck, __u8 polar, __u16 freq, __u32 sym, __u8 fec, __u8 as_stat)
{
    struct nim_c3505_private *priv = (struct nim_c3505_private *)p_priv;
    priv->as_status = 0;

    nim_send_as_msg(priv->nim_kumsgq, lck, polar, freq, sym, fec, as_stat);
    wait_event_interruptible_timeout(priv->as_sync_wait_queue, priv->as_status & 0x01, 0x7fffffff);
    priv->as_status = 0;

    return SUCCESS;
}

/*****************************************************************************
Name:
    multu64div
Description:
    This function implment v1*v2/v3. And v1*v2 is 64 bit
Parameters:
    [IN]
    [OUT]
Return:
***********************************************************************/
//Get HI value from the multple result
#ifdef CONFIG_ARM
DWORD nim_c3505_multu64div(UINT32 v1, UINT32 v2, UINT32 v3)
{
    UINT32 v = 0;
    UINT64 tmp = 0;
    if(v3 == 0)
    {
        return 0;
    }
    tmp = (UINT64)v1 * (UINT64)v2;
    while(tmp >= v3)
    {
        tmp = tmp - v3;
        v++;
    }
    return v; 
}
#else
//kent,ok
# define mult64hi(v1, v2)           \
({  DWORD __ret;            \
    __asm__ volatile(                   \
        "multu  %1, %2\n    "               \
        "mfhi %0\n" \
         : "=r" (__ret)         \
         : "r" (v1), "r"(v2));                  \
    __ret;                      \
})

//Get LO value from the multple result
# define mult64lo(v1, v2)           \
({  DWORD __ret;            \
    __asm__ volatile(                   \
        "multu  %1, %2\n    "               \
        "mflo %0\n" \
         : "=r" (__ret)         \
         : "r" (v1), "r"(v2));                  \
    __ret;                      \
})

DWORD nim_c3505_multu64div(UINT32 v1, UINT32 v2, UINT32 v3)
{
    DWORD                 hi, lo;
    unsigned long long     tmp;
    DWORD                 *tmp64;
    DWORD                 ret;
    if (v3 == 0)
        return 0;
    hi = mult64hi(v1, v2);
    lo = mult64lo(v1, v2);
    tmp64 = ((DWORD *)(&tmp)) + 1;
    *tmp64-- = hi;
    *tmp64 = lo;
    //Few nop here seems required, if no nop is here,
    //then the result wont be correct.
    //I guess maybe this is caused by some GCC bug.
    //Because I had checked the compiled result between with nop and without nop,
    //they are quite different!! I dont have time to search more GCC problem,
    //Therefore, I can only leave the nop here quietly. :(
    //--Michael 2003/10/10
    __asm__("nop; nop; nop; nop");
    do_div(tmp, v3);
    ret = tmp;
    //ret = tmp/v3;  //kent
    return ret;
}

#endif


INT32 nim_c3505_crc8_check(void *input, INT32 len,INT32 polynomial)
{
	/*
	   Func: Implement CRC decoder based on the structure of 
	         figure 2 in DVB-S2 specification (ETSI EN 302 307).
	*/

	INT32 i = 0;
	INT32 j = 0;
	INT32 bit_idx     = 0;

	INT8 *byte_data = NULL;
	INT8 curr_byte  = 0;
	UINT8 shift_reg  = 0;
	INT8 xor_flag   = 0;
	INT8  bit_data  = 0;

    //polynomial = 0x56;// 0x56  test
	byte_data = (INT8 *)input;
	curr_byte = byte_data[i];
	
	while (bit_idx < len )
	{
		bit_data = (curr_byte & 0x80) >> 7;
		bit_idx ++;
		j ++;

		// get one bit  from MSB of a byte
		if (j == 8)
		{
			curr_byte = byte_data[++i]; // input new byte 
			j = 0;
		}
		else
		{   // get next bit of curr_byte
			curr_byte <<= 1;
		}

		// crc check process
		xor_flag = bit_data ^ (shift_reg & 0x01);
		if (xor_flag)
		{
			shift_reg ^= polynomial;  // 0x56 is derived by the tap structure in figure 2 of DVB-S2 Spec.
		}

		shift_reg >>= 1;
		if (xor_flag)
		{
			shift_reg |= 0x80;  // MSB of shift_reg is set to 1.
		}

		//BBFRAME_PRINTF("%d\n",shift_reg);
	}

	if (0 == shift_reg)
	{
		return SUCCESS;
	}
	else
	{
		return ERR_FAILED;
	}
}


UINT32 nim_c3505_Log10Times100_L( UINT32 x)
{
	const UINT8 scale=15;
	const UINT8 indexWidth=5;
	/*
	log2lut[n] = (1<<scale) * 200 * log2( 1.0 + ( (1.0/(1<<INDEXWIDTH)) * n ))
	0 <= n < ((1<<INDEXWIDTH)+1)
	*/
	const UINT32 log2lut[] = {
	0, 290941,  573196,	847269,1113620, 1372674, 1624818, 
	1870412, 2109788, 2343253, 2571091, 2793569,3010931, 
	3223408, 3431216, 3634553, 3833610, 4028562, 4219576, 
	4406807, 4590402, 4770499, 4947231, 5120719, 5291081, 
	5458428, 5622864, 5784489, 5943398,	6099680, 6253421, 
	6404702,  6553600  };

	UINT8  i = 0;
	UINT32 y = 0;
	UINT32 d = 0;
	UINT32 k = 0;
	UINT32 r = 0;
 
	if (x==0) return (0);
 
	/* Scale x (normalize) */
	/* computing y in log(x/y) = log(x) - log(y) */
	if ( (x & (((UINT32)(-1))<<(scale+1)) ) == 0 )
	{
	   for (k = scale; k>0 ; k--)
	   {
		 if (x & (((UINT32)1)<<scale)) break;
		 x <<= 1;
	   }
	} else {
	   for (k = scale; k<31 ; k++)
	   {
		 if ((x & (((UINT32)(-1))<<(scale+1)))==0) break;
		 x >>= 1;
	   }
	}
	/*
	  Now x has binary point between bit[scale] and bit[scale-1]
	  and 1.0 <= x < 2.0 */
 
	/* correction for divison: log(x) = log(x/y)+log(y) */
	y = k * ( ( ((UINT32)1) << scale ) * 200 );
 
	/* remove integer part */
	x &= ((((UINT32)1) << scale)-1);
	/* get index */
	i = (UINT8) (x >> (scale -indexWidth));
	/* compute delta (x-a) */
	d = x & ((((UINT32)1) << (scale-indexWidth))-1);
	/* compute log, multiplication ( d* (.. )) must be within range ! */
	y += log2lut[i] + (( d*( log2lut[i+1]-log2lut[i] ))>>(scale-indexWidth));
	/* Conver to log10() */
	y /= 108853; /* (log2(10) << scale) */
	r = (y>>1);
	/* rounding */
	if (y&((UINT32)1)) r++;
 
	return (r);
}

INT32 call_tuner_command(struct nim_device *dev, INT32 cmd, INT32 *param)
{

    struct nim_c3505_private *priv = (struct nim_c3505_private *) dev->priv;
	
	if((NULL == priv) || (NULL == dev))
	{
		return ERR_FAILUE;
	}

	if (priv->nim_tuner_command)
    {
    	return priv->nim_tuner_command(priv->tuner_index, cmd, param);
	}
    else
    {
    	return ERR_FAILUE;
    }
}
/*****************************************************************************
*  void nim_c3505_task(UINT32 param1, UINT32 param2)
*  Task of nim driver,  do some monitor or config
*
* Arguments:
*  Parameter1:  device struct point
*  Parameter2:  unused
*
* Return Value: INT32
*****************************************************************************/
void nim_c3505_task(UINT32 param1, UINT32 param2)
{
	#ifdef DEBUG_IN_TASK
    UINT32 v_cnt_val=0x00;
    #endif
	struct nim_device *dev = (struct nim_device *) param1 ;
	struct nim_c3505_private *priv = (struct nim_c3505_private *) dev->priv ;
	UINT8 work_mode = 0;
    
	priv->tsk_status.m_sym_rate = 0x00;
	priv->tsk_status.m_code_rate = 0x00;
	priv->tsk_status.m_map_type = 0x00;
	priv->tsk_status.m_work_mode = 0x00;
	priv->tsk_status.m_info_data = 0x00;
    
	NIM_PRINTF("            Enter nim_C3505_task:\n");

 // Has been locked
	if(priv->tsk_status.m_lock_flag == NIM_LOCK_STUS_CLEAR)
	{
		;
	}
	else
	{   // Waitting lock(lock process han been break by chanscan_stop_flag ) and i2c is normal 
		if ((priv->tsk_status.m_lock_flag == NIM_LOCK_STUS_SETTING) && (priv->t_param.t_i2c_err_flag == 0x00))
		{
			nim_c3505_get_lock(dev, &(priv->tsk_status.m_info_data));
            // Found locked then refresh the tp information
			if (priv->tsk_status.m_info_data && (priv->t_param.t_i2c_err_flag == 0x00))
			{
				nim_c3505_get_symbol_rate(dev, &(priv->tsk_status.m_sym_rate));
				nim_c3505_reg_get_code_rate(dev, &(priv->tsk_status.m_code_rate));
				nim_c3505_reg_get_work_mode(dev, &(priv->tsk_status.m_work_mode));
				nim_c3505_reg_get_map_type(dev, &(priv->tsk_status.m_map_type));
				nim_c3505_interrupt_clear(dev);
				priv->tsk_status.m_lock_flag = NIM_LOCK_STUS_CLEAR;
			}
		}
	}
            
	if(priv->search_type == NIM_OPTR_CHL_CHANGE)
	{					
	#ifdef ANTI_WIMAX_INTF
				if(force_adpt_disable == 1)
				   {
					   nim_c3505_cr_new_adaptive_unlock_monitor(dev);
					   nim_c3505_nframe_step_tso_setting(dev,100,0x00);
				   }
				   else
				   {
					   nim_c3505_task_tso_setting(dev,100,0x00);
				   }
	#else		 
					nim_c3505_cr_new_adaptive_unlock_monitor(dev);
					nim_c3505_nframe_step_tso_setting(dev,100,0x00);
	#endif	
			 }

	#if (NIM_OPTR_CCM == ACM_CCM_FLAG)	
        nim_c3505_get_per(dev , &(priv->channel_info->per));
		priv->channel_info->period_per+= priv->channel_info->per;
	#else
		nim_c3505_reg_get_work_mode(dev, &work_mode);
		if (1 == work_mode)
       		nim_c3505_get_fer(dev , (UINT32 *)(&(priv->channel_info->per)));
		else
			{
				nim_c3505_get_per(dev , (UINT32 *)(&(priv->channel_info->per)));
				priv->channel_info->period_per+= priv->channel_info->per;
			}
	#endif
	if (priv->tsk_status.m_info_data) {
    		nim_c3505_get_snr_db(dev, &(priv->channel_info->snr));
	}    
	nim_c3505_get_phase_error(dev , (INT32 *)(&(priv->channel_info->phase_err)));
    nim_c3505_get_mer_task(dev);
	//when channel change or autoscan finished,the following function can be executed
    if (0 == priv->chanscan_autoscan_flag)
	{
		 mutex_lock(&priv->multi_process_mutex);
		 #ifdef DEBUG_IN_TASK 
	     nim_c3505_debug_intask(dev);
		 #endif  

		 #ifdef IMPLUSE_NOISE_IMPROVED
	     nim_c3505_auto_adaptive(dev);    
		 #endif
		/*monitor demod whether fake lock or not*/
		 nim_c3505_mon_fake_lock(dev);
		/*For fc searh/ppll/CR Feedback Carrier Threshold adaptive */
		 nim_c3505_lock_unlock_adaptive(dev);
	     mutex_unlock(&priv->multi_process_mutex);
	 } 
	if(priv->work_alive)
		queue_delayed_work(dev->workqueue, &dev->delay_work,TIMER_DELAY * HZ / 1000);
    NIM_PRINTF("Run out of %s \n", __FUNCTION__);
	
}

extern INT32 *plsn_sq_rn_pilot_table;
void nim_c3505_generate_table_task(struct delayed_work *work)
{
	struct nim_device *dev = NULL;
	struct nim_c3505_private *priv = NULL;
	UINT32 gen_start = 0;
	
	dev = container_of((void *)work, struct nim_device, delay_plsn_gen_table_work);	

	priv = (struct nim_c3505_private *) dev->priv;
	
	PLSN_PRINTF("start generate table!\n");
	gen_start = jiffies;
	nim_c3505_generate_table(dev);
	PLSN_PRINTF("generate tabel cost:%d ms\n", jiffies - gen_start);	
}

void nim_c3505_plsn_task(struct delayed_work *work)
{    
	struct nim_device *dev = NULL;
	struct nim_c3505_private *priv = NULL;
	UINT32 start = 0;
	
	NIM_PRINTF("Enter %s \n", __FUNCTION__);
	dev = container_of((void *)work, struct nim_device, delay_plsn_search_work);

	priv = (struct nim_c3505_private *) dev->priv;
	
	//priv->plsn.super_scan is set by upper level
	//priv->plsn.start_search is set in nim_c3505_channel_change and nim_c3505_soft_search
	//priv->ul_status.c3505_chanscan_stop_flag is cleared in nim_c3505_waiting_channel_lock
	//priv->plsn.auto_scan_start is set in nim_c3505_soft_search
	PLSN_PRINTF("start to search plsn!\n");
	start = jiffies;
	priv->plsn.search_plsn_force_stop = 0;
	PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_force_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_force_stop);
	priv->plsn.search_plsn_stop = 0;
	PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_stop);
	priv->plsn.plsn_find = 0;
	priv->plsn.plsn_num = 0;
	priv->plsn.plsn_try = 0;
	priv->plsn.plsn_try_val = 0;
	priv->plsn.auto_scan_start = 0;
	PLSN_PRINTF("priv->plsn.auto_scan_start=%d\n", priv->plsn.auto_scan_start);
	memset(priv->plsn.plsn_val, 0, sizeof(priv->plsn.plsn_val));

	nim_c3505_search_plsn_top(dev);
		
	priv->plsn.start_search = 0;

	PLSN_PRINTF("search end, cost %d ms\n", jiffies - start);
	NIM_PRINTF("%s search end, cost %d ms\n",  __FUNCTION__,(int)(jiffies - start));
}

static void nim_c3505_task_open(struct delayed_work *work)
{
    struct nim_device 				*dev;
	
    dev = container_of((void *)work, struct nim_device, delay_work);
    NIM_PRINTF("[kangzh]line=%d,%s enter!\n", __LINE__, __FUNCTION__);
    nim_c3505_task((UINT32)dev, 0);
}
static void nim_c3505_channel_change_wq(struct delayed_work *work)
{
	struct nim_device 				*dev;
	struct nim_c3505_private 		*priv;
	UINT8 lock = 0;
	INT32 ret = SUCCESS;
	
    dev = container_of((void *)work, struct nim_device, delay_channel_change_work);
	priv = dev->priv;
	
	nim_flag_clear(&priv->flag_lock, NIM_FLAG_CHN_CHG_START);
    nim_flag_set(&priv->flag_lock, NIM_FLAG_CHN_CHANGING);
	priv->wait_chanlock_finish_flag = 0;
	ret = nim_c3505_waiting_channel_lock(dev, priv->cur_freq, priv->cur_sym, priv->change_type, priv->isid);
	//demod_mutex_flag = 0;
	if (SUCCESS != ret)
      {   
	  	nim_c3505_get_lock(dev, &lock);
        if (0 == lock)
             nim_c3505_set_work_mode(dev, 0x03); // Lock fail, then need to set work mode to auto
		if (ERR_TIME_OUT == ret)
			priv->chanscan_autoscan_flag = 0;//when time out ,it mean that channel changel finish
	  }
	else
	  {
		priv->chanscan_autoscan_flag = 0;//channel changel finish
	  }
	priv->wait_chanlock_finish_flag = 1;
	nim_flag_clear(&priv->flag_lock, NIM_FLAG_CHN_CHANGING);
	NIM_PRINTF("\t\t Here is the task for C3505 wait channel lock \n");

}

static void nim_c3505_autoscan_open(struct delayed_work *work)
{
    NIM_AUTO_SCAN_T					*pstauto_scan;
    struct nim_device 				*dev;
    struct nim_c3505_private 		*priv;

    //S3503_PRINTF(NIM_LOG_DBG,"[kangzh]line=%d,%s enter!\n",__LINE__, __FUNCTION__);

    dev = container_of((void *)work, struct nim_device, delay_as_work);
    priv = dev->priv;
    pstauto_scan = &priv->as_info;

    nim_c3505_autoscan(dev, pstauto_scan);
}

static INT32 nim_c3505_task_init(struct nim_device *dev)
{
    UINT8 dev_idx = 0;
    struct nim_c3505_private 	*priv = dev->priv;

    NIM_PRINTF("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

	dev_idx = priv->dev_idx;
    dev->workqueue = create_workqueue(nim_workq[dev_idx]);

    if (!(dev->workqueue))
    {
        NIM_PRINTF("Failed to allocate work queue\n");
        return -1;
    }

    dev->autoscan_work_queue = create_workqueue(as_workq[dev_idx]);
    if (!(dev->autoscan_work_queue))
    {
        NIM_PRINTF("%s in:Failed to create nim autoscan workequeue!\n",__FUNCTION__);
        destroy_workqueue(dev->workqueue);
        return -1;
    }

	dev->plsn_gen_table_workqueue = create_workqueue(plsn_gen_tab_workq);
    if (!(dev->plsn_gen_table_workqueue))
    {
        NIM_PRINTF("%s in:Failed to create nim autoscan workequeue!\n",__FUNCTION__);
        destroy_workqueue(dev->workqueue);
		destroy_workqueue(dev->autoscan_work_queue);
        return -1;
    }

	dev->plsn_search_workqueue = create_workqueue(plsn_search_workq);
    if (!(dev->plsn_search_workqueue))
    {
        NIM_PRINTF("%s in:Failed to create nim autoscan workequeue!\n",__FUNCTION__);
        destroy_workqueue(dev->workqueue);
		destroy_workqueue(dev->autoscan_work_queue);
		destroy_workqueue(dev->plsn_gen_table_workqueue);
        return -1;
    }
	dev->channel_change_workqueue= create_workqueue(channel_change_workq);
    if (!(dev->channel_change_workqueue))
    {
        NIM_PRINTF("%s in:Failed to create nim autoscan workequeue!\n",__FUNCTION__);
        destroy_workqueue(dev->workqueue);
		destroy_workqueue(dev->autoscan_work_queue);
		destroy_workqueue(dev->plsn_gen_table_workqueue);
		destroy_workqueue(dev->plsn_search_workqueue);
        return -1;
    }
	
    init_waitqueue_head(&priv->as_sync_wait_queue);
    priv->work_alive = 1;
	priv->ul_status.c3505_chanscan_stop_flag = 0;

	INIT_DELAYED_WORK(&dev->delay_work,(void *)nim_c3505_task_open);
	INIT_DELAYED_WORK(&dev->delay_plsn_gen_table_work, (void *)nim_c3505_generate_table_task);
	INIT_DELAYED_WORK(&dev->delay_plsn_search_work, (void *)nim_c3505_plsn_task);
    INIT_DELAYED_WORK(&dev->delay_as_work, (void *)nim_c3505_autoscan_open);
	INIT_DELAYED_WORK(&dev->delay_channel_change_work,(void *)nim_c3505_channel_change_wq);
	queue_delayed_work(dev->workqueue, &dev->delay_work,TIMER_DELAY * HZ / 1000);
	
    return SUCCESS;
}

static INT32 ali_nim_c3505_hw_initialize(struct nim_device *dev, struct ali_nim_m3501_cfg *nim_cfg)
{
    INT32 	ret = 0;
	INT32	channel_freq_err = 0;
	TUNER_IO_FUNC               *p_io_func = NULL;
    struct nim_c3505_private 	*priv = dev->priv;
	UINT8  data = 0;

    NIM_PRINTF("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

	if(priv->nim_init)
	{	
		NIM_PRINTF("[%s]line=%d,C3505 have inited! priv->nim_init=%d!\n", __FUNCTION__, __LINE__, priv->nim_init);
    	return RET_SUCCESS;
	}

	//diseqc state inits
	priv->diseqc_info.diseqc_type = 0;
    priv->diseqc_info.diseqc_port = 0;
    priv->diseqc_info.diseqc_k22 = 0;
	if ((priv->tuner_config_data.qpsk_config & C3505_POLAR_REVERT) == C3505_POLAR_REVERT) //bit4: polarity revert.
		priv->diseqc_info.diseqc_polar = 2;//LNB_POL_V;
	else //default usage, not revert.
		priv->diseqc_info.diseqc_polar = 1;//LNB_POL_H;

    //priv->diseqc_typex = 0;
    //priv->diseqc_portx = 0;
    
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
	priv->autoscan_debug_flag = 0;

    priv->ul_status.m_enable_dvbs2_hbcd_mode = 0;
    priv->ul_status.m_dvbs2_hbcd_enable_value = 0x7f;
    priv->ul_status.nim_c3505_sema = OSAL_INVALID_ID;
    priv->ul_status.c3505_autoscan_stop_flag = 0;
    priv->ul_status.c3505_chanscan_stop_flag = 0;
    priv->ul_status.old_ber = 0;
    priv->ul_status.old_per = 0;
    priv->ul_status.m_hw_timeout_thr = 0;
    priv->ul_status.old_ldpc_ite_num = 0;
    priv->ul_status.c_rs = 0;
    priv->ul_status.phase_err_check_status = 0;
    priv->ul_status.c3505_lock_status = NIM_LOCK_STUS_NORMAL;
    priv->ul_status.m_c3505_type = 0x00;
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

	p_io_func = tuner_setup(NIM_DVBS,nim_cfg->tuner_id);
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
		NIM_PRINTF("[%s %d]set tuner error! p_io_func==NULL!\n", __FUNCTION__, __LINE__);
		return ERR_FAILURE;
	}

    // Initial the QPSK Tuner
    if (priv->nim_tuner_init != NULL)
    {
        NIM_PRINTF(" Initial the Tuner \n");
        if (priv->nim_tuner_init(&priv->tuner_index, &(priv->tuner_config)) != SUCCESS)
        {
            NIM_PRINTF("Error: Init Tuner Failure!\n");
            return ERR_NO_DEV;
        }
		NIM_PRINTF("[%s %d]priv->tuner_index=%d\n", __FUNCTION__, __LINE__, (int)priv->tuner_index);
        priv->tuner_opened = 1;

		if(SUCCESS == call_tuner_command(dev, NIM_TUNER_M3031_ID, &channel_freq_err))
		{
			priv->tuner_type = IS_M3031;
			NIM_PRINTF("[%s]line=%d,M3031 Tuner!\n", __FUNCTION__, __LINE__);
		}
		else
		{
			priv->tuner_type = NOT_M3031;
			NIM_PRINTF("[%s]line=%d, NOT M3031 Tuner!\n", __FUNCTION__, __LINE__);
		}
    }
	else
	{
		NIM_PRINTF("[%s %d]priv->nim_tuner_init is NULL!\n", __FUNCTION__, __LINE__);
		return ERR_FAILURE;
	}

	if(priv->ext_lnb_type != 0)
	{
		NIM_PRINTF("[%s %d] Start set ext_lnb_type %d\n",__FUNCTION__,__LINE__,(int)priv->ext_lnb_type);
		nim_c3505_set_ext_lnb(dev);
	}
    nim_c3505_reg_get_chip_type(dev);
#if 0
    if ((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3503) && 			// Chip 3501A
            ((priv->tuner_config_data.qpsk_config & 0xc0) == M3501_2BIT_MODE))	//TS 2bit mode
    {
        NIM_PRINTF("[kangzh]line=%d,%s 4!\n", __LINE__, __FUNCTION__);
        //for M3606+M3501A full nim ssi-2bit patch, auto change to 1bit mode.
        priv->tuner_config_data.qpsk_config &= 0x3f; // set to TS 1 bit mode
    }
#endif

	data = 0x00;
	nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);
	
    //nim_s3503_set_acq_workmode(dev, NIM_OPTR_HW_OPEN);

    ret = nim_c3505_hw_check(dev);
    if (ret != SUCCESS)
    {
        NIM_PRINTF("[%s] line=%d,error,back!\n", __FUNCTION__, __LINE__);
        return ret;
    }

	if ((NIM_OPTR_ACM == ACM_CCM_FLAG) && (CHIP_ID_3503C == priv->ul_status.m_c3505_type))
    {
        NIM_PRINTF("Error, C3503c can not support ACM work mode, Exit!\n");
        return ERR_FAILED;
    }
	
    nim_c3505_hw_init(dev);
	nim_c3505_after_reset_set_param(dev);

    //nim_s3503_hbcd_timeout(dev, NIM_OPTR_HW_OPEN);

    nim_c3505_task_init(dev);

#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_create(&priv->flag_lock);
#endif
    NIM_PRINTF("[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
	priv->nim_init = TRUE;

	return RET_SUCCESS;
}

static long ali_c3505_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    INT32 ret = 0;

    struct nim_device 			*dev = file->private_data;
    struct nim_c3505_private 	*priv = dev->priv;
    unsigned long 				arg = (unsigned long) parg;
	UINT32 plsn_value;
	UINT32 temp_value;
	struct ali_plsn_address plsn_addr;
	UINT8 work_mode = 0;
	UINT8 fast_lock_detect = 0;

	mutex_lock(&priv->multi_process_mutex);
    switch (cmd)
    {
    case ALI_NIM_HARDWARE_INIT_S:
    {
        struct ali_nim_m3501_cfg nim_param;
		
        if(copy_from_user(&nim_param, (struct ali_nim_m3501_cfg*)parg, sizeof(struct ali_nim_m3501_cfg))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}			
        ret = ali_nim_c3505_hw_initialize(dev, &nim_param);
        break;
    }
    case ALI_NIM_SET_POLAR:
    {
        UINT8 polar_param = 0;

        get_user(polar_param, (unsigned char *)parg);
        ret = nim_c3505_set_polar(dev, polar_param);
        break;
    }
    case ALI_NIM_CHANNEL_CHANGE:
    {
        NIM_CHANNEL_CHANGE_T nim_param;

        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		//demod_mutex_flag = 1;
		priv->chanscan_autoscan_flag = 1;//Enter channelchange function
        ret = nim_c3505_channel_change(dev, &nim_param);
		#ifndef CHANNEL_CHANGE_ASYNC
            //demod_mutex_flag = 0;
             priv->chanscan_autoscan_flag = 0;//Leave channelchange function
        #endif
		if (SUCCESS != ret)
    	{
        	priv->chanscan_autoscan_flag = 0;//Leave channelchange function
        }
		break;
    }
	case ALI_NIM_LOG_LEVEL:
	{
        UINT32 log_level= 0;

		if(copy_from_user(&log_level, (UINT32 *)parg, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
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

        nim_c3505_get_lock(dev, &lock);
        ret = lock;

        break;
    }
    case ALI_NIM_READ_QPSK_BER:
    {
        UINT32 ber = 0;

        nim_c3505_get_ber(dev, &ber);
        ret = ber;
        break;
    }
	case ALI_NIM_READ_PRE_BER:
	{
		UINT32 pre_ber = 0;
		nim_c3505_check_ber(dev,&pre_ber);
		ret = pre_ber;
		break;
	}
    case ALI_NIM_READ_RSUB:
    {
        UINT32 per = 0;

        nim_c3505_get_per(dev, &per);
        ret = per;
        break;
    }
    case ALI_NIM_READ_AGC:
    {
        UINT8 agc = 0;

        nim_c3505_get_agc(dev, &agc);
        ret = agc;
        break;
    }
	case ALI_NIM_GET_RF_LEVEL : //uint 0.1dbm range:s(0--14) s2(0-25);precison:(-+1dbm)
	{
		INT8 rf_value = 0;
		INT32 tmp = 0;
		nim_c3505_get_agc_dbm(dev,&rf_value);
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
 
        nim_c3505_get_snr(dev, &snr);
        ret = snr;

        break;
    }
	case ALI_NIM_GET_CN_VALUE: //uint 0.01db,range :0-25;precision: 0-20:(+-1db),>25:(+-2db)
	{	
		INT16 cn_value = 0;
		nim_c3505_get_snr_db(dev,&cn_value);//snr uint is 0.01 db
		ret = cn_value;
		break;
	}
	case ALI_NIM_GET_MER: //uint 0.1db
	{
		UINT32 mer = 0;
		nim_c3505_get_mer(dev, &mer);
		ret = mer;
		break;
	}
    case ALI_NIM_READ_SYMBOL_RATE:
    {
        UINT32 sym = 0;

        nim_c3505_get_symbol_rate(dev, &sym);
        ret = sym;

        break;
    }
    case ALI_NIM_READ_FREQ:
    {
        UINT32 freq = 0;
	
		ret = nim_c3505_get_cur_freq(dev ,&freq);
        break;
    }
    case ALI_NIM_READ_CODE_RATE:
    {
        UINT8 fec = 0;

        nim_c3505_reg_get_code_rate(dev, &fec);
        ret = fec;

        break;
    }
    case ALI_NIM_AUTO_SCAN:          /* Do AutoScan Procedure */
    {
		NIM_AUTO_SCAN_T as_load;
        flush_workqueue(dev->autoscan_work_queue);
        
        if(copy_from_user(&as_load, (NIM_AUTO_SCAN_T *)parg, sizeof(NIM_AUTO_SCAN_T))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}			
        priv->yet_return = FALSE;
        priv->as_info.sfreq = as_load.sfreq;
        priv->as_info.efreq = as_load.efreq;
        priv->as_info.unicable = as_load.unicable;
        priv->as_info.fub = as_load.fub;
        priv->as_info.callback = nim_c3505_dvbs_as_cb2_ui;
		priv->autoscan_control = NORMAL_MODE;

        //demod_mutex_flag = 1;
		priv->chanscan_autoscan_flag = 1;//Enter autoscan function
        ret = queue_delayed_work(dev->autoscan_work_queue, &dev->delay_as_work, 0);
		//demod_mutex_flag = 0;
		priv->chanscan_autoscan_flag = 0;//leave autoscan funtion
        break;
    }
	
    case ALI_NIM_STOP_AUTOSCAN:
        priv->ul_status.c3505_autoscan_stop_flag = arg;
        break;
		
    case ALI_NIM_DISEQC_OPERATE:
    {
        struct ali_nim_diseqc_cmd dis_cmd;

        if(copy_from_user(&dis_cmd, (struct ali_nim_diseqc_cmd *)parg, sizeof(struct ali_nim_diseqc_cmd))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}				
			
        switch(dis_cmd.diseqc_type)
        {
        case 1:
            ret = nim_c3505_diseqc_operate(dev, dis_cmd.mode, dis_cmd.cmd, dis_cmd.cmd_size);
            break;
        case 2:
            ret = nim_c3505_diseqc2x_operate(dev, dis_cmd.mode, \
                                             dis_cmd.cmd, dis_cmd.cmd_size, dis_cmd.ret_bytes, &dis_cmd.ret_len);
            if(copy_to_user((struct ali_nim_diseqc_cmd *)parg, &dis_cmd, sizeof(struct ali_nim_diseqc_cmd))>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
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
		if (copy_from_user(&flags, (int *)parg, sizeof(int))>0)
		{
			pr_err("[%s %d]error\n", __FUNCTION__, __LINE__);
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
	            ret = priv->ul_status.m_setting_freq;
	        case NIM_FREQ_RETURN_REAL:
	        default:
	           {
			   		UINT8 fec = 0;
					ret = nim_c3505_reg_get_code_rate(dev, &fec);
	        	}
        }
    case ALI_NIM_TURNER_SET_STANDBY:

        if (priv->nim_tuner_command != NULL)
        {
            priv->nim_tuner_command(priv->tuner_index, NIM_TURNER_SET_STANDBY, 0);
        }

        break;
    case ALI_NIM_DRIVER_GET_ID:

        ret = priv->ul_status.m_c3505_type;

        break;

    case ALI_NIM_REG_RW:
        {
	        UINT8 reg_rw_cmd[16] ={0};
            //printk("[%s] line=%d,ret=%d\n",__FUNCTION__,__LINE__,ret);
	        if(copy_from_user(reg_rw_cmd, (UINT8 *)parg, 16)>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}			
	        if (1 == reg_rw_cmd[0]) // Register Read
	        {
	            ret = nim_reg_read(dev,reg_rw_cmd[1], &(reg_rw_cmd[3]), reg_rw_cmd[2]);
	            if(copy_to_user((UINT8 *)parg, reg_rw_cmd, 16)>0)
				{
					NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					mutex_unlock(&priv->multi_process_mutex);
					return -EFAULT;
				}
								
	        }
	        else if (2 == reg_rw_cmd[0]) // Register Write
	        {
				// NIM_PRINTF("[%s] line=%d,reg_rw_cmd[3]=0x%x,len=%d\n",__FUNCTION__,__LINE__,reg_rw_cmd[3],reg_rw_cmd[2]);
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
	case ALI_NIM_GET_LNB_OCP:
		if (priv->ext_lnb_command)
			ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_GET_OCP, 0);	
		break;
	case ALI_NIM_SET_LNB_POWER_ONOFF:
		{
			UINT8 onoff = 0;
			get_user(onoff, (unsigned char *)parg);
			if (priv->ext_lnb_command)
				ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_POWER_ONOFF, onoff);
		}
		break;
	case ALI_NIM_LNB_CURRENT_LIMIT_CONTROL:
		{
			UINT8 onoff = 0;
			get_user(onoff, (unsigned char *)parg);
			if (priv->ext_lnb_command)
				ret =  priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_CURRENT_LIMIT_CONTROL, onoff);
		}
		break;
	case ALI_NIM_SET_LOOPTHRU:
		break;
	case ALI_NIM_GET_LOOPTHRU:
        break;
	case ALI_NIM_ACM_MODE_GET_TP_INFO:
		nim_c3505_get_tp_info(dev);
		break;
	case ALI_NIM_ACM_MODE_SET_ISID:
		{
			struct nim_dvbs_isid isid;
        	if(copy_from_user(&isid, (struct nim_dvbs_isid *)parg, sizeof(struct nim_dvbs_isid))>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			nim_c3505_set_isid(dev,&isid);
		}
		break;
	case ALI_NIM_ACM_MODE_GET_ISID:
		{
			if (priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
			{
				NIM_PRINTF("[%s %d]don't allow get isid!\n",__FUNCTION__, __LINE__);
				priv->isid->get_finish_flag = 0;			
			}
			if(copy_to_user((struct nim_dvbs_isid *)parg, priv->isid, sizeof(struct nim_dvbs_isid))>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
		}
		break;
	case ALI_NIM_DRIVER_CAP_DATA:
		{
			struct nim_dvbs_data_cap data_cap;
			if(copy_from_user(&data_cap, (struct nim_dvbs_data_cap *)parg, sizeof(struct nim_dvbs_data_cap))>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			nim_c3505_adc2mem_entity(dev, (UINT8 *)data_cap.dram_base_addr, data_cap.cap_len, data_cap.cap_src);
		}
		break;

	case ALI_NIM_DRIVER_GET_SYMBOL:
		{
			struct nim_get_symbol symbol;
			if(copy_from_user(&symbol, (struct nim_get_symbol *)parg, sizeof(struct nim_get_symbol))>0)
			{
				NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}
			if ((symbol.p_cr_out_i != NULL) && (symbol.p_cr_out_q != NULL))
			{
				nim_c3505_get_symbol(dev, &symbol);
			}
		}

	case ALI_NIM_DRIVER_SET_PLSN:
	
		if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		priv->plsn.plsn_try = 0;
		priv->plsn.plsn_now = plsn_value;
        nim_c3505_set_plsn(dev);
        break;
		
	case ALI_NIM_DRIVER_GET_PLSN:
		if(copy_to_user((UINT32 *)parg, &priv->plsn.plsn_report, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		NIM_PRINTF("[%s %d]*(UINT32 *)parg=%d, priv->plsn.plsn_report=%d\n", __FUNCTION__, __LINE__, *(int *)parg, (int)priv->plsn.plsn_report);
		break;
	
	case ALI_NIM_DRIVER_PLSN_GOLD_TO_ROOT:
		if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		temp_value = nim_c3505_plsn_gold_to_root(plsn_value);
		if(copy_to_user((UINT32 *)parg, &temp_value, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		
		break;
		
	case ALI_NIM_DRIVER_PLSN_ROOT_TO_GOLD:
		if(copy_from_user(&plsn_value, (UINT32 *)parg, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		temp_value = nim_c3505_plsn_root_to_gold(plsn_value);

		if(copy_to_user((UINT32 *)parg, &temp_value, sizeof(UINT32))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		
		break;

	case ALI_NIM_DRIVER_GENERATE_TABLE:
		if(copy_from_user(&plsn_addr, (struct ali_plsn_address *)parg, sizeof(struct ali_plsn_address))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		priv->plsn.generate_table_happened_flag = 1;
		nim_c3505_start_generate(dev, &plsn_addr);
		PLSN_PRINTF("[%s %d]generate_table_happened_flag=%d\n", __FUNCTION__, __LINE__,  priv->plsn.generate_table_happened_flag);	
		break;
		
	case ALI_NIM_DRIVER_RELEASE_TABLE:
		if(priv->plsn.generate_table_happened_flag == 1)
		{
			nim_c3505_release_table(dev);
			priv->plsn.generate_table_happened_flag = 0;
			PLSN_PRINTF("[%s %d]generate_table_happened_flag=%d\n", __FUNCTION__, __LINE__,  priv->plsn.generate_table_happened_flag);	

		}
		break;

	case ALI_NIM_DRIVER_GET_PLSN_FINISH_FLAG:
		if(copy_to_user((UINT8 *)parg, &priv->plsn.plsn_finish_flag, sizeof(UINT8))>0)
		{
			NIM_PRINTF("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}
		break;

	case ALI_NIM_GET_WORK_MODE:
    {
        nim_c3505_reg_get_work_mode(dev,&work_mode);
		if(copy_to_user((UINT8 *)parg, &work_mode, sizeof(work_mode))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			mutex_unlock(&priv->multi_process_mutex);
			return -EFAULT;
		}

        break;
    }
	case ALI_NIM_DRIVER_GET_FAST_LOCK_DETECT:
		{
			nim_c3505_get_fast_lock(dev, &fast_lock_detect);
			if(copy_to_user((UINT8 *)parg, &fast_lock_detect, sizeof(fast_lock_detect))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				mutex_unlock(&priv->multi_process_mutex);
				return -EFAULT;
			}

		break;
	}
    default:
        ret = -ENOIOCTLCMD;
        break;
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
static int ali_c3505_nim_open(struct inode *inode, struct file *file)
{
    UINT8 dev_idx = 0;
	struct nim_device    * dev = NULL;
	struct nim_c3505_private * priv = NULL;

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

	priv->nim_kumsgq = ali_new_kumsgq();
	printk(KERN_INFO "###dev_idx=%d nim_kumsgq=0x%x \r\n", dev_idx, (unsigned int)priv->nim_kumsgq);
	if (!priv->nim_kumsgq)
	{
		goto out0;
    }

	priv->nim_used++;
    file->private_data = dev;
    NIM_PRINTF("[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
    return RET_SUCCESS;
out0:
	dev_idx = MINOR(inode->i_rdev);
	WARN(1,"False to new 3505 nim[%d] kumsgq!!!!!!",dev_idx);
	return -EFAULT;
}


static int ali_c3505_nim_release(struct inode *inode, struct file *file)
{
    struct nim_device 		  *dev = file->private_data;
    struct nim_c3505_private  *priv = dev->priv;
	UINT8 data = 0;

    NIM_PRINTF("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);
	
	if (priv->nim_used != 0)
	{
		priv->nim_used--;
		if(priv->nim_used)
		{
			NIM_PRINTF("[%s %d]enter,priv->nim_used=%d!\n", __FUNCTION__, __LINE__, (int)priv->nim_used);
			return RET_SUCCESS;
		}
	}

    if (priv->tuner_opened && priv->nim_tuner_close)
    {
		priv->nim_tuner_close(priv->tuner_index);	
    }
	if (priv->ext_lnb_command)
		priv->ext_lnb_command(&priv->ext_lnb_id, LNB_CMD_RELEASE_CHIP, 0);
	
    nim_c3505_set_demod_ctrl(dev, NIM_DEMOD_CTRL_0X90);
    //nim_c3505_set_acq_workmode(dev, NIM_OPTR_HW_CLOSE);

	//close ADC
	data = 0x07;
	nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);

	//data = 0x3f;
	data = 0x1f;//S2_CLK_EN bit set to 0 for C3505
	nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);

    priv->work_alive = 0;
    priv->ul_status.c3505_chanscan_stop_flag = 1;
	priv->ul_status.c3505_autoscan_stop_flag = 1;	

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

	if (dev->plsn_gen_table_workqueue)
    {
		cancel_delayed_work(&dev->delay_plsn_gen_table_work);
        flush_workqueue(dev->plsn_gen_table_workqueue);
        destroy_workqueue(dev->plsn_gen_table_workqueue);
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
    NIM_PRINTF("[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
	priv->nim_init = FALSE;

	ali_destroy_kumsgq(priv->nim_kumsgq);
	priv->nim_kumsgq = NULL;

    return RET_SUCCESS;
}


static struct file_operations ali_c3505_nim_fops =
{
    .owner						= THIS_MODULE,
    .write                      = NULL,
    .unlocked_ioctl             = ali_c3505_nim_ioctl,
    .open                       = ali_c3505_nim_open,
    .release                    = ali_c3505_nim_release,
};

static int ali_c3505_malloc_memory(struct nim_device *dev)
{
	struct nim_dvbs_channel_info *channel_info_mem = NULL;
	struct nim_dvbs_bb_header_info *bb_header_info_mem = NULL;
	struct nim_dvbs_isid *isid_mem = NULL;
	struct nim_c3505_private 	*priv = NULL;

	pr_info("[%s %d]enter\n", __FUNCTION__, __LINE__);
	if (NULL == dev)
	{
		pr_err("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	priv = dev->priv;
	
	channel_info_mem = comm_malloc(sizeof(struct nim_dvbs_channel_info));
	if (!channel_info_mem)
	{
	    pr_err("[%s %d]channel_info_mem kmalloc failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(channel_info_mem, 0, sizeof(struct nim_dvbs_channel_info));
	priv->channel_info = channel_info_mem;

	bb_header_info_mem = comm_malloc(sizeof(struct nim_dvbs_bb_header_info));
	if (!bb_header_info_mem)
	{
		comm_free(priv->channel_info);
	    pr_err("[%s %d]bb_header_info_mem kmalloc failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(bb_header_info_mem, 0, sizeof(struct nim_dvbs_bb_header_info));
	priv->bb_header_info = bb_header_info_mem;

	isid_mem = comm_malloc(sizeof(struct nim_dvbs_isid));
	if (!isid_mem)
	{
		comm_free(priv->bb_header_info);
		comm_free(priv->channel_info);
	    pr_err("[%s %d]isid_mem kmalloc failed!\n", __FUNCTION__, __LINE__);
	    return -ENOMEM;
	}
	comm_memset(isid_mem, 0, sizeof(struct nim_dvbs_isid));
	priv->isid = isid_mem;	

	//To solve malloc failure when autoscan
	priv->ul_status.adc_data_malloc_addr = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE * 2);//256K
	if(NULL == priv->ul_status.adc_data_malloc_addr)			
	{
		comm_free(priv->isid);
		comm_free(priv->bb_header_info);
		comm_free(priv->channel_info);
		pr_err("[%s %d]priv->ul_status.adc_data_malloc_addr kmalloc failed!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
	
	//To solve malloc failure when autoscan
	if(NULL == channel_spectrum)
	{
	    channel_spectrum = (INT32 *) comm_malloc(FS_MAXNUM * 4);//58K
	    if(channel_spectrum == NULL)
	    {
	    	comm_free(priv->ul_status.adc_data_malloc_addr);
	    	comm_free(priv->isid);
			comm_free(priv->bb_header_info);
			comm_free(priv->channel_info);
	        pr_err("\n channel_spectrum--> no enough memory!\n");
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
			comm_free(priv->isid);
			comm_free(priv->bb_header_info);
			comm_free(priv->channel_info);
			pr_err("\n channel_spectrum_tmp--> no enough memory!\n");			
			return ERR_NO_MEM;
	   	}
	}
	
	if(NULL == dram_base_t)
	{
		dram_base_t = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE_DMA);//64K
		if(NULL == dram_base_t)
	   	{
	   		comm_free(channel_spectrum_tmp);
	   		comm_free(channel_spectrum);
			comm_free(priv->ul_status.adc_data_malloc_addr);
			comm_free(priv->isid);
			comm_free(priv->bb_header_info);
			comm_free(priv->channel_info);
			pr_err("\n dram_base_t--> no enough memory!\n");
			return ERR_NO_MEM;
	   	}
	}
	
	pr_info("[%s %d]over\n", __FUNCTION__, __LINE__);
	return SUCCESS;
}

static int ali_c3505_free_memory(struct nim_device *dev)
{
	struct nim_c3505_private 	*priv = NULL;

	pr_info("[%s %d]enter\n", __FUNCTION__, __LINE__);
	if (NULL == dev)
	{
		pr_err("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	priv = dev->priv;	

	if (dram_base_t != NULL)
	{
		comm_free(dram_base_t);
	}

	if (channel_spectrum_tmp != NULL)
	{
		comm_free(channel_spectrum_tmp);
	}

	if (priv->ul_status.adc_data_malloc_addr != NULL)
	{
		comm_free(channel_spectrum);
	}

	if (priv->ul_status.adc_data_malloc_addr != NULL)
	{
		comm_free(priv->ul_status.adc_data_malloc_addr);
	}

	if (priv->bb_header_info != NULL)
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
	
	pr_info("[%s %d]over\n", __FUNCTION__, __LINE__);
	return SUCCESS;
}


static int ali_nim_probe(struct platform_device * pdev)
{
    INT32 ret;
	static int dev_num = 0;
    dev_t devno;	
	struct nim_device 				*ali_c3505_nim_dev = NULL;
	struct nim_c3505_private 		*ali_c3505_nim_priv = NULL;
	struct device 					*ali_c3505_dev_node = NULL;	
	
    NIM_PRINTF("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);
	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, 4, ALI_NIM_DEVICE_NAME);
	if (ret  < 0) {
		printk(KERN_ERR"unable to get major and minor for ali_nim_c3505 device\n");
		return ret;
	}

	if (g_c3505_class_create_flag)
	{
	    ali_c3505_nim_class = class_create(THIS_MODULE, "ali_c3505_nim_class");
	    if (IS_ERR(ali_c3505_nim_class))
	    {
	        printk(KERN_ERR"[%s %d]class_create error,back!\n", __FUNCTION__, __LINE__);
	        ret = PTR_ERR(ali_c3505_nim_class);
	        return ret;
	    }
	    g_c3505_class_create_flag = FALSE;
	}

	ali_c3505_nim_dev = kmalloc(sizeof(struct nim_device), GFP_KERNEL);
	if (!ali_c3505_nim_dev)
	{
	    printk(KERN_ERR"ali_c3505_nim_dev kmalloc failed!\n");
	    return -ENOMEM;
	}
	comm_memset(ali_c3505_nim_dev, 0, sizeof(struct nim_device));
	
	ali_c3505_nim_priv = kmalloc(sizeof(struct nim_c3505_private), GFP_KERNEL);
	if (!ali_c3505_nim_priv)
	{
	    printk(KERN_ERR"ali_c3505_nim_priv kmalloc failed!\n");
	    return -ENOMEM;
	}
	comm_memset(ali_c3505_nim_priv, 0, sizeof(struct nim_c3505_private));

	cdev_init(&ali_c3505_nim_dev->cdev, &ali_c3505_nim_fops);
	ali_c3505_nim_dev->cdev.owner = THIS_MODULE;
	ali_c3505_nim_dev->cdev.ops = &ali_c3505_nim_fops;
	ali_c3505_nim_dev->priv = (void *)ali_c3505_nim_priv;
	
	ali_c3505_nim_priv->dev_idx = dev_num;
	ali_c3505_nim_priv->nim_init = FALSE;
	ali_c3505_nim_priv->nim_used = 0;
	
	ret = ali_c3505_malloc_memory(ali_c3505_nim_dev);
	if (ret != SUCCESS)
	{
		kfree(ali_c3505_nim_priv);
		kfree(ali_c3505_nim_dev);
		pr_err("[%s %d]malloc c3501h memory failed!\n", __FUNCTION__, __LINE__);
		return ret;
	}	

	ret = cdev_add(&ali_c3505_nim_dev->cdev, devno + dev_num, 1);
	if(ret)	
	{	     
		ali_c3505_free_memory(ali_c3505_nim_dev);
	    kfree(ali_c3505_nim_priv);
		kfree(ali_c3505_nim_dev);
		pr_err("[%s %d]cdev_add failed, err: %d.\n", __FUNCTION__, __LINE__, (int)ret);	 
	}

	mutex_init(&ali_c3505_nim_priv->i2c_mutex);
	mutex_init(&ali_c3505_nim_priv->multi_process_mutex);
	mutex_init(&ali_c3505_nim_priv->plsn_mutex);
	ali_c3505_nim_priv->flag_lock.flagid_rwlk = __RW_LOCK_UNLOCKED(ali_c3505_nim_priv->flagid_rwlk);
	
	ali_c3505_dev_node = device_create(ali_c3505_nim_class, NULL, MKDEV(MAJOR(devno), dev_num),
	                            ali_c3505_nim_dev, "ali_c3505_nim%d", dev_num);
	if (IS_ERR(ali_c3505_dev_node))
	{
	    pr_err("[%s %d]device_create() failed!\n", __FUNCTION__, __LINE__);
	    ret = PTR_ERR(ali_c3505_dev_node);
	    cdev_del(&ali_c3505_nim_dev->cdev);
	    mutex_destroy(&ali_c3505_nim_priv->i2c_mutex);
		mutex_destroy(&ali_c3505_nim_priv->multi_process_mutex);
		mutex_destroy(&ali_c3505_nim_priv->plsn_mutex);
		ali_c3505_free_memory(ali_c3505_nim_dev);
	    kfree(ali_c3505_nim_priv);
		kfree(ali_c3505_nim_dev);
	}
	ali_c3505_nim_dev->ali_nim_dev_node = ali_c3505_dev_node;
	
	pdev->dev.platform_data = ali_c3505_nim_dev;

	pr_info("[%s %d]leave, id = %d\n", __FUNCTION__, __LINE__, dev_num);
	dev_num++;
	return SUCCESS;
}

static int ali_nim_suspend(struct platform_device *pdev,pm_message_t state)
{
	UINT16 c3505_reg_num = 0x0;
	UINT8 reg_value = 0x0;
	UINT8 data = 0;
	struct nim_device *dev = NULL;
	struct nim_c3505_private *priv = NULL;

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
	for(c3505_reg_num = 0;c3505_reg_num < 0x1c0; c3505_reg_num++)
	{
		if(nim_reg_read(dev, c3505_reg_num, &reg_value, 1) < 0)
		{
		#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
			printk(KERN_EMERG "Function ali_nim_suspend read error");
		#endif
		}
		//printk("[suspend] reg :0x%x = 0x%x\n",c3505_reg_num,reg_value);
		demod_str_reg_buffer[c3505_reg_num]=reg_value;
		reg_value = 0x0;
	}
	//sleep tuner
	if(pdev->id ==1)
	{
		return SUCCESS;
	}
	
	if (priv->nim_tuner_command !=  NULL)
	{	
		priv->nim_tuner_command(priv->tuner_index, NIM_TURNER_SET_STANDBY, 0);
	}
	
	//sleep tuner
	NIM_PRINTF("[%s %d]priv->tuner_opened=%d!\n", __FUNCTION__, __LINE__, (int)priv->tuner_opened);
    if (priv->tuner_opened && priv->nim_tuner_close)
    {
		priv->nim_tuner_close(priv->tuner_index);	
    }
    nim_c3505_set_demod_ctrl(dev, NIM_DEMOD_CTRL_0X90);
    //nim_c3505_set_acq_workmode(dev, NIM_OPTR_HW_CLOSE);

	//close ADC
	data = 0x07;
	nim_reg_write(dev, RA0_RXADC_REG + 0x02, &data, 1);

	//data = 0x3f;
	data = 0x1f;//S2_CLK_EN bit set to 0 for C3505
	nim_reg_write(dev, R5B_ACQ_WORK_MODE, &data, 1);

    priv->work_alive = 0;
    priv->ul_status.c3505_chanscan_stop_flag = 1;
	priv->ul_status.c3505_autoscan_stop_flag = 1;	
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

	if (dev->plsn_gen_table_workqueue)
    {
        flush_workqueue(dev->plsn_gen_table_workqueue);
        destroy_workqueue(dev->plsn_gen_table_workqueue);
        dev->plsn_gen_table_workqueue = NULL;
    }

	if (dev->plsn_search_workqueue)
    {
        flush_workqueue(dev->plsn_search_workqueue);
        destroy_workqueue(dev->plsn_search_workqueue);
        dev->plsn_search_workqueue = NULL;
    }

#ifdef CHANNEL_CHANGE_ASYNC
    nim_flag_del(&priv->flag_lock);
#endif

	ali_destroy_kumsgq(priv->nim_kumsgq);
	priv->nim_kumsgq = NULL;

	return SUCCESS;
}
static int ali_nim_resume(struct platform_device *pdev)
{
	//ali_suspend_output_string("Function ali_nim_resume....===");
	//return SUCCESS;
	UINT16 c3505_reg_num = 0x0;
	UINT8 reg_value = 0x0;
	struct nim_device *dev = NULL;
	struct nim_c3505_private *priv = NULL;
	
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "Function ali_nim_resume....===");
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
		printk(KERN_EMERG "c3505 ali_nim_suspend have not init");
	#endif
		return SUCCESS;
	}

	//wakeup demod
	for(c3505_reg_num=0;c3505_reg_num<0x1c0;c3505_reg_num++)
	{
		
		reg_value = demod_str_reg_buffer[c3505_reg_num];
		if(nim_reg_write(dev, c3505_reg_num, &reg_value, 1) < 0)
		{
		#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
			printk(KERN_EMERG "Function ali_nim_resume write error");
		#endif
		}
		reg_value = 0x0;
	}

	if(pdev->id ==1)
	{
		return SUCCESS;
	}
	
	//wakeup tuner
	if (priv->nim_tuner_command != NULL)
	{
		priv->nim_tuner_command(priv->tuner_index, NIM_TUNER_WAKE_UP, 0);
	}
	else
	{
		NIM_PRINTF("[%s]line=%d,function: nim_tuner_control don't find\n", __FUNCTION__, __LINE__);
		return -EFAULT;
	}

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "nim tuner wakeup...");
#endif
	
	return SUCCESS;
}
static int ali_nim_remove(struct platform_device *pdev)
{
	struct nim_device *dev = NULL;
	struct nim_c3505_private *priv = NULL;

	pr_info("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

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

	ali_c3505_free_memory(dev);
	    
    mutex_destroy(&priv->i2c_mutex);
	mutex_destroy(&priv->multi_process_mutex);
	mutex_destroy(&priv->plsn_mutex);
	
    if (dev->autoscan_work_queue)
    {
        destroy_workqueue(dev->autoscan_work_queue);
    }

	if (dev->workqueue)
    {
    	destroy_workqueue(dev->workqueue);
	}

	if (dev->plsn_gen_table_workqueue)
	{
        destroy_workqueue(dev->plsn_gen_table_workqueue);
	}

	if (dev->plsn_search_workqueue)
	{
        destroy_workqueue(dev->plsn_search_workqueue);
	}
	if (dev->channel_change_workqueue)
	{
        destroy_workqueue(dev->channel_change_workqueue);
	}
    kfree(priv);

    cdev_del(&dev->cdev);
	
	if(ali_c3505_nim_class != NULL)
	{
		class_destroy(ali_c3505_nim_class);
	}

	if (dev->ali_nim_dev_node != NULL)
	{
		device_del(dev->ali_nim_dev_node);
	} 

    NIM_PRINTF("[%s]line=%d,end!\n", __FUNCTION__, __LINE__);

	return 0;
}

static	const	struct	of_device_id	ali_c3505_nim_match[]	= {
				{	.compatible	= "alitech, c3505_nim",	},
				{},
};

MODULE_DEVICE_TABLE(of, ali_c3505_nim_match);

static struct platform_driver nim_platform_driver = {
	.probe   = ali_nim_probe, 
	.remove   = ali_nim_remove,
	.suspend  = ali_nim_suspend,
	.resume   = ali_nim_resume,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = ALI_NIM_DEVICE_NAME,
			.of_match_table	= ali_c3505_nim_match,
	},
};

module_platform_driver(nim_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dennis.Dai");
MODULE_DESCRIPTION("Ali C3505 Demod driver");
