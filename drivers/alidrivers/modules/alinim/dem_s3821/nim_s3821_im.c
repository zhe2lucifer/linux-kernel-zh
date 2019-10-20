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

/*****************************************************************************
*    File:  nim_s3821_im.c
*
*    Description:  s3821 im layer
*    History:
*           Date            Athor        Version          Reason
*        ============    =============    =========    =================
*    1.    6.14.2013        Joey.Gao         Ver 1.0       Create file.
*
*****************************************************************************/

#include "nim_s3821.h"


#define MAGIC_CONST_1          1
#define MAGIC_CONST_3          3
#define MAGIC_CONST_4          4
#define MAGIC_CONST_5          5
#define MAGIC_CONST_70         70
#define MAGIC_CONST_300        300
#define MAGIC_CONST_40000      40000
#define MAGIC_CONST_900000     900000



#define FAST_TIMECST_AGC    1
#define SLOW_TIMECST_AGC    0
#define _1ST_I2C_CMD        0
#define _2ND_I2C_CMD        1


#define tuner_chip_sanyo        9
#define TUNER_CHIP_CD1616LF_GIH    8
#define tuner_chip_nxp        7
#define tuner_chip_maxlinear    6
#define tuner_chip_microtune    5
#define tuner_chip_quantek    4
#define tuner_chip_rfmagic  3
#define tuner_chip_alps        2    //60120-01Angus
#define tuner_chip_philips    1
#define tuner_chip_infineon    0



INT32 nim_s3821_tuner_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param)
{
    struct nim_s3821_private *dev_priv = NULL;
    INT32 ret_val = SUCCESS;

    dev_priv = (struct nim_s3821_private *)dev->priv;
	
	S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);

    //joey, 20111031. for adc2dma function.
    if(cmd & NIM_TUNER_COMMAND)
    {
        if(dev_priv->tuner_control.nim_tuner_command != NULL)
        {
            ret_val = dev_priv->tuner_control.nim_tuner_command(dev_priv->tuner_id, cmd, param);
        }
        else
        {
            S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d dev_priv->tuner_control.nim_tuner_command is NULL\n",__FUNCTION__,__LINE__);
            ret_val = ERR_FAILUE;
        }
      
    }

    return ret_val;
	
}


INT32 nim_s3821_hw_init(struct nim_device *dev)
{
	struct nim_s3821_private *priv_mem = NULL;

    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter!\n", __FUNCTION__, __LINE__);
	
	if(NULL == dev)
	{
		S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,error,back!\n", __FUNCTION__, __LINE__); 
		return ERR_FAILURE;
	}

	priv_mem = (struct nim_s3821_private *)dev->priv;
    if (priv_mem->tuner_control.nim_tuner_init != NULL)
    {
        if (priv_mem->tuner_control.nim_tuner_init(&(priv_mem->tuner_id), 
               &priv_mem->tuner_control.tuner_config) != SUCCESS)
        {
            S3821_PRINTF(NIM_LOG_DBG,"Error: Init Tuner Failure!\n");
            return ERR_NO_DEV;
        }
    }

	S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d,end\n", __FUNCTION__, __LINE__);
	return SUCCESS;
	
}


static INT32 nim_s3821_getinfo(struct nim_device *dev, UINT8 *guard_interval, UINT8 *fft_mode,
                                 UINT8 *modulation, UINT8 *spectrum)
{
    UINT8 data = 0;

    nim_s3821_read(dev, 0x20, &data, 1);
    switch (data & 0x03)
    {
    case 0:
        *guard_interval = GUARD_1_32;
        break;
    case 1:
        *guard_interval = GUARD_1_16;
        break;
    case 2:
        *guard_interval = GUARD_1_8;
        break;
    case 3:
        *guard_interval = GUARD_1_4;
        break;
    default:
        *guard_interval = 0xff; /* error */
        break;
    }

    nim_s3821_read(dev, 0x1e, &data, 1);
    switch ((data & 0x08) >> 3)
    {
    case 0:
        *spectrum = IQ_NORMAL;
        break;
    case 1:
        *spectrum = IQ_SWAP;
        break;
    default:
        *spectrum = IQ_NORMAL; /* error */
        break;
    }

    switch ((data & 0x30) >> 4)
    {
    case 0:
        *fft_mode = MODE_2K;
        break;
    case 1:
        *fft_mode = MODE_8K;
        break;
    case 2:
        *fft_mode = MODE_4K;
        break;
    default:
        *fft_mode = 0xff; /* error */
        break;
    }

    //joey, 20111018. only for DVB-T, tps report.
    //joey, 20120611. for ISDB-T and DVB-T tmcc/tps report.
    //judge isdb-t or dvb-t firstly.
    nim_s3821_read(dev, 0x12, &data, 1);
    if (0x00 == (data & 0x80)) // isdb-t mode, read tmcc register.
    {
        nim_s3821_read(dev, 0x84, &data, 1); // only dealing layer A.
        switch ((data & 0x38) >> 3)
        {
        case 0:
            * modulation = TPS_CONST_DQPSK;//0x02; // use for DQPSK, not define yet.
            break;
        case 1:
            * modulation = TPS_CONST_QPSK;
            break;
        case 2:
            * modulation = TPS_CONST_16QAM;
            break;
        case 3:
            * modulation = TPS_CONST_64QAM;
            break;
        default:
            * modulation = 0xff; /* error */
            break;

        }

    }
    else // dvb-t mode.
    {
        nim_s3821_read(dev, 0x74, &data, 1);
        switch ((data & 0x0c) >> 2)
        {
        case 0:
            * modulation = TPS_CONST_QPSK;
            break;
        case 1:
            * modulation = TPS_CONST_16QAM;
            break;
        case 2:
            * modulation = TPS_CONST_64QAM;
            break;
        default:
            * modulation = 0xff; /* error */
            break;
        }
    }

    //S3821_PRINTF(NIM_LOG_DBG,"fft=0x%x,GI=0x%x,mod=0x%x, spectrum=0x%x\n",*fft_mode,*guard_interval,*modulation,*spectrum);
    //S3821_PRINTF(NIM_LOG_DBG,"fft=0x%x,GI=0x%x,spectrum=0x%x\n",*fft_mode,*guard_interval,*spectrum);
    return SUCCESS;

}


//end of declaration.

static INT32 nim_s3821_read_soc_mode(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len)
{
    INT32 i = 0;
    struct nim_s3821_private *dev_priv = (struct nim_s3821_private *)(dev->priv);

    //joey, 20120613, for register access protection when HW operation.

    //osal_mutex_lock(dev_priv->nim_s3821_i2c_mutex, OSAL_WAIT_FOREVER_TIME);
    NIM_MUTEX_ENTER(dev_priv);
	
    for (i = 0; i < len; i++)
    {
        *(data + i) = NIM_S3821_GET_BYTE(( dev_priv->base_addr + reg_add + i));
    }

    //joey, 20120613, for register access protection when HW operation.
    //osal_mutex_unlock(dev_priv->nim_s3821_i2c_mutex);
    NIM_MUTEX_LEAVE(dev_priv);
	
    return SUCCESS;

}



static INT32 nim_s3821_write_soc_mode(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len)
{
    INT32 i = 0;
    struct nim_s3821_private *dev_priv = (struct nim_s3821_private *)(dev->priv);

    //joey, 20120613, for register access protection when HW operation.
    //osal_mutex_lock(dev_priv->nim_s3821_i2c_mutex, OSAL_WAIT_FOREVER_TIME);
    NIM_MUTEX_ENTER(dev_priv);
	
    for (i = 0; i < len; i++)
    {

        NIM_S3821_SET_BYTE(( dev_priv->base_addr  + reg_add + i),  *(data + i));
        //osal_delay(1);
        //tmp_var |= NIM_S3821_GET_BYTE((dev->base_addr + 0x3b));
        //osal_delay(1);
    }

    //joey, 20120613, for register access protection when HW operation.
    //osal_mutex_unlock(dev_priv->nim_s3821_i2c_mutex);
    NIM_MUTEX_LEAVE(dev_priv);
	
    //joey, 20130703, for test_mux selection out purpose.
    if (MAGIC_CONST_3 == reg_add)
    {
        nim_s3821_proc_test_mux(*data);
    }

    return SUCCESS;
}


INT32 nim_s3821_read(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len)
{
    INT32 err = 0;
    struct nim_s3821_private *priv = (struct nim_s3821_private *)(dev->priv);

    if ( priv->tuner_control.work_mode == NIM_COFDM_SOC_MODE )
    {
        err = nim_s3821_read_soc_mode(dev, reg_add, data, len);
    }
    else
    {
        err = nim_s3821_read_external_cofdm_mode(dev, reg_add, data, len);
    }
    return err;
}



INT32 nim_s3821_write(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len)
{

    INT32 err = 0;
    struct nim_s3821_private *priv = (struct nim_s3821_private *)(dev->priv);

    if ( priv->tuner_control.work_mode == NIM_COFDM_SOC_MODE )
    {
        err = nim_s3821_write_soc_mode(dev, reg_add, data, len);
    }
    else
    {
        err = nim_s3821_write_external_cofdm_mode(dev, reg_add, data, len);
    }
    return err;

}



/*****************************************************************************

* INT32 nim_s3821_dvbt_isdbt_proc(struct nim_device *dev)

* Description: m3101 open

* Arguments:

*  Parameter1: struct nim_device *dev

* Return Value: INT32

*****************************************************************************/



void nim_s3821_dvbt_isdbt_proc(struct nim_device *dev,PNIM_S3821_PRIVATE priv,UINT8 *unlock_cnt)
{
    UINT32 cur_time = 0;
    UINT8 ber_vld = 0;
    UINT32 m_vbber = 0;
    UINT32 m_per  = 0;
    UINT8 iflock = 0;
	UINT8 cur_mode = 0; 


        nim_s3821_dvbt_isdbt_monitor_ber(dev, &ber_vld, &m_vbber, &m_per);
        nim_s3821_dvbt_isdbt_monitor_cnr(dev, NULL);
        nim_s3821_read(dev, 0x1d, &iflock, 1);
        //S3821_PRINTF(NIM_LOG_DBG,"CR1d=%x-- FSM state\n",iflock);
        if (!(iflock & (1 << 6)))
        {
#if (CHIP_VERIFY_FUNC == SYS_FUNC_ON) //for S3821 chip test support.
            *unlock_cnt += 1;
            //joey, 20111019. for FSM print.
            //if (unlock_cnt > 30)
            if (*unlock_cnt > MAGIC_CONST_4)
            {
                libc_printf("BER= 0,PER= 0,Unlock, FSM = 0x%2x;\n", iflock);
                //sprintf(osd_buffer, "Unlock!\n");
                //osd_printf(osd_buffer);
                *unlock_cnt = 0;
            }
#else
            if (priv->log_en)
            {
                *unlock_cnt += 1;

                //joey, 20111019. for FSM print.
                //if (unlock_cnt > 30)
                if (*unlock_cnt > MAGIC_CONST_4)
                {
                    S3821_PRINTF(NIM_LOG_DBG,"BER= 0,PER= 0,Unlock, FSM = 0x%2x;\n", iflock);
                    //sprintf(osd_buffer, "Unlock!\n");
                    //osd_printf(osd_buffer);    // Note: this and above code indicate print BerPer On TV screen
                    *unlock_cnt = 0;
                }
            }
#endif

        }
        else // (iflock & S3821_COFDM_STATUS_LOCK) == TRUE
        {
            if (ber_vld == 1)
            {
#if (CHIP_VERIFY_FUNC == SYS_FUNC_ON) //for S3821 chip test support.
                libc_printf("BER=%6d,PER=%d,Locked;\n", m_vbber, m_per);
                //sprintf(osd_buffer, "B=%6d,  P=%d\n", m_vbber,m_per);
                //osd_printf(osd_buffer);// Note: this and above code indicate print BerPer On TV screen
#else
                if (priv->log_en)
                {
                    S3821_PRINTF(NIM_LOG_DBG,"BER=%6d,PER=%d,Locked;\n", m_vbber, m_per);
                }
#endif
            }

        }

    //joey, 20130704, for TF illegal interrupt service.
    //joey, 20140328, for found a potential case when DVBT2/T combo, not dealing isdbt case currently.
    nim_s3821_get_cur_mode(dev, &cur_mode);

		if (MODE_ISDBT == cur_mode)
		{
			nim_s3821_tf_int_proc(dev);		
		}
}



/*****************************************************************************
* INT32 nim_s3821_disable(struct nim_device *dev)
* Description: m3101 disable
* Arguments:
*  Parameter1: struct nim_device *dev
* Return Value: INT32
*****************************************************************************/

INT32 nim_s3821_disable(struct nim_device *dev)

{

    UINT8 data = 0;

    nim_s3821_read(dev, 0x00, &data, 1);

    data |= 0x80;

    nim_s3821_write(dev, 0x00, &data, 1);

    return SUCCESS;

}



/*****************************************************************************
* INT32 nim_s3811_channel_search(struct nim_device *dev, UINT32 freq);
* Description: m3101 channel search operation
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT32 freq : Frequency*
* Return Value: INT32
*****************************************************************************/
#if 0
static INT32 nim_s3821_channel_search(struct nim_device *dev, UINT32 freq, UINT32 bandwidth,
           UINT8 guard_interval, UINT8 fft_mode, UINT8 modulation, UINT8 fec, UINT8 usage_type,
       UINT8 inverse, UINT16 freq_offset, UINT8 priority)
{
    INT8 i = 0;
    INT32 chsearch = ERR_FAILUE;
    UINT32 tmp_freq = 0;

    S3821_PRINTF(NIM_LOG_DBG,"%s: freq %d, bandwidth %d,guard_interval %d, fft_mode %d,modulation %d,fec %d,\
 usage_type %d, inverse %d\n ", \
                    __FUNCTION__, freq, bandwidth, guard_interval, fft_mode, modulation, fec, usage_type, inverse);
    {
        tmp_freq = freq ; //Khz. for tuner - step meet. 2*step_size.
        //chsearch=nim_s3821_internal_channel_change(dev,tmp_freq,bandwidth,guard_interval,
    //fft_mode,modulation,fec,usage_type,inverse,priority);
        return chsearch;
    }

    return chsearch;
}
#endif

/*****************************************************************************
* INT32 nim_s3811_channel_change(struct nim_device *dev, UINT32 freq, UINT32 bandwidth,
    UINT8 guard_interval, UINT8 fft_mode, UINT8 modulation, UINT8 fec, UINT8 usage_type,UINT8 inverse);
* Description: m3101 channel change operation
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT32 freq : Frequency
*  Parameter3: UINT32 bandwidth
*  Parameter4: UINT8  guard_interval
*  Parameter5: UINT8  fft_mode
*  Parameter6: UINT8  modulation
*  Parameter7: UINT8  fec
*
* Return Value: INT32
*****************************************************************************/
#if 0
static INT32 nim_s3821_channel_change(struct nim_device *dev, UINT32 freq, UINT32 bandwidth,
                                      UINT8 guard_interval, UINT8 fft_mode, UINT8 modulation,
                      UINT8 fec, UINT8 usage_type, UINT8 inverse, UINT8 priority)
{
    INT32 chsearch = 0;

    S3821_PRINTF(NIM_LOG_DBG,"%s: freq %d, bandwidth %d,guard_interval %d, fft_mode %d,modulation %d,fec %d,\
                            usage_type %d, inverse %d\n ", \
                    __FUNCTION__, freq, bandwidth, guard_interval, fft_mode, modulation, fec, usage_type, inverse);

    //chsearch=nim_s3821_internal_channel_change(dev,freq,bandwidth,guard_interval,fft_mode,modulation,
    //                   fec,usage_type,inverse,priority);
    return chsearch;
}
#endif

/*****************************************************************************
* INT32 nim_s3821_get_curr_dvbt2_chninfo(struct nim_device *dev, ...);
* Description: s3821 dvbt2 channel change operation
* Return Value: INT32
*****************************************************************************/
static void nim_s3821_get_curr_dvbt2_chninfo(struct nim_device *dev)
{
    UINT8 dvbt2_guard_interval = 0;
    UINT8 dvbt2_fft_mode = 0;
    UINT8 dvbt2_modulation = 0;
    UINT8 dvbt2_code_rate = 0;
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);

    nim_s3821_dvbt2_getinfo(dev, &dvbt2_guard_interval, &dvbt2_fft_mode, &dvbt2_modulation, &dvbt2_code_rate);

    dev_priv->s3821_cur_channel_info.guard = dvbt2_guard_interval;
    dev_priv->s3821_cur_channel_info.mode = dvbt2_fft_mode;

    dev_priv->s3821_cur_channel_info.modulation = dvbt2_modulation;
    dev_priv->s3821_cur_channel_info.fecrates = dvbt2_code_rate;

    dev_priv->s3821_cur_channel_info.spectrum = 0;
    dev_priv->s3821_cur_channel_info.hierarchy = 0;
    dev_priv->s3821_cur_channel_info.freq_offset = 0;


}

static INT32 nim_s3821_dvbt2_lock_proc(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{
    UINT8 lock = 0;
    UINT8 try_time = 0;
    UINT8 ret_flag = 0;
    UINT8 data[6] = {0};
    UINT8 wait_lock_time = 0;
    UINT8 usage_type = 0;
    UINT8 ch_plp_idx = 0;
    UINT8 plp_id = 0;
    UINT8 t2_system_id = 0;
    UINT8 tot_plp_num = 0;
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;

    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
   nim_s3821_read(dev, 0x00, data, 1);
    data[0] |= 0x80;                // reset the demod
    nim_s3821_write(dev, 0x00, data, 1);

    //joey, 20120928, for T2 5/6/7/8 and 1.7M( round to 2M) support.
    //bandwidth setting.
    nim_s3821_read(dev, 0x06, data, 1);
    data[0] &= 0xf8;        // clear the bit[2:0], chan_bw

    usage_type = change_para->usage_type;

    switch(change_para->bandwidth)
    {
    
    //joey, 20141223, optimize 1.7M support function.
	case (1): // 1 also denote for 1.7M. as compatible.
    case (2): // denote for 1.7M.
        data[0] |= 0x04;
        break;
    case (5): // denote for 5M.
        data[0] |= 0x00;
        break;
    case BW_6M:
        data[0] |= 0x01;
        break;
    case BW_7M:
        data[0] |= 0x02;
        break;
    case BW_8M:
        data[0] |= 0x03;
	    break;
    case BW_10M:
		data[0] |= 0x05;	
        break;
    default:
        data[0] |= 0x03;
        if(dev_priv->log_en)
        {
             S3821_PRINTF(NIM_LOG_DBG,"Error! UnKnown bandwidth mode\n");
        }
        break;

    }
    nim_s3821_write(dev, 0x06, data, 1);

    //set plp index.
    if (USAGE_TYPE_CHANCHG == usage_type)
    {
        ch_plp_idx = change_para->plp_index;
    }
    else if (USAGE_TYPE_NEXT_PIPE_SCAN == usage_type) // maybe no-signal during scan, not change the plp.
    {
        ch_plp_idx = change_para->plp_index;
    }
    else if ((USAGE_TYPE_AUTOSCAN == usage_type) \
             || (USAGE_TYPE_CHANSCAN == usage_type) \
             || (USAGE_TYPE_AERIALTUNE == usage_type))
    {
        ch_plp_idx = 0;
        // plp_index is set back to 0.
        change_para->plp_index = 0; // update the plp index.
    }
    //nim_s3821_write(dev, 0x66, &ch_plp_idx, 1);
    nim_s3821_check_rev_id(dev, data);
    if (0 == data[0]) //S3821.
    {
    		nim_s3821_write(dev, 0x66, &ch_plp_idx, 1);
    }
    else
    {
    		nim_s3821_write(dev, 0x11b, &ch_plp_idx, 1);
    }

    
    nim_s3821_read(dev, 0x00, data, 1);
    data[0] |= 0x40;                // reset the demod
    nim_s3821_write(dev, 0x00, data, 1);

    // wait for demod look, loop+interval check.
    //joey, 20130708, for DVBT2 signal, wait 250ms for blank channel check.
    //joey, 20131015, for DVBT2 signal, for blank channel judging, change from 250ms to 350ms.
    wait_lock_time = 35;
    try_time = 0;
    ret_flag = SCAN_UNLOCK;

    while(1)
    {
        comm_sleep(10);
        try_time++;

        //nim_s3821_get_lock(dev, &lock);
        nim_s3821_read(dev, 0x67, data, 1);
        if ((data[0] & 0x0f) > 0x03)
        {
            ret_flag = LOCK_OK;
		    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 P1 Lock OK, P1 use %d!!!!!!!!!!!!!!!!!!\n", try_time);
            break;
        }

        if(try_time > wait_lock_time)
        {
            S3821_PRINTF(NIM_LOG_DBG,"DVBT2 P1 Unlock!!!!!!!!!!!!!!!!!!\n");
            return ERR_FAILURE;
        }

    }

    //joey, 20130708, for DVBT2 signal, wait one more 250ms for final lock wait.
    //joey, 20140507, for DVBT2 signal, wait one more 1000ms for final lock wait, according to DCN 20140506_lock time.
    wait_lock_time = 100;
    try_time = 0;
    ret_flag = SCAN_UNLOCK;
    while(1)
    {

        comm_sleep(10);
        try_time++;

        nim_s3821_get_dvbt2_lock(dev, &lock);
        if ( MAGIC_CONST_1 == lock)
        {
            ret_flag = LOCK_OK;
		    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 lock OK, final use %d!!!!!!!!!!!!!!!!!!\n", try_time);
           break;
        }

        if(try_time > wait_lock_time) // joey, modify to 350+1000 = 1350ms, 20140507.
        {
            S3821_PRINTF(NIM_LOG_DBG,"DVBT2 unlock!!!!!!!!!!!!!!!!!!\n");
            return ERR_FAILURE;
        }

    }
    //update the t2 information, if need feedback the t2 information.
    {
        // get tot_plp_num.
        nim_s3821_read(dev, 0x170, &tot_plp_num, 1);
        change_para->plp_num = tot_plp_num;

        //get plp_id.
        nim_s3821_read(dev, 0x161, &plp_id, 1);
        change_para->plp_id = plp_id;

        //get t2_system_id. // this will be ready soon.
        t2_system_id = 0;
        change_para->t2_system_id = t2_system_id;
    }
    //joey, as here, to update the GI, FFT_mode, Modulation parameters.
    {
       nim_s3821_get_curr_dvbt2_chninfo(dev);
    }
    //joey, 20120810, for not config demod.
    return SUCCESS;


}

static INT32 nim_s3821_dvbt2_config_tuner(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para)
{
    UINT32 freq = 0;
    UINT32 bandwidth = 0;
    UINT8 lock = 0;
    UINT8 tuner_retry = 0;
    struct nim_s3821_private *dev_priv = NULL;
    UINT32 tuner_id = 0;
    UINT8 data[6] = {0};
    INT32 ret=RET_CONTINUE;

    dev_priv = (struct nim_s3821_private *)dev->priv;

    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);

    tuner_id = dev_priv->tuner_id;

  // freq & bandwidth use.
    freq = change_para->freq;
    bandwidth = change_para->bandwidth;

    //step 1: config tuner.
    do
    {
        //joey, 20130703, for dvbt/t2 combo solution.
        if (DVBT2_COMBO == dev_priv->cofdm_type)
        {
            if ((change_para->freq == dev_priv->s3821_cur_channel_info.frequency) &&
                        (change_para->bandwidth == dev_priv->s3821_cur_channel_info.channel_bw))
            {
                if(dev_priv->tuner_control.nim_tuner_status(tuner_id, &lock) == ERR_FAILUE)
                {
                    //if i2c read failure, no lock state can be report.
                    lock = 0;
                }

                if (1 == lock)
                {
                    break;
                }
            }
        }

        tuner_retry++;
        if ((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_maxlinear)
        {
            if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, FAST_TIMECST_AGC,
                  data, _1ST_I2C_CMD) == ERR_FAILUE)
            {
                S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
            }
        }
        else
        {
            if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, FAST_TIMECST_AGC,
                 data, _1ST_I2C_CMD) == ERR_FAILUE)
            {
                S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
            }
            if((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_infineon)
            {
                comm_sleep(10);
                if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, FAST_TIMECST_AGC,
                data, _2ND_I2C_CMD) == ERR_FAILUE)
                {
                    S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
                }
            }

            //attention: tuner ATC constant time should always at normal state after setting, not fast state
            // whatever the system in search channel or play channel!
            dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, SLOW_TIMECST_AGC, data, _1ST_I2C_CMD);

            if((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_infineon)
            {
                comm_sleep(10);
                S3821_PRINTF(NIM_LOG_DBG,"SLOW_TIMECST_AGC_2nd_i2c_cmd\n");
                dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, SLOW_TIMECST_AGC,
                data, _2ND_I2C_CMD);

            }

        }

        if(dev_priv->tuner_control.nim_tuner_status(tuner_id, &lock) == ERR_FAILUE)
        {
           //if i2c read failure, no lock state can be report.
            lock = 0;
            S3821_PRINTF(NIM_LOG_DBG,"ERROR! Tuner Rread Status Fail\n");
        }

        //S3821_PRINTF(NIM_LOG_DBG,"Tuner Lock Times=0x%d,Status=0x%d\n",tuner_retry,lock);
        if(tuner_retry > MAGIC_CONST_5)
        {
            return ERR_FAILUE;
        }

    }
    while(0 == lock);

    dev_priv->s3821_cur_channel_info.frequency = freq;
    dev_priv->s3821_cur_channel_info.channel_bw = bandwidth;


    return ret;

}

static INT32 nim_s3821_verify_t2_signal(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;

    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
    //joey, 20130703, for dvbt/t2 combo solution.
    if (DVBT2_COMBO == dev_priv->cofdm_type)
    {
        if ((USAGE_TYPE_CHANCHG == change_para->usage_type) && (0 == change_para->t2_signal)) //dvbt channel change.
        {
            return ERR_FAILUE;
        }
        else
        {
            //check if need switch work mode, or already is.
            if (DVBT2_TYPE != dev_priv->cur_type)
            {
                UINT8 cur_mode = 0;

                //set demod to DVB-T2 mode.
                cur_mode = MODE_DVBT2;
                nim_s3821_set_cur_mode(dev, &cur_mode);
                dev_priv->cur_type = DVBT2_TYPE;

            }
        }
    }

    //joey, 20130705, when set mode to DVBT, can set the t2_signal indicator as DVBT2 here.
    if ((DVBT2_TYPE == dev_priv->cofdm_type) || (DVBT2_COMBO == dev_priv->cofdm_type))
    {
        if ((USAGE_TYPE_AUTOSCAN == change_para->usage_type) \
                || (USAGE_TYPE_CHANSCAN == change_para->usage_type) \
                || (USAGE_TYPE_AERIALTUNE == change_para->usage_type))
        {
            change_para->t2_signal = 1; // now is dvbt2, should be dvbt2.
        }
    }

    return RET_CONTINUE;
}

static void nim_s3821_update_dvbt2_info(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{
    UINT8 plp_id = 0;
    UINT8 t2_system_id = 0;
    UINT8 tot_plp_num = 0;

    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
	
    // get tot_plp_num.
    nim_s3821_read(dev, 0x170, &tot_plp_num, 1);
    change_para->plp_num = tot_plp_num;

    //get plp_id.
    nim_s3821_read(dev, 0x161, &plp_id, 1);
    change_para->plp_id = plp_id;

    //get t2_system_id. // this will be ready soon.
    t2_system_id = 0;
    change_para->t2_system_id = t2_system_id;

}

INT32 nim_s3821_dvbt2_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para)
{
    UINT8 wait_lock_time = 0;
    UINT8 lock = 0;
    UINT8 ret_flag = 0;
    UINT8 try_time = 0;
    UINT8 usage_type = 0;
    UINT8 ch_plp_idx = 0;
    UINT8 cur_plp_idx = 0;
    UINT8 dat_plp_num = 0;

    UINT8 data[6] = {0};
    INT32 ret=0;
    struct nim_s3821_private *dev_priv = NULL;

    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
    dev_priv = (struct nim_s3821_private *)dev->priv;

    S3821_PRINTF(NIM_LOG_DBG,"%s: freq %d, bandwidth %d\n ", __FUNCTION__, change_para->freq, change_para->bandwidth);
#ifdef T2_EXTERNAL_EVKIT
    //joey, 20120807, for external sony box support.
    return SUCCESS;
#endif

    ret = nim_s3821_verify_t2_signal(dev,change_para);
    if(ret!=RET_CONTINUE)
    {
        return ret;
    }
    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
    usage_type = change_para->usage_type;

    nim_s3821_get_dvbt2_lock(dev, &lock);
    if ((dev_priv->s3821_cur_channel_info.frequency == change_para->freq) &&
           (dev_priv->s3821_cur_channel_info.channel_bw == change_para->bandwidth) &&
           (1 == lock))
    {
        S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
        if (USAGE_TYPE_CHANCHG == usage_type)
        {
            ch_plp_idx = change_para->plp_index;
        }
        else if (USAGE_TYPE_NEXT_PIPE_SCAN == usage_type)
        {
           // 1. get the data_plp number.
            nim_s3821_read(dev, 0x185, &dat_plp_num, 1);
            if ((change_para->plp_index + 1) < dat_plp_num) // means exist next data PLP.
            {
                ch_plp_idx = change_para->plp_index + 1;
                change_para->plp_index += 1; // update the plp index.
            }
            else // no next one. return directly.
            {
                return ERR_FAILURE;
            }
        }
        else if ((USAGE_TYPE_AUTOSCAN == usage_type) \
                 || (USAGE_TYPE_CHANSCAN == usage_type) \
                 || (USAGE_TYPE_AERIALTUNE == usage_type))
         {
            ch_plp_idx = 0;
            // plp_index is set back to 0.
            change_para->plp_index = 0; // update the plp index.
        }
        //check if the target plp_index is same as the current demod using one.
        //nim_s3821_read(dev, 0x66, &cur_plp_idx, 1);
        nim_s3821_check_rev_id(dev, data);
        if (0 == data[0]) //S3821.
        {
            nim_s3821_read(dev, 0x66, &cur_plp_idx, 1);
        }
        else
        {
//joey, 20140507, for DVBT2 MPLP change plp_id, no need do reset demod.
				    //nim_s3821_read(dev, 0x11b, &cur_plp_idx, 1); // still can read the "cur_plp_idx", else the below "if" will free run.
				    nim_s3821_write(dev, 0x11b, &ch_plp_idx, 1);
        }
        
        if ((cur_plp_idx != ch_plp_idx) && (0 == data[0]))// not same, for S3821, need reset demod..
        {
            //joey, 20121022, no need to 80/40, reset the demod.
            nim_s3821_read(dev, 0x00, data, 1);
            data[0] |= 0x80;                // reset the demod
            nim_s3821_write(dev, 0x00, data, 1);

            // set the plp_index.
            nim_s3821_write(dev, 0x66, &ch_plp_idx, 1);

            nim_s3821_read(dev, 0x00, data, 1);
            data[0] |= 0x40;                // start the demod
            nim_s3821_write(dev, 0x00, data, 1);

            // set the plp_index.
            // wait for demod look, loop+interval check.
            //joey, 20130708, for DVBT2 signal, wait 250ms for blank channel check.
            //joey, 20131015, for DVBT2 signal, for blank channel judging, change from 250ms to 350ms.
            wait_lock_time = 35;
            try_time = 0;
            ret_flag = SCAN_UNLOCK;

            while(1)
            {
                comm_sleep(10);
                try_time++;

                //nim_s3821_get_lock(dev, &lock);
                nim_s3821_read(dev, 0x67, data, 1);
                if ((data[0] & 0x0f) > 0x03)
                {
                    ret_flag = LOCK_OK;
                    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 P1 Lock OK!!!!!!!!!!!!!!!!!!\n");
                    break;
                }

                if(try_time > wait_lock_time)
                {
                    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 P1 Unlock!!!!!!!!!!!!!!!!!!\n");
                    return ERR_FAILURE;
                }
            }

            //joey, 20130708, for DVBT2 signal, wait one more 250ms for final lock wait.
            wait_lock_time = 25;
            try_time = 0;
            ret_flag = SCAN_UNLOCK;
            while(1)
            {
                comm_sleep(10);
                try_time++;
                nim_s3821_get_dvbt2_lock(dev, &lock);
                if (MAGIC_CONST_1 == lock)
                {
                    ret_flag = LOCK_OK;
                    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 lock OK!!!!!!!!!!!!!!!!!!\n");
                    break;
                }

                if(try_time > wait_lock_time) // joey, modify to 250+250 = 500ms, 20130708.
                {
                    S3821_PRINTF(NIM_LOG_DBG,"DVBT2 unlock!!!!!!!!!!!!!!!!!!\n");
                    return ERR_FAILURE;
                }

            }
        }

        //update the t2 information, if need feedback the t2 information.
        nim_s3821_update_dvbt2_info(dev,change_para);

        //joey, as here, to update the GI, FFT_mode, Modulation parameters.
        {
            nim_s3821_get_curr_dvbt2_chninfo(dev);
        }
        return SUCCESS;
    }

    ret=nim_s3821_dvbt2_config_tuner(dev, change_para);
    if(ret!=RET_CONTINUE)
    {
        return ret;
    }

    return nim_s3821_dvbt2_lock_proc(dev,change_para);

 }


/*****************************************************************************
* INT32 nim_s3821_combo_channel_change(struct nim_device *dev, ...);
* Description: s3821 combo channel change operation
* Return Value: INT32
*****************************************************************************/

INT32 nim_s3821_combo_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para)
{
    INT32 result = ERR_FAILUE;
    UINT8 cur_mode = 0;
    UINT8 lock = 0;
    struct nim_s3821_private *dev_priv = NULL;


    if ((change_para->freq <= MAGIC_CONST_40000) || (change_para->freq >= MAGIC_CONST_900000))
    {
        return ERR_FAILUE;
    }

    dev_priv = (struct nim_s3821_private *)dev->priv;

    S3821_PRINTF(NIM_LOG_DBG,"%s: usage_type %d, freq %d, bandwidth %d, priority %d, t2_signal %d, plp_index %d, plp_id %d\r\n",
          __FUNCTION__, change_para->usage_type, change_para->freq, change_para->bandwidth, change_para->priority,
            change_para->t2_signal, change_para->plp_index, change_para->plp_id);

    do
    {
        //joey, 20130705. for the auto-scan or blind mode, for the break signal, pre-dealing.
        dev_priv->autoscan_stop_flag = 0;

        //joey, 20130705, for the auto-scan or blind mode, if already lock, select the first try as current mode.
        if ((USAGE_TYPE_AUTOSCAN == change_para->usage_type) \
                || (USAGE_TYPE_CHANSCAN == change_para->usage_type) \
                || (USAGE_TYPE_AERIALTUNE == change_para->usage_type))
        {
            nim_s3821_get_lock(dev, &lock);
            nim_s3821_get_cur_mode(dev, &cur_mode);

            if ((dev_priv->s3821_cur_channel_info.frequency == change_para->freq) &&
                  (dev_priv->s3821_cur_channel_info.channel_bw == change_para->bandwidth) &&
                  (1 == lock) && (MODE_DVBT2 == cur_mode)) //if this kind, just try as DVBT2 -> DVBT.
            {
                result = nim_s3821_dvbt2_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }

                if (1 == dev_priv->autoscan_stop_flag)
                {
                    break;
                }

                result = nim_s3821_dvbt_isdbt_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }

            }
            else //if DVBT mode or unkown information, just try as DVBT -> DVBT2.
            {
                result = nim_s3821_dvbt_isdbt_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }

                if (1 == dev_priv->autoscan_stop_flag)
                {
                    break;
                }

                result = nim_s3821_dvbt2_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }

            }

        }
        else if (1 == change_para->t2_signal) // when this branch, means already know the signal mode. now is DVBT2.
        {
            result = nim_s3821_dvbt2_channel_change(dev, change_para);
            break;
        }
        else // when this branch, means already know the signal mode. now is DVBT.
        {
            result = nim_s3821_dvbt_isdbt_channel_change(dev, change_para);
            break;
        }

        //joey, 20130705, original combo design, mark off now.
        /*
                result = nim_s3821_dvbt_isdbt_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }

                if (1 == dev_priv->autoscan_stop_flag)
                {
                    break;
                }

                result = nim_s3821_dvbt2_channel_change(dev, change_para);
                if (SUCCESS == result)
                {
                    break;
                }
        */

    }
    while (0);
    return SUCCESS;
}

static INT32 nim_s3821_get_hierarchy_info(struct nim_device *dev, UINT8 priority, UINT8 *hier_info, UINT8 *code_rate)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 data = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    nim_s3821_read(dev, 0x74, &data, 1);
    switch (data & 0x03)
    {
    case 0:
        *hier_info = HIER_NONE;
        break;
    case 1:
        *hier_info = HIER_1;
        break;
    case 2:
        *hier_info = HIER_2;
        break;
    case 3:
        *hier_info = HIER_4;
        break;
    default:
        *hier_info = HIER_NONE; /* error */
        break;

    }

    nim_s3821_read(dev, 0x73, &data, 1);
    if ((*hier_info > HIER_NONE) && (priority == LPSEL))//exist hierachy and need LP.
    {
        switch (data & 0x07)
        {
        case 0:
            *code_rate = FEC_1_2;
            break;
        case 1:
            *code_rate = FEC_2_3;
            break;
        case 2:
            *code_rate = FEC_3_4;
            break;
        case 3:
            *code_rate = FEC_5_6;
            break;
        case 4:
            *code_rate = FEC_7_8;
            break;
        default:
            *code_rate = 0; /* error */
            break;

        }
    }
    else//non-hierachy or need HP.
    {
        switch ((data & 0x38) >> 3)
        {
        case 0:
            *code_rate = FEC_1_2;
            break;
        case 1:
            *code_rate = FEC_2_3;
            break;
        case 2:
            *code_rate = FEC_3_4;
            break;
        case 3:
            *code_rate = FEC_5_6;
            break;
        case 4:
            *code_rate = FEC_7_8;
            break;
        default:
            *code_rate = 0; /* error */
            break;
        }
    }

    if(dev_priv->log_en)
    {
        S3821_PRINTF(NIM_LOG_DBG,"hier=0x%x,code_rate=0x%x \n", *hier_info, *code_rate);
    }

    return SUCCESS;
}


static INT32 nim_s3821_get_int_freqoffset(struct nim_device *dev, INT32 *intf_offset)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 data[2] = {0};
    UINT8 fft_mode = 0;
    INT32 tmp_off = 0;

    //joey, 20111019, for frequency offset check, old code.
    dev_priv = (struct nim_s3821_private *)dev->priv;
    nim_s3821_read(dev, 0x1e, data, 1);
    data[0] = data[0] & 0x30;
    data[0] = data[0] >> 4;
    switch (data[0])
    {
    case 0:
        fft_mode = MODE_2K;
        break;
    case 1:
        fft_mode = MODE_8K;
        break;
    case 2:
        fft_mode = MODE_4K;
        break;
    default:
        fft_mode = MODE_8K; /* error */
        break;
    }

    nim_s3821_read(dev, 0x1e, data, 2);
    if ((data[0] & 0x04) == 0x00) //positive value
    {
        tmp_off = (INT32)(((data[0] & 0x07) << 8) | data[1]);
    }
    else
    {
        tmp_off = (INT32)(((data[0] & 0x07) << 8) | data[1]) ;
        tmp_off = tmp_off - 2048;
    }

    if (dev_priv->cofdm_type == DVBT_TYPE)
    {
        if (dev_priv->s3821_cur_channel_info.channel_bw == BW_6M)
        {
            tmp_off *= 6;
        }

        else if (dev_priv->s3821_cur_channel_info.channel_bw == BW_7M)
        {
            tmp_off *= 7;
        }
        else
        {
            tmp_off *= 8;
        }
    }

    if (dev_priv->cofdm_type == DVBT_TYPE)
    {
        if (fft_mode == MODE_2K)
        {
            *intf_offset = (INT32)((tmp_off * 8 * 1000 / (7 * 2048)));
        }
        else
        {
            *intf_offset = (INT32)((tmp_off * 8 * 1000 / (7 * 8192)));
        }
    }
    else                // ISDBT mode, only support 6M channel.
    {
        if(fft_mode == MODE_2K)
        {
            *intf_offset = (INT32)((tmp_off * 250 / (63)));
        }
        else if (fft_mode == MODE_4K)
        {
            *intf_offset = (INT32)((tmp_off * 125 / (63)));
        }
        else
        {
           *intf_offset = (INT32)((tmp_off * 125 / (126)));
        }
    }

    //S3821_PRINTF(NIM_LOG_DBG,"Int frequency offset is %d !!!!\n", *intf_offset);
    //joey, 20111019, for frequency offset check.
    *intf_offset = 0;
    return SUCCESS;
}


static void nim_s3821_get_curr_dvbt_chninfo(struct nim_device *dev,UINT8 priority)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 tps_guard_interval = 0;
    UINT8 tps_fft_mode = 0;
    UINT8 tps_modulation = 0;
    UINT8 tps_hierarchy = 0;
    UINT8 tps_code_rate = 0;
    UINT8 tps_spectrum = 0;
    INT32 freq_offset = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;

    nim_s3821_getinfo(dev, &tps_guard_interval, &tps_fft_mode, &tps_modulation, &tps_spectrum);

    //joey, 20111108, check if dvbt, and hierachary information.
    if ((DVBT_TYPE == dev_priv->cofdm_type) || (DVBT2_COMBO == dev_priv->cofdm_type))
    {
        nim_s3821_get_hierarchy_info(dev, priority, &tps_hierarchy, &tps_code_rate);
    }

    dev_priv->s3821_cur_channel_info.guard = tps_guard_interval;
    dev_priv->s3821_cur_channel_info.mode = tps_fft_mode;
    dev_priv->s3821_cur_channel_info.modulation = tps_modulation;
    dev_priv->s3821_cur_channel_info.spectrum = tps_spectrum;
    dev_priv->s3821_cur_channel_info.hierarchy = tps_hierarchy;

    dev_priv->s3821_cur_channel_info.fecrates = tps_code_rate;

    nim_s3821_get_int_freqoffset(dev, &freq_offset);
    dev_priv->s3821_cur_channel_info.freq_offset = freq_offset;


}

static INT32 nim_s3821_dvbt_isdbt_lock_proc(struct nim_device *dev,UINT8 usage_type,UINT8 priority,UINT32 bandwidth)
{
    UINT8 try_time = 0;
    UINT8 ret_flag = 0;
    UINT8 data[6] = {0};
    UINT8 wait_lock_time = 0;
    struct nim_s3821_private *dev_priv = NULL;


    dev_priv = (struct nim_s3821_private *)dev->priv;
    //step 2: init cofdm...
    //step 2.1, reset whole demod, just before 80/40.
    //nim_s3821_idle_reset_proc(dev);-->reset s3821 by nim_s3821_reset() API

    nim_s3821_read(dev, 0x00, data, 1);
    data[0] |= 0x80;                // reset the demod
    nim_s3821_write(dev, 0x00, data, 1);

    //clear interrupt.
    data[0] = 0x00;
    nim_s3821_write(dev, 0x02, data, 1);

    //Step3: Config FFT/GI/FEC/Bandwidth ... etc.
    //bandwidth setting.
    nim_s3821_read(dev, 0x13, data, 1);
    data[0] &= 0xcf;        // clear the bit[5:4], chan_bw

    switch(bandwidth)
    {
    case BW_6M:
        data[0] |= (S3821_BW_6M << 4);
        break;
    case BW_7M:
        data[0] |= (S3821_BW_7M << 4);
        break;
    case BW_8M:
        data[0] |= (S3821_BW_8M << 4);
        break;
    default:
        data[0] |= (S3821_BW_8M << 4);
        if(dev_priv->log_en)
            S3821_PRINTF(NIM_LOG_DBG,"Error! UnKnown bandwidth mode\n");
        break;
    }
    nim_s3821_write(dev, 0x13, data, 1);

    //sub carrier space setting.
    //ITP configuation setting.

    //HP or LP setting according to priority para.
    nim_s3821_read(dev, 0x1b, data, 1);
    if (priority == LPSEL)//LP stream setting.
    {
        data[0] = (data[0] & 0xef) | 0x00;
    }
    else//Hp or default setting.
    {
        data[0] = (data[0] & 0xef) | 0x10;
    }
    nim_s3821_write(dev, 0x1b, data, 1);

    //Step4: Start to Capture
    nim_s3821_read(dev, 0x00, data, 1);
    data[0] |= 0x40;
    nim_s3821_write(dev, 0x00, data, 1);

    //Step4.5:Wait TPS LOCK
    try_time = 0;
    ret_flag = TPS_UNLOCK;

    while(1)
    {
        comm_sleep(10);
        try_time++;

        nim_s3821_read(dev, 0x1d, data, 1);
        if (0x20 == (data[0] & 0x20))
        {
            ret_flag = LOCK_OK;
            break;
        }

        if(try_time >= MAGIC_CONST_70)
        {
            S3821_PRINTF(NIM_LOG_DBG,"TPS unlock, FSM is 0x%2x   !!!!!!!!!!!!!!!!!!\n", data[0]);
            ret_flag = TPS_UNLOCK;
            return ERR_FAILUE; //maybe exist channel.
        }
    }


    if (usage_type == MODE_CHANCHG)
    {
        wait_lock_time = 40;
    }
    else
    {
        wait_lock_time = 25;
    }

    try_time = 0;
    ret_flag = SCAN_UNLOCK;
    while(1)
    {
        comm_sleep(10);
        try_time++;

        nim_s3821_read(dev, 0x1d, data, 1);
        if (0x40 == (data[0] & 0x40))
        {
            ret_flag = LOCK_OK;
            break;
        }

        //actually, when TPS lock, COFDM is very easy lock. So we should length TPS time.
        if(try_time > wait_lock_time * 5)
        {
            S3821_PRINTF(NIM_LOG_DBG,"COFDM unlock, FSM is 0x%2x   !!!!!!!!!!!!!!!!!!\n", data[0]);
            return ERR_FAILUE;
        }
    }

    S3821_PRINTF(NIM_LOG_DBG,"COFDM lock, FSM is 0x%2x   !!!!!!!!!!!!!!!!!!\n", data[0]);

    if (ret_flag == LOCK_OK)
    {
        nim_s3821_get_curr_dvbt_chninfo(dev,priority);
        return SUCCESS;
    }

    return ERR_FAILUE;
}




INT32 nim_s3821_dvbt_isdbt_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para)
{
    UINT32 freq = 0;
    UINT32 bandwidth = 0;
    UINT8 usage_type = 0;
    UINT8 priority = 0;

    UINT8 lock = 0;
    UINT8 tuner_retry = 0;
    UINT8 data[6] = {0};
    struct nim_s3821_private *dev_priv = NULL;
    UINT32 tuner_id = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);

    //joey, 20130627. for dvbt/isdbt driver call.
    usage_type = change_para->usage_type;
    freq = change_para->freq;
    bandwidth = change_para->bandwidth;
    priority = change_para->priority;
    tuner_id = dev_priv->tuner_id;

    if (dev_priv->log_en)
    {
    //S3821_PRINTF(NIM_LOG_DBG,"%s: usage_type %d, freq %d, bandwidth %d, priority %d, t2_signal %d, plp_index %d, plp_id %d\r\n",
    //      __FUNCTION__, change_para->usage_type, change_para->freq, change_para->bandwidth, change_para->priority,
    //        change_para->t2_signal, change_para->plp_index, change_para->plp_id);
    }

    //joey, 20130703, for dvbt/t2 combo solution.
    if (DVBT2_COMBO == dev_priv->cofdm_type)
    {
        if ((USAGE_TYPE_NEXT_PIPE_SCAN == change_para->usage_type) || \
                ((USAGE_TYPE_CHANCHG == change_para->usage_type) && (1 == change_para->t2_signal)))
        {
            return ERR_FAILUE;
        }
        else
        {
            //check if need switch work mode, or already is.
            if (DVBT_TYPE != dev_priv->cur_type)
            {
                UINT8 cur_mode = 0;

                //set demod to DVB-T2 mode.
                cur_mode = MODE_DVBT;
                nim_s3821_set_cur_mode(dev, &cur_mode);
                dev_priv->cur_type = DVBT_TYPE;
            }
        }
    }

    //joey, 20130705, when set mode to DVBT, can set the t2_signal indicator as DVBT here.
    if ((DVBT_TYPE == dev_priv->cofdm_type) || (DVBT2_COMBO == dev_priv->cofdm_type))
    {
        if ((USAGE_TYPE_AUTOSCAN == usage_type) \
                || (USAGE_TYPE_CHANSCAN == usage_type) \
                || (USAGE_TYPE_AERIALTUNE == usage_type))
        {
            change_para->t2_signal = 0; // not dvbt2, should be dvbt.
        }
    }

    //step 0: If same config data, and dem work in lock state, not config dem again.
    nim_s3821_read(dev, 0x1d, data, 1);
    if (((data[0] & 0x40) == 0x40) && (freq == dev_priv->s3821_cur_channel_info.frequency) &&
                        (bandwidth == dev_priv->s3821_cur_channel_info.channel_bw))
    {
        //joey. 20081103. for M3101C hierarchy LP mode work un-correct issue.
        //HP or LP setting according to priority para.
        // ???? for DVB-T not realization yet.

        //joey, 20111108, check if dvbt, and hierachary information.
        if ((DVBT_TYPE == dev_priv->cofdm_type) || (DVBT2_COMBO == dev_priv->cofdm_type))
        {
            nim_s3821_read(dev, 0x1b, data, 1);
            if (LPSEL == priority)
            {
                data[0] = (UINT8)((data[0] & 0xef) | 0x00); // Lp selection.
            }
            else
            {
                data[0] = (UINT8)((data[0] & 0xef) | 0x10); // Hp selection.
            }
            nim_s3821_write(dev, 0x1b, data, 1);
        }

        nim_s3821_get_curr_dvbt_chninfo(dev,priority);
        return SUCCESS;
    }

    //step 1: config tuner.
    do
    {
        //joey, 20130703, for dvbt/t2 combo solution.
        if (DVBT2_COMBO == dev_priv->cofdm_type)
        {
            if ((change_para->freq == dev_priv->s3821_cur_channel_info.frequency) &&
                      (change_para->bandwidth == dev_priv->s3821_cur_channel_info.channel_bw))
            {
                if(dev_priv->tuner_control.nim_tuner_status(tuner_id, &lock) == ERR_FAILUE)
                {
                    //if i2c read failure, no lock state can be report.
                    lock = 0;
                }

                if (1 == lock)
                {
                    break;
                }
            }
        }

        tuner_retry++;

        if ((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_maxlinear)
        {
            if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth,
                            FAST_TIMECST_AGC, data, _1ST_I2C_CMD) == ERR_FAILUE)
            {
                S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
            }
        }
        else
        {
            if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth,
                          FAST_TIMECST_AGC, data, _1ST_I2C_CMD) == ERR_FAILUE)
            {
                S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
            }

            if((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_infineon)
            {
				comm_sleep(10);
                if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth,
                             FAST_TIMECST_AGC, data, _2ND_I2C_CMD) == ERR_FAILUE)
                {
                    S3821_PRINTF(NIM_LOG_DBG,"Config tuner failed step 2!\n");
                }
            }

            //attention: tuner ATC constant time should always at normal state after setting, not fast state
            // whatever the system in search channel or play channel!

            if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, SLOW_TIMECST_AGC,
                                        data, _1ST_I2C_CMD) == ERR_FAILUE)
            {
              S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
            }

            if((dev_priv->tuner_control.tuner_config.c_chip) == tuner_chip_infineon)
            {
                comm_sleep(10);
                S3821_PRINTF(NIM_LOG_DBG,"SLOW_TIMECST_AGC_2nd_i2c_cmd\n");
                if(dev_priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, SLOW_TIMECST_AGC,
                                 data, _2ND_I2C_CMD) == ERR_FAILUE)
                {
              S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,here!\n",__FUNCTION__,__LINE__);
                }
            }
        }

        if(dev_priv->tuner_control.nim_tuner_status(tuner_id, &lock) == ERR_FAILUE)
        {
            //if i2c read failure, no lock state can be report.
            lock = 0;
            S3821_PRINTF(NIM_LOG_DBG,"ERROR! Tuner Rread Status Fail\n");
        }
        S3821_PRINTF(NIM_LOG_DBG,"Tuner Lock Times=0x%d,Status=0x%d\n", tuner_retry, lock);
        if(tuner_retry > MAGIC_CONST_5)
        {
            return ERR_FAILUE;
        }
    }
    while(0 == lock);

    dev_priv->s3821_cur_channel_info.frequency = freq;
    dev_priv->s3821_cur_channel_info.channel_bw = bandwidth;

    return nim_s3821_dvbt_isdbt_lock_proc(dev,usage_type,priority,bandwidth);

}


/*****************************************************************************
* INT32 nim_s3821_get_osd_int_freqoffset(struct nim_device *dev, UINT32 *freq)
* Get real freq offset and displayed on OSD.
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT32 *freq            :
*
* Return Value: void
*****************************************************************************/
INT32 nim_s3821_get_osd_int_freqoffset(struct nim_device *dev, INT32 *intf_offset)
{
    INT32 tmp_offset = 0;
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    nim_s3821_get_int_freqoffset(dev, &tmp_offset);
    //for zero-if tuner , no i/q swap, the freq offset is real freq offset.
    if ( (dev_priv->tuner_control.config_data.cofdm_config & 0x10) == 0x10)
    {
        *intf_offset = -tmp_offset;
    }
    *intf_offset = 0;        // add by sean for debug 2011/04/02
    return SUCCESS;

}

/*****************************************************************************
* INT32 nim_s3821_get_freq(struct nim_device *dev, UINT32 *freq)
* Read M3327 frequence
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT32 *freq            :
*
* Return Value: void
*****************************************************************************/
INT32 nim_s3821_get_freq(struct nim_device *dev, UINT32 *freq)
{
    INT32 intf_offset = 0;
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    nim_s3821_get_int_freqoffset(dev, &intf_offset);
    //for zero-if tuner , no i/q swap, the freq offset is real freq offset.
    if ( (dev_priv->tuner_control.config_data.cofdm_config & 0x10) == 0x10)
    {
        *freq = (UINT32)(dev_priv->s3821_cur_channel_info.frequency + intf_offset);
    }
    else       //for low-if tuner , i/q swapped,  the freq offset is sign invert.
    {
        *freq = (UINT32)(dev_priv->s3821_cur_channel_info.frequency - intf_offset);
    }

    if(dev_priv->log_en)
    {
        S3821_PRINTF(NIM_LOG_DBG,"Real offset is %d, final freq is %d . \n", intf_offset, *freq);
    }

    return SUCCESS;
}



/*****************************************************************************
* INT32 nim_s3821_get_code_rate(struct nim_device *dev, UINT8* code_rate)
* Description: Read m3101 code rate
*   FEC status (b6-b4)                      code rate                 return value
*    0                    1/2            1
*    1                    2/3            4
*    2                    3/4            8
*    3                    5/6            16
*    5                    7/8            32
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8* code_rate
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_get_code_rate(struct nim_device *dev, UINT8 *code_rate)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *code_rate = dev_priv->s3821_cur_channel_info.fecrates;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3821_get_gi(struct nim_device *dev, UINT8 *guard_interval)
* Description: Read m3101 guard interval
*  Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8* guard_interval
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_get_gi(struct nim_device *dev, UINT8 *guard_interval)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *guard_interval = dev_priv->s3821_cur_channel_info.guard;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3821_get_fftmode(struct nim_device *dev, UINT8 *fft_mode)
* Description: Read m3101 fft_mode
*  Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8* fft_mode
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_get_fftmode(struct nim_device *dev, UINT8 *fft_mode)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *fft_mode = dev_priv->s3821_cur_channel_info.mode;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3821_get_modulation(struct nim_device *dev, UINT8 *modulation)
* Description: Read m3101 modulation
*  Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8* modulation
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_get_modulation(struct nim_device *dev, UINT8 *modulation)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *modulation = dev_priv->s3821_cur_channel_info.modulation;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3821_get_specinv(struct nim_device *dev, UINT8 *inv)
*
*  Read FEC lock status
*
*Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8 *inv
*
*Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_get_specinv(struct nim_device *dev, UINT8 *inv)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *inv = dev_priv->s3821_cur_channel_info.spectrum;
    return SUCCESS;
}


INT32 nim_s3821_get_freq_offset(struct nim_device *dev, INT32 *freq_offset)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *freq_offset = dev_priv->s3821_cur_channel_info.freq_offset;
    return SUCCESS;
}



INT32 nim_s3821_get_hier_mode(struct nim_device *dev, UINT8 *hier)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *hier = dev_priv->s3821_cur_channel_info.hierarchy;
    return SUCCESS;
}

INT32 nim_s3821_get_agc(struct nim_device *dev, UINT8 *agc)
{
	UINT8 data[3];
	UINT16 if_ad_val;
	UINT8 cur_mode;
	//joey, 20140527, for DVBT2 FEF SSI display.
	UINT16 tmp_ssi;
	UINT8 cur_ssi;	
	UINT32  flgptn;
	UINT32  flag_ptn = 0;
	//OSAL_ER result;

	struct nim_s3821_private *priv = (struct nim_s3821_private *)dev->priv;
	struct COFDM_TUNER_CONFIG_API *   config_info = &(priv->tuner_control);

	flag_ptn = nim_flag_read(&priv->flag_lock, NIM_3821_SCAN_END, OSAL_TWF_ANDW , 0);
	if((flag_ptn & NIM_3821_SCAN_END) && (flag_ptn != OSAL_INVALID_ID)) 
	{
		// get IF AD value. 
		// IF_AGC_GAIN is signed value. do the signed bit check.value range is -512~+511.
		//so we should shift it +512 first.
		nim_s3821_get_cur_mode(dev, &cur_mode);

		if (MODE_DVBT2 == cur_mode)
		{
			nim_s3821_read(dev, 0x86, data, 2);
		}
		else if ((MODE_DVBT == cur_mode) || (MODE_ISDBT == cur_mode))
		{
			nim_s3821_read(dev, 0x30, data, 2);
		}
		
		//nim_s3821_read(dev, 0x86, data, 2);
		if_ad_val = (UINT16)((data[1]&0x03)<<8 | data[0]);
		if_ad_val = (UINT16)((if_ad_val + 512) & 0x3ff);            // shift from (-512~511) to (0~1023).			

		S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);

		if(SUCCESS ==  nim_s3821_tuner_ioctl(dev, NIM_TUNER_GET_AGC, &if_ad_val))
		{  
			S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);
			if(0x80 == (config_info->config_data.cofdm_config & 0x80))
		     {
		            S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);
			        *agc = if_ad_val;
			        return SUCCESS;
		     }
		     else
		     {
		        S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);

//joey, 20140414, for dvbt/dvbt2 SSI function enhance.
//dvbt part.
static const UINT8 ssi_table[3][5] = {{14,16,17,18,19}, \
								{20,22,23,24,25}, \
								{25,27,29,30,31}};
//dvbt2 part.
static const UINT8 ssi_table_t2[4][6] = {{11,12,13,14,15,15}, \
									{16,18,19,20,21,21}, \
									{21,22,24,25,26,27}, \
									{25,27,29,31,32,33}};
//function part.
		UINT8 ssi_ref = 0;
		UINT8 tmp_cr, tmp_con;

		if (MODE_DVBT == cur_mode) //dvbt.
		{
			nim_s3821_read(dev, 0x73, data, 2);
			tmp_cr = (data[0] & 0x38) >> 3;
			tmp_con = (data[1] & 0x0c) >> 2;

			ssi_ref = ssi_table[tmp_con][tmp_cr];
		}
		else if (MODE_DVBT2 == cur_mode) // dvbt2.
		{
			nim_s3821_read(dev, 0x183, data, 1);
			tmp_cr = (data[0] & 0x07);
			tmp_con = (data[0] & 0x18) >> 3;

			ssi_ref = ssi_table_t2[tmp_con][tmp_cr];
		}

		if ((MODE_DVBT == cur_mode) || (MODE_DVBT2 == cur_mode)) //dvbt or dvbt2.
		{
//joey, 20140414, if want the resolution as 1%, need the return value of rf_level as 0.1dbuV. 
//joey, 20140414, now return value is 1dbuV, step is not 1%, but still can pass Nordig SSI.
			if ((if_ad_val + 15) < ssi_ref) // Prel < -15dB
			{
				cur_ssi = 0;
			}
			else if (if_ad_val < ssi_ref) // -15 dB ¡Ü Prel < 0dB
			{
				cur_ssi = ((if_ad_val + 15 -ssi_ref) * 20/3 + 5)/10;
			}
			else if (if_ad_val < (ssi_ref+20)) // 0 dB ¡Ü Prel < 20 dB
			{
				cur_ssi = ((if_ad_val -ssi_ref) * 4 + 10);
			}
			else if (if_ad_val < (ssi_ref+35)) // 20 dB ¡Ü Prel < 35 dB
			{
				cur_ssi = ((if_ad_val -ssi_ref -20) * 20/3 + 5)/10 + 90;
			}
			else // Prel ¡Ý 35 dB
			{
				cur_ssi = 100;
			}

			//NIM_S3821_DEBUG("rf_level %d, ssi_ref %d, SSI: %d!\n", if_ad_val, ssi_ref, cur_ssi);
			
		}
		else //isdbt or other.
		{
		    S3821_PRINTF(NIM_LOG_DBG,"[%s] line=%d \n",__FUNCTION__,__LINE__);
			cur_ssi = if_ad_val;
		}

		//NIM_S3821_DEBUG("SSI: %d!\n", cur_ssi);
		if (MODE_DVBT2 == cur_mode) //dvbt2.
		{
			nim_s3821_get_dvbt2_lock(dev, data);
			if(1 == data[0])
			{
				nim_s3821_read(dev, 0x178, data, 3);
				data[0] = data[0] | data[1] | data[2]; // if not a "zero" value, means FEF.

				if (data[0] > 0) // FEF mode.
				{
					if (0 == cur_ssi) // for SFU or dectec, if FEF, seems the SSI always to "0"; sure it is FEF case.
					{
						cur_ssi = priv->last_ssi;
						priv->con_ssi_small_cnt = 0;
					}
					else if ((cur_ssi + 5) < (priv->last_ssi)) // gap is big.
					{
						if (priv->con_ssi_small_cnt < 10)
						{
							cur_ssi = priv->last_ssi;

							priv->con_ssi_small_cnt += 1;
						}
						else
						{
							//update the last_ssi of FEF.
							priv->last_ssi = cur_ssi;					

							priv->con_ssi_small_cnt = 0;
						}
					}
					else // update last_ssi value.
					{
						//update the last_ssi of FEF.
						priv->last_ssi = cur_ssi;					
						priv->con_ssi_small_cnt = 0;
					}

				}
			}
			
		}		
			*agc = cur_ssi;	
			return SUCCESS;
		}
	  }
	}
    else{
	  *agc = 0x0;
	  return SUCCESS;
	}
	
	*agc = 0x70;
	return SUCCESS;
}




INT32 nim_s3821_get_ber(struct nim_device *dev, UINT32 *vbber)
{
    struct nim_s3821_private *dev_priv = NULL;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    *vbber = dev_priv->snr_ber;
    return SUCCESS;
}

