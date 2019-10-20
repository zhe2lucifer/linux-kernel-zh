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

*    File:  nim_s3821_ic_02.c
*
*    Description:  s3821 ic layer
*    History:
*           Date            Athor        Version          Reason
*        ============    =============    =========    =================
*    1.    6.14.2013        Joey.Gao         Ver 1.0       Create file.
*
*****************************************************************************/

#include "nim_s3821.h"

#define MAGIC_CONST_0X0A     0x0a
#define MAGIC_CONST_8000     8000
#define MAGIC_CONST_6000     6000
#define MAGIC_CONST_500      500
#define MAGIC_CONST_1500     1500
#define MAGIC_CONST_1000     1000
#define MAGIC_CONST_200      200
#define MAGIC_CONST_100      100
#define MAGIC_CONST_19       19
#define MAGIC_CONST_18       18
#define MAGIC_CONST_10       10
#define MAGIC_CONST_6        6
#define MAGIC_CONST_1        1
#define MAGIC_CONST_0        0



static UINT8 test_mux_st = 0; // disable.
static UINT32 reg_494 = 0;
static UINT32 reg_490 = 0;
static UINT32 reg_074 = 0;



//joey, 20111019, for TF illegal interrupt service.
INT32 nim_s3821_tf_int_proc(struct nim_device *dev)
{
    UINT8 data = 0;
    UINT8 tmp_data = 0;

    // step A. read out interrupt information.
    nim_s3821_read(dev, 0x02, &data, 1);

    // Step B. check if tf buffer exception.
    if (0x20 == (data & 0x20))
    {
        tmp_data = data & 0xdf;

        S3821_PRINTF(NIM_LOG_DBG,"Enter TF exception interrupt state!\n");

        // step C: 80/40 the COFDM.
        data = 0x80;
        nim_s3821_write(dev, 0x00, &data, 1);

        // clear the TF interrupt here.
        nim_s3821_write(dev, 0x02, &tmp_data, 1);

        data = 0x40;
        nim_s3821_write(dev, 0x00, &data, 1);
        S3821_PRINTF(NIM_LOG_DBG,"Finish TF exception interrupt process!\n");
    }


    return SUCCESS;
}

//joey, 20120503, for unlock interrupt proc service.
//joey, 20120614, for add-in tf exp interrupt proc service.
static INT32 nim_s3821_unlock_int_proc(struct nim_device *dev)
{
    UINT8 data = 0;

    // step A. read out interrupt information.
    nim_s3821_read(dev, 0x02, &data, 1);

    // Step B. check if unlock or tf buffer exception.
    if ((0x40 == (data & 0x40)) || (0x20 == (data & 0x20)))
    {
        //libc_printf("----------------unlock interrupt whole chip reset!\n");
        //find the interrupt
        struct nim_s3821_private *dev_priv = (struct nim_s3821_private *)(dev->priv);

        UINT32 tmp_data = 0;

        //joey, 20120613, for register access protection when HW operation.
        //osal_mutex_lock(dev_priv->nim_s3821_i2c_mutex, OSAL_WAIT_FOREVER_TIME);
        NIM_MUTEX_ENTER(dev_priv);

        //step 1: do the COFDM whole reset.

        tmp_data = *(volatile UINT32 *)0xb8000084;
        //comm_delay(10);
		comm_sleep(10);

        tmp_data = tmp_data | (1 << 22);
        *(volatile UINT32 *)0xb8000084 = tmp_data;
        //comm_delay(10);
        comm_sleep(10);

        tmp_data = tmp_data & (~(1 << 22));
        *(volatile UINT32 *)0xb8000084 = tmp_data;
        //comm_delay(10);
        comm_sleep(10);

        //joey, 20120613, for register access protection when HW operation.
        //osal_mutex_unlock(dev_priv->nim_s3821_i2c_mutex);
        NIM_MUTEX_LEAVE(dev_priv);
		
       // step 2: store in the init register values.
        nim_s3821_init_config(dev);

        //set back the channel bw.
        //bandwidth setting.
        nim_s3821_read(dev, 0x13, &data, 1);
        data &= 0xcf;        // clear the bit[5:4], chan_bw

        switch(dev_priv->s3821_cur_channel_info.channel_bw)
        {
        case BW_6M:
            data |= (S3821_BW_6M << 4);
            break;
        case BW_7M:
            data |= (S3821_BW_7M << 4);
            break;
        case BW_8M:
            data |= (S3821_BW_8M << 4);
            break;
        default:
            data |= (S3821_BW_8M << 4);
            if(dev_priv->log_en)
            {
                S3821_PRINTF(NIM_LOG_DBG,"Error! UnKnown bandwidth mode\n");
            }
            break;
        }

        nim_s3821_write(dev, 0x13, &data, 1);
        // step 3: 80/40 the COFDM.
        data = 0x80;
        nim_s3821_write(dev, 0x00, &data, 1);
        data = 0x40;
        nim_s3821_write(dev, 0x00, &data, 1);

        S3821_PRINTF(NIM_LOG_DBG,"Finish TF exception interrupt process!\n");
    }

    return SUCCESS;

}


//joey, 20120503, for idle reset proc service.
INT32 nim_s3821_idle_reset_proc(struct nim_device *dev)
{
    UINT8 data = 0;

   //joey, 20120504, add for only act as C3811 needs.
    nim_s3821_read(dev, 0x3b, &data, 1);
    if (MAGIC_CONST_1 == data) // check if S3811/C3811.
    {
        nim_s3821_read(dev, 0x147, &data, 1);
        if (data > MAGIC_CONST_0) // denote for C3811.
        {
           //joey, 20120613, for register access protection when HW operation.

            struct nim_s3821_private *dev_priv = (struct nim_s3821_private *)(dev->priv);
        UINT32 tmp_data = 0;

            //osal_mutex_lock(dev_priv->nim_s3821_i2c_mutex, OSAL_WAIT_FOREVER_TIME);
            NIM_MUTEX_ENTER(dev_priv);
			
            //libc_printf("----------------80/40 whole chip reset!\n");
            //step 1: do the COFDM whole reset.
            tmp_data = *(volatile UINT32 *)0xb8000084;
            //comm_delay(10);
	    comm_sleep(10);

            tmp_data = tmp_data | (1 << 22);
            *(volatile UINT32 *)0xb8000084 = tmp_data;
            //comm_delay(10);
	    comm_sleep(10);
            tmp_data = tmp_data & (~(1 << 22));
            *(volatile UINT32 *)0xb8000084 = tmp_data;
            //comm_delay(10);
	    comm_sleep(10);

            //joey, 20120613, for register access protection when HW operation.
            //osal_mutex_unlock(dev_priv->nim_s3821_i2c_mutex);
            NIM_MUTEX_LEAVE(dev_priv);

            // step 2: store in the init register values.
            nim_s3821_init_config(dev);
        }
    }

    return SUCCESS;

}

//joey, 20111101, for AC information proc.
static INT32 nim_s3821_ac_info_proc(struct nim_device *dev)
{
    UINT8 data = 0;
    UINT8 tmp_arr[4];
    UINT8 ac_len = 0;
    UINT8 i = 0;

    //step 1. read out interrupt information.
    nim_s3821_read(dev, 0x02, &data, 1);
    if (0x08 == (data & 0x08)) // means AC buffer overflow.
    {
        //clear interrupt bit.
        data = data & 0xf7;
        nim_s3821_write(dev, 0x02, &data, 1);
        S3821_PRINTF(NIM_LOG_DBG,"AC buffer overflow interrupt!!!!!!!!!!!!!!!!!!!!!\n");
    }
    else if (0x10 == (data & 0x10)) // means AC info arrive.
    {
        //clear interrupt bit.
        data = data & 0xef;
        nim_s3821_write(dev, 0x02, &data, 1);

        //read out current ac len.
        nim_s3821_read(dev, 0x11f, &data, 1);
        ac_len = (data & 0x3f);

        //real read ac info action
        for (i = 0; i < ac_len; i++)
        {
            nim_s3821_read(dev, 0x120, tmp_arr, 4);
            S3821_PRINTF(NIM_LOG_DBG,"AC data[31:0]: 0x%2x, 0x%2x, 0x%2x, 0x%2x! \n", tmp_arr[3],
                   tmp_arr[2], tmp_arr[1], tmp_arr[0]);
        }
    }

    return SUCCESS;

}

//joey, 20130719, for T2 256Qam, 1/2CR CNR issue, change CR119 proc.
INT32 nim_s3821_ldpc_proc(struct nim_device *dev)
{
    UINT8 data = 0;

    nim_s3821_read(dev, 0x67, &data, 1);
    if ((data & 0x0f) >= 0x0a)
    {
        nim_s3821_read(dev, 0x183, &data, 1);
        if (0x58 == (data & 0x5e)) // CR183 [6] = 1, [4:3] = 3, [2:0] = 0 or 1.
        {
            nim_s3821_read(dev, 0x119, &data, 1);
            if (0x5a != data)
            {
                data = 0x5a;
                nim_s3821_write(dev, 0x119, &data, 1); // set CR119 to 0x5a.
            }
        }
        else
        {
            nim_s3821_read(dev, 0x119, &data, 1);
            if (0x65 != data)
            {
                data = 0x65;
                nim_s3821_write(dev, 0x119, &data, 1); // set CR119 to 0x65.
            }
        }
    }
}

//joey, 20130711, for ADC2DMA function.
INT32 nim_s3821_adc2dma_func_start(struct nim_device *dev, UINT32 type)
{
    UINT8 data = 0;
    UINT16 tmp_cnt = 0;
    struct nim_s3821_private *dev_priv = NULL;
    INT32 ret_val = ERR_FAILURE;

    dev_priv = (struct nim_s3821_private *)dev->priv;

    // step 0: force demod init offset free thus demod unlock.
    nim_s3821_get_cur_mode(dev, &data);
    if (MODE_DVBT2 == data) // for dvb-t2 branch.
    {
        //first, disable the watch dog.
        nim_s3821_read(dev, 0x02, &data, 1);
        data = data & 0xfe;
        nim_s3821_write(dev, 0x02, &data, 1);

        //second, force the FREQ_OFFSET to "0".
        nim_s3821_read(dev, 0x64, &data, 1);
        dev_priv->rec_init_offset = data;

        data = 0x00;
        nim_s3821_write(dev, 0x64, &data, 1);

//joey, 20140326. for ADC2DMA force demod unlock. 
    		data = 0x80;
		    nim_s3821_write(dev, 0x00, &data, 1);		
    		data = 0x40;
	    	nim_s3821_write(dev, 0x00, &data, 1);		
    }
    else if ((MODE_DVBT == data) || (MODE_ISDBT == data)) // for dvbt or isdb-t branch.
    //else if (MODE_ISDBT == data) // for isdb-t branch.
    {
        //just force the FREQ_OFFSET to "0".
        nim_s3821_read(dev, 0x13, &data, 1);
        dev_priv->rec_init_offset = (data & 0x0f);

        data = (data & 0xf0);
        nim_s3821_write(dev, 0x13, &data, 1);

//joey, 20140326. for ADC2DMA force demod unlock. 
    		data = 0x80;
		    nim_s3821_write(dev, 0x00, &data, 1);		
    		data = 0x40;
    		nim_s3821_write(dev, 0x00, &data, 1);		
    }

    // step 0: Change to ADC2DMA memory space directly.
    nim_s3821_read(dev, 0x2ff, &data, 1);
    data = data | 0x80;
    nim_s3821_write(dev, 0x2ff, &data, 1);
    /*
    // step 1: get memory address and len.
    //because the below is from CPU address, get rid of the highest 4bit when set to adc2dma register.

        tmp_1 = (NIM_S3821_ADC2DMA_START_ADDR & 0x0fffffff) >> 2; // base_address/4.
    //    tmp_1 = (((UINT32)(dev_priv->Tuner_Control.config_data.memory))  & 0x0fffffff) >> 2; // base_address/4.


        tmp_len = (NIM_S3821_ADC2DMA_MEM_LEN) >> 13; // 8K uinit.
        //tmp_len = dev_priv->Tuner_Control.config_data.memory_size >> 13; // 8K uinit.
        //tmp_len = (70*1024*1024) >> 13; // 8K uinit.

        if (tmp_len <= 0) // no real buffer, return.
        {
            return ERR_FAILURE;
        }



        // step 2: set memory address to buffer address.
        tmp_arr[0] = (UINT8)(tmp_1 & 0xff);
        tmp_arr[1] = (UINT8)((tmp_1>>8) & 0xff);
        tmp_arr[2] = (UINT8)((tmp_1>>16) & 0xff);
        tmp_arr[3] = (UINT8)((tmp_1>>24) & 0x0f);

        nim_s3821_write(dev, 0x04, tmp_arr, 4);


        tmp_arr[0] = (UINT8)(tmp_len & 0xff);
        tmp_arr[1] = (UINT8)((tmp_len>>8) & 0x7f);

        nim_s3821_write(dev, 0x02, tmp_arr, 2);
    */

    //clear the cap_end flag if need.

    //osal_task_sleep(10);
//joey, 20140326, for demod reset, wait for 100ms for AGC lock, but risk in FEF case.
    comm_sleep(100);

    data = 0x00;
    nim_s3821_write(dev, 0x00, &data, 1);

    nim_s3821_read(dev, 0x00, &data, 1);
    S3821_PRINTF(NIM_LOG_DBG,"ADC2DMA CR00 is 0x%2x !\n", data);

    //reset adc2dma.
    nim_s3821_read(dev, 0x08, &data, 1);
    data |= 0x44;
    nim_s3821_write(dev, 0x08, &data, 1);

    comm_sleep(1);

    //start adc2dma.
    nim_s3821_read(dev, 0x08, &data, 1);
    data |= 0x42;
    nim_s3821_write(dev, 0x08, &data, 1);

    //need wait for sometime for AGC stable.
    //comm_sleep(2000);

    // step 5, polling the interrupt status.
    tmp_cnt = 0;
    while (1)
    {
        comm_sleep(10); // wait 10ms.
        nim_s3821_read(dev, 0x00, &data, 1);
        if (0x02 == (data & 0x02)) //adc2dma overflowcase.
        {
            ret_val = ERR_FAILURE;
            S3821_PRINTF(NIM_LOG_DBG,"ADC2DMA overflow, tmp_cnt is %d !\n", tmp_cnt);
            break;
        }
        else if (0x01 == (data & 0x01))
        {
            ret_val = SUCCESS;
            S3821_PRINTF(NIM_LOG_DBG,"ADC2DMA OK, tmp_cnt is %d !\n", tmp_cnt);
            break;
        }
        else
        {
            tmp_cnt += 1;
            if (tmp_cnt > MAGIC_CONST_6000) // 60s.
            {
                S3821_PRINTF(NIM_LOG_DBG,"ADC2DMA timeout, tmp_cnt is %d !\n", tmp_cnt);
                ret_val = ERR_FAILURE;
                break;
            }

        }

    }

    return ret_val;
}

//joey, 20111103, for ADC2DMA function quit after data dump finished.
INT32 nim_s3821_adc2dma_func_stop(struct nim_device *dev, UINT32 type)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 data = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    //when dump data finished, set back to normal mode.

    nim_s3821_read(dev, 0x2ff, &data, 1);
    data = data & 0x7f;
    nim_s3821_write(dev, 0x2ff, &data, 1);

    // step 0: force demod offset certain thus demod work normal.
    nim_s3821_get_cur_mode(dev, &data);
    if (MODE_DVBT2 == data) // for dvb-t2 branch.
    {
        //first, enable as requirement.
        nim_s3821_read(dev, 0x02, &data, 1);
        data = data | 0x01;
        nim_s3821_write(dev, 0x02, &data, 1);

        //second, force the FREQ_OFFSET to "normal".
        data = dev_priv->rec_init_offset;
        nim_s3821_write(dev, 0x64, &data, 1);
    }
    else if ((MODE_DVBT == data) || (MODE_ISDBT == data)) // for dvbt or isdb-t branch.
    //else if (MODE_ISDBT == data) // for isdb-t branch.
    {
        //just force the FREQ_OFFSET to "normal".
        nim_s3821_read(dev, 0x13, &data, 1);
        data = (data & 0xf0) | dev_priv->rec_init_offset;
        nim_s3821_write(dev, 0x13, &data, 1);
    }

    return SUCCESS;
}

INT32 nim_s3821_get_cur_mode(struct nim_device *dev, UINT8 *mode)
{
    UINT8 data = 0;

    nim_s3821_read(dev, 0x2ff, &data, 1);
    *mode = (data & 0xf0) >> 4;

	//S3821_PRINTF(NIM_LOG_DBG,"[%s]line=%d,mode=%d!\n",__FUNCTION__,__LINE__,*mode);
    return SUCCESS;
}

INT32 nim_s3821_set_cur_mode(struct nim_device *dev, UINT8 *mode)
{
    UINT8 data = 0;

    data = 0x80;
    nim_s3821_write(dev, 0x00, &data, 1);
    //joey, 20140604, for switch mode safety, wait for a moment.
    comm_sleep(1); // 1ms.

    nim_s3821_read(dev, 0x2ff, &data, 1);
    data = (data & 0x0f) | (((*mode) & 0x0f) << 4);
    nim_s3821_write(dev, 0x2ff, &data, 1);

    return SUCCESS;
}

INT32 nim_s3821_dvbt_isdbt_monitor_ber(struct nim_device *dev, UINT8 *ber_vld, UINT32 *m_vbber, UINT32 *m_per)
{
    struct nim_s3821_private *dev_priv = NULL;

    UINT8 data = 0;
    UINT8 data_b[3] = {0};
    UINT32 btemp = 0;
    UINT32 rec_ber = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    if (ber_vld)
    {
        *ber_vld = 0;
    }
    if (m_vbber)
    {
        *m_vbber = 0;
    }
    if (m_per)
    {
        *m_per = 0;
    }


    nim_s3821_read(dev, 0x1d, &data, 1);
    data = data & (1 << 6);            // for S3821, sean, modified by sean
    if(!data )
    {
        dev_priv->snr_ber = 0;
        dev_priv->snr_per = 0;

        return SUCCESS;
    }

    nim_s3821_read(dev, 0x0b, &data, 1);
    if(0 == (data & 0x80))
    {
        data_b[2] = data & 0x7f;
        nim_s3821_read(dev, 0x0c, &data_b[0], 2);

        data = 0x80;
        nim_s3821_write(dev, 0x0b, &data, 1);

        rec_ber = (UINT32)((data_b[2] << 16 | data_b[0] << 8 | data_b[1]) >> 1);
        dev_priv->snr_ber = rec_ber;
        if (ber_vld)
        {
            *ber_vld = 1;
        }

        if (m_vbber)
        {
            *m_vbber = rec_ber;
        }

        nim_s3821_read(dev, 0x0a, &data, 1);
        if(0x00 == (data & 0x80))
        {
            btemp = (data & 0x7f) << 8;
            nim_s3821_read(dev, 0x09, &data, 1);
            btemp |= data;

            data = 0x80;
            nim_s3821_write(dev, 0x0a, &data, 1);

            dev_priv->snr_per = btemp;

            if (m_per)
            {
                *m_per = btemp;
            }

            dev_priv->per_tot_cnt += 1;

        }

    }

    return SUCCESS;
}

INT32 nim_s3821_dvbt_isdbt_monitor_cnr(struct nim_device *dev, UINT32 *m_cnr)
{
    struct nim_s3821_private *dev_priv = NULL;

    UINT8 data = 0;
    UINT8 data_b[3] = {0};
    UINT32 btemp = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    if (m_cnr)
    {
        *m_cnr = 0;
    }


    nim_s3821_read(dev, 0x1d, &data, 1);
    if (0x00 == (data & 0x40)) // not lock.
    {
        dev_priv->cnr_info = 0;

        return SUCCESS;
    }

    //step 1, check HP status.(removed).

    //step 2. read out CCI info.
    nim_s3821_read(dev, 0x10b, data_b, 3);
    btemp = (UINT32)((data_b[0] << 16) | (data_b[1] << 8) | data_b[2]);

    //step 3. read out the TRANSMODE.
    nim_s3821_read(dev, 0x74, &data, 1);
    if (0 == (data & 0x30)) // 2k mode, need enlarge by 4.
    {
        btemp = btemp << 2;
    }

    //step 4. provide the CCI info to other program.
    dev_priv->cnr_info = btemp;

    if (m_cnr)
    {
        *m_cnr = btemp;
    }

    return SUCCESS;
}

INT32 nim_s3821_dvbt2_monitor_cnr(struct nim_device *dev, UINT32 *m_cnr)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 data = 0;
    UINT8 data_b[4] = {0};
    UINT32 btemp = 0;

    dev_priv = (struct nim_s3821_private *)dev->priv;
    if (m_cnr)
    {
        *m_cnr = 0;
    }

    nim_s3821_read(dev, 0x67, &data, 1);
    if((data & 0x0f) < 0x0a) // FSM still unlock.
    {
        dev_priv->cnr_info = 0;

        return SUCCESS;
    }

    //step 1~2. read out CCI info.
    nim_s3821_read(dev, 0x1a2, data_b, 4);
    //S3821_PRINTF(NIM_LOG_DBG,"CCI CR, 0x%2x, 0x%2x, 0x%2x, 0x%2x! \n", data_b[3], data_b[2], data_b[1], data_b[0]);
    btemp = (UINT32)(((data_b[3] & 0x3f) << 24) | (data_b[2] << 16) | (data_b[1] << 8) | data_b[0]);

    //step 3. read out the TRANSMODE.
    //joey, 20140604, for C3821, FFT mode position to CR60.
		//nim_s3821_read(dev, 0x65, &data, 1);
		//data = ((data & 0x70) >> 4);
		nim_s3821_read(dev, 0x60, &data, 1);
		data = (data&0x07);

    switch (data)
    {
        //because what ever the large number, the final normalize value of 32K is still 30bit.
        //we don't need the overflow protection.
    case 0: // 2k.
        btemp = btemp << 4;
        break;
    case 1:
    case 6: // 8k.
        btemp = btemp << 2;
        break;
    case 2: // 4k.
        btemp = btemp << 3;
        break;
    case 3: // 1k.
        btemp = btemp << 5;
        break;
    case 4:// 16k.
        btemp = btemp << 1;
        break;
    case 5:
    case 7:
    default:// 32k.
        btemp = btemp;
        break;
    }

    //step 4. provide the CCI info to other program.
    //dev_priv->cnr_info = btemp;

    if ((MAGIC_CONST_0 == btemp) && (dev_priv->cnr_info > MAGIC_CONST_1000)) // means olv value far from 0, the latest is 0.
    {
        if (dev_priv->flt_cci_info_cnt > MAGIC_CONST_6) // the 7th case, update.
        {
            dev_priv->cnr_info = btemp;
            dev_priv->flt_cci_info_cnt = 0;
        }
        else
        {
            dev_priv->flt_cci_info_cnt += 1;
        }
    }
    else
    {
        dev_priv->cnr_info = btemp;

        if (dev_priv->flt_cci_info_cnt > 0)
        {
            dev_priv->flt_cci_info_cnt = 0;
        }
    }

    if (m_cnr)
    {
        *m_cnr = btemp;
    }

    return SUCCESS;
}



/*****************************************************************************
* INT32 nim_s3821_get_t2_qpsk_cn(struct nim_device *dev, UINT8 *snr)
*
* This function returns an approximate estimation of the SNR from the NIM
*
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8 *snr
*
* Return Value: * snr
*****************************************************************************/
static UINT16 nim_s3821_get_t2_qpsk_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;

   const UINT32 tmp_arr_t2_0[20] = {0xffffffff, 526381, 453770, 389293, 335411, 280875, \
                                            234440, 187229, 145623, 106830, 74545, 47656, 27785, \
                                            14025, 6057, 2262, 665, 146, 28, 8
                                           };

    cn_est = 0;
    for (i = 0; i < 19; i++)
    {
        if ((cnr_info < tmp_arr_t2_0[i]) && (cnr_info >= tmp_arr_t2_0[i + 1]))
        {
            //cn_est = 5 + i*10;

            // calculate the step first.
            temp_step = (tmp_arr_t2_0[i] - tmp_arr_t2_0[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }

            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_t2_0[i + 1] + (j + 1)*temp_step)) &&
                (cnr_info >= (tmp_arr_t2_0[i + 1] + j * temp_step)))
                {
                    break;
                }
            }

            cn_est = 0 + i * 10 + (10 - j);

            break;
        }
    }

    if (MAGIC_CONST_19 == i)
    {
        cn_est = 0 + i * 10;
    }

    // for QPSK, because the start point is at "-1", so we need kick back the data.
    if (cn_est < MAGIC_CONST_10)
    {
        cn_est = 0;
    }
    else
    {
        cn_est = cn_est - 10;
    }

    return cn_est;

}

static UINT16 nim_s3821_get_t2_16qam_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;

    const UINT32 tmp_arr_t2_1[20] = {0xffffffff, 253352, 213038, 173256, 140614, 111205, \
                                            87787, 67545, 51265, 36377, 25119, 16413, 10027, 5482, \
                                            2637, 1105, 404, 115, 42, 12
                                           };


    cn_est = 0;
    for (i = 0; i < 19; i++)
    {
        if ((cnr_info < tmp_arr_t2_1[i]) && (cnr_info >= tmp_arr_t2_1[i + 1]))
        {
            //cn_est = 55 + i*10;

            // calculate the step first.
            temp_step = (tmp_arr_t2_1[i] - tmp_arr_t2_1[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }

            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_t2_1[i + 1] + (j + 1)*temp_step)) &&
              (cnr_info >= (tmp_arr_t2_1[i + 1] + j * temp_step)))
                {
                    break;
                }
            }

            cn_est = 30 + i * 10 + (10 - j);

            break;
        }
    }

    if (MAGIC_CONST_19 == i)
    {
        cn_est = 30 + i * 10;
    }

    return cn_est;
}


static UINT16 nim_s3821_get_t2_64qam_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;

    const UINT32 tmp_arr_t2_2[20] = {0xffffffff, 169944, 137826, 114151, 94623, 82212, \
                                            63268, 51199, 40480, 31534, 24129, 18152, 12776, \
                                            8638, 5833, 3582, 2106, 1216, 604, 261
                                           };

    cn_est = 0;
    for (i = 0; i < 19; i++)
    {
        if ((cnr_info < tmp_arr_t2_2[i]) && (cnr_info >= tmp_arr_t2_2[i + 1]))
        {
            //cn_est = 95 + i*10;

            // calculate the step first.
            temp_step = (tmp_arr_t2_2[i] - tmp_arr_t2_2[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }

            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_t2_2[i + 1] + (j + 1)*temp_step)) &&
            (cnr_info >= (tmp_arr_t2_2[i + 1] + j * temp_step)))
                {
                    break;
                }
            }

            cn_est = 80 + i * 10 + (10 - j);

            break;
        }
    }

    if (MAGIC_CONST_19 == i)
    {
        cn_est = 80 + i * 10;
    }

    return cn_est;
}

static UINT16 nim_s3821_get_t2_256qam_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;
    const UINT32 tmp_arr_t2_3[20] = {0xffffffff, 105106, 87298, 74143, 58529, 47356, 37399, \
                                            27783, 21309, 15734, 11389, 7811, 5242, 3377, 2062, \
                                            1165, 639, 347, 181, 91
                                           };


    cn_est = 0;
    for (i = 0; i < 19; i++)
    {
        if ((cnr_info < tmp_arr_t2_3[i]) && (cnr_info >= tmp_arr_t2_3[i + 1]))
        {
            //cn_est = 95 + i*10;

            // calculate the step first.
            temp_step = (tmp_arr_t2_3[i] - tmp_arr_t2_3[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }

            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_t2_3[i + 1] + (j + 1)*temp_step)) &&
               (cnr_info >= (tmp_arr_t2_3[i + 1] + j * temp_step)))
                {
                    break;
                }
            }

            cn_est = 120 + i * 10 + (10 - j);

            break;
        }
    }

    if (MAGIC_CONST_19 == i)
    {
        cn_est = 120 + i * 10;
    }

    return cn_est;
}

static INT32 nim_s3821_get_t2_snr_x(struct nim_device *dev, UINT8 *snr)
{
    struct nim_s3821_private *dev_priv = NULL;

    UINT8 tmp_cr = 0;
    UINT8 tmp_con = 0;
    UINT8 cn_lim = 0;
    UINT8 cn_ndg = 0;
    UINT16 cn_est = 0;
    UINT8 cnr_sqi = 0;
    UINT16 ber_sqi = 0;
    UINT8 data[2] = {0};
    UINT32 ber_tmp = 0;
    UINT32 tmp_cnr_info = dev_priv->cnr_info;

        //dvbt2 part.
    const UINT8 cci_lim_table_t2[4][6] = {{15, 27, 36, 46, 52, 57}, \
        {67, 81, 94, 105, 113, 120}, \
        {111, 129, 141, 156, 167, 173}, \
        {152, 175, 187, 207, 219, 227}
    };

    const UINT8 cci_ndg_table_t2[4][6] = {{35, 47, 56, 66, 72, 77}, \
        {87, 101, 114, 125, 133, 138}, \
        {130, 148, 162, 177, 187, 194}, \
        {170, 194, 208, 229, 243, 251}
    };

    dev_priv = (struct nim_s3821_private *)dev->priv;

    //step 1: get CON, and CR.
    nim_s3821_read(dev, 0x183, data, 1);

    tmp_cr = (data[0] & 0x07);
    tmp_con = (data[0] & 0x18) >> 3;

    cn_lim = cci_lim_table_t2[tmp_con][tmp_cr];
    cn_ndg = cci_ndg_table_t2[tmp_con][tmp_cr];

    //step 2: Estimate the CNR current input.
    switch (tmp_con)
    {

    case 0: // QPSK.
        cn_est = nim_s3821_get_t2_qpsk_cn(tmp_cnr_info);
        break;

    case 1: // 16Qam.
        cn_est = nim_s3821_get_t2_16qam_cn(tmp_cnr_info);
        break;

    case 2: // 64Qam.
        cn_est = nim_s3821_get_t2_64qam_cn(tmp_cnr_info);
        break;

    case 3: // 256Qam.
        cn_est = nim_s3821_get_t2_256qam_cn(tmp_cnr_info);
         break;
    default:
        break;
    }


    //step 3, calculate the cnr_sqi, with cn_est and cn_ndg.
    if ((cn_est + 30) < cn_ndg)
    {
        cnr_sqi = 0;
    }
    else if ((cn_ndg + 30) <= cn_est)
    {
        cnr_sqi = 100;
    }
    else
    {
        cnr_sqi = (UINT8)(30 + cn_est - cn_ndg);
    }

    //step 4, calculate the ber_sqi, with cn_est, cn_lim.
    ber_tmp = dev_priv->snr_ber;

    if (ber_tmp > MAGIC_CONST_1000) // >10^-4.
    {
        ber_sqi = 0;
    }
    else if (0 == ber_tmp) // <10^-7.
    {
        ber_sqi = 60;
    }
    else
    {
        ber_sqi = 150;
    }

    //step 5, combine the cnr_sqi and ber_sqi as final.
    if ((MAGIC_CONST_0 == cnr_sqi) || (MAGIC_CONST_100 == cnr_sqi))
    {
        dev_priv->rec_snr = cnr_sqi;
    }
    else
    {
        if (0 == ber_sqi) // because here 0 means not to dealing with the divide operation.
        {
            dev_priv->rec_snr = (UINT8)(ber_sqi * cnr_sqi);
        }
        else
        {
            dev_priv->rec_snr = (UINT8)((cnr_sqi * 100) / ber_sqi);
        }
    }

    //S3821_PRINTF(NIM_LOG_DBG,"CCI %d, cn_est %d, cn_lim %d, cn_ndg %d, cnr_sqi %d, ber_sqi %d, ber is %d, SQI: %d!\n",
//              tmp_cnr_info, cn_est, cn_lim, cn_ndg, cnr_sqi, ber_sqi, ber_tmp, dev_priv->rec_snr);


    *snr = dev_priv->rec_snr;
    return SUCCESS;

}


static UINT16 nim_s3821_get_t_qpsk_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;

    const UINT32 tmp_arr_0[19] = {0xffffffff, 535071, 470813, 399074, 331130, 259995, 196496, \
                                         138873, 90277, 53272, 27940, 12748, 4573, \
                                         1397, 340, 47, 10, 2, 0
                                        };
    cn_est = 0;
    for (i = 0; i < 18; i++)
    {
        if ((cnr_info < tmp_arr_0[i]) && (cnr_info >= tmp_arr_0[i + 1]))
        {
            //cn_est = 5 + i*10;
            // calculate the step first.
            temp_step = (tmp_arr_0[i] - tmp_arr_0[i + 1]) / 10;
            if (0 == temp_step)
            {
                temp_step = 1;
            }
            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_0[i + 1] + (j + 1)*temp_step)) &&
              (cnr_info >= (tmp_arr_0[i + 1] + j * temp_step)))
                {
                    break;
                }
            }
            cn_est = 0 + i * 10 + (10 - j);
            break;
        }
    }
    if (MAGIC_CONST_18 == i)
    {
        cn_est = 0 + i * 10;
    }

    return cn_est;

}

static UINT16 nim_s3821_get_t_16qam_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;
    const UINT32 tmp_arr_1[19] = {0xffffffff, 300619, 254915, 218796, 179845, 142051, 106777, \
                                         75606, 49948, 29305, 15743, 7223, 2896, \
                                         905, 242, 59, 17, 13, 6
                                        };

    cn_est = 0;
    for (i = 0; i < 18; i++)
    {
        if ((cnr_info < tmp_arr_1[i]) && (cnr_info >= tmp_arr_1[i + 1]))
        {
            //cn_est = 55 + i*10;
            // calculate the step first.
            temp_step = (tmp_arr_1[i] - tmp_arr_1[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }
            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_1[i + 1] + (j + 1)*temp_step)) &&
              (cnr_info >= (tmp_arr_1[i + 1] + j * temp_step)))
                {
                    break;
                }
            }
            cn_est = 50 + i * 10 + (10 - j);
            break;
        }
    }
    if (MAGIC_CONST_18 == i)
    {
        cn_est = 50 + i * 10;
    }

    return cn_est;
}

static UINT16 nim_s3821_get_t_64qam_cn(UINT32 cnr_info)
{
    UINT8 i = 0;
    UINT8 j = 0;
    UINT16 cn_est = 0;
    UINT32 temp_step = 0;

    const UINT32 tmp_arr_2[19] = {0xffffffff, 184437, 160595, 136860, 114410, 92540, 73271, \
                                         55176, 39019, 25661, 15613, 8460, 3973, \
                                         1609, 550, 151, 45, 19, 8
                                        };

    cn_est = 0;
    for (i = 0; i < 18; i++)
    {
        if ((cnr_info < tmp_arr_2[i]) && (cnr_info >= tmp_arr_2[i + 1]))
        {
            //cn_est = 95 + i*10;

            // calculate the step first.
            temp_step = (tmp_arr_2[i] - tmp_arr_2[i + 1]) / 10;

            if (0 == temp_step)
            {
                temp_step = 1;
            }

            for (j = 0; j < 10; j++) // to find out the 0.1 CNR step, 10-th.
            {
                if ((cnr_info < (tmp_arr_2[i + 1] + (j + 1)*temp_step)) &&
              (cnr_info >= (tmp_arr_2[i + 1] + j * temp_step)))
                {
                    break;
                }
            }

            cn_est = 90 + i * 10 + (10 - j);

            break;
        }
    }

    if (MAGIC_CONST_18 == i)
    {
        cn_est = 90 + i * 10;
    }

    return cn_est;
}
static INT32 nim_s3821_get_t_and_isdbt_snr_x(struct nim_device *dev, UINT8 *snr)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 tmp_cr = 0;
    UINT8 tmp_con = 0;
    UINT8 cn_lim = 0;
    UINT8 cn_ndg = 0;
    UINT16 cn_est = 0;

    UINT8 cnr_sqi = 0;
    UINT16 ber_sqi = 0;
    UINT8 data[2] = {0};
    UINT32 tmp_cnr_info = dev_priv->cnr_info;

       //dvbt part.
    const UINT8 cci_lim_table[3][5] = {{18, 35, 45, 55, 61}, \
        {72, 95, 107, 120, 129}, \
        {115, 146, 161, 176, 185}
    };

    const UINT8 cci_ndg_table[3][5] = {{51, 69, 79, 89, 97}, \
        {108, 131, 146, 156, 160}, \
        {165, 187, 202, 216, 225}
    };


    dev_priv = (struct nim_s3821_private *)dev->priv;
    //step 1: get CON, and CR.
    nim_s3821_read(dev, 0x73, data, 2);

    tmp_cr = (data[0] & 0x38) >> 3;
    tmp_con = (data[1] & 0x0c) >> 2;
    cn_lim = cci_lim_table[tmp_con][tmp_cr];
    cn_ndg = cci_ndg_table[tmp_con][tmp_cr];

    //step 2: Estimate the CNR current input.
    switch (tmp_con)
    {
    case 0: // QPSK.
        cn_est=nim_s3821_get_t_qpsk_cn(tmp_cnr_info);
        break;

    case 1: // 16Qam.
        cn_est=nim_s3821_get_t_16qam_cn(tmp_cnr_info);
        break;
    case 2: // 64Qam.
        cn_est=nim_s3821_get_t_64qam_cn(tmp_cnr_info);
        break;
    default:
        break;
    }


    //step 3, calculate the cnr_sqi, with cn_est and cn_ndg.
    if ((cn_est + 70) < cn_ndg)
    {
        cnr_sqi = 0;
    }
    else if ((cn_ndg + 30) < cn_est)
    {
        cnr_sqi = 100;
    }
    else
    {
        cnr_sqi = (UINT8)(70 + cn_est - cn_ndg);
    }

    //step 4, calculate the ber_sqi, with cn_est, cn_lim.
    if ((cn_est + 8) < cn_lim)
    {
        ber_sqi = 0;
    }
    else
    {
        ber_sqi = 19 * (cn_est + 8 - cn_lim);
    }

    if (ber_sqi < MAGIC_CONST_200) //because the cn related has enlarge by 10.
    {
        ber_sqi = 0;
    }
    else if (ber_sqi > MAGIC_CONST_1000)
    {
        ber_sqi = 1000;
    }

    //step 5, combine the cnr_sqi and ber_sqi as final.
    dev_priv->rec_snr = (UINT8)((ber_sqi * cnr_sqi) / 1000);

    //S3821_PRINTF(NIM_LOG_DBG,"CCI %d, cn_est %d, cn_lim %d, cn_ndg %d, cnr_sqi %d, ber_sqi %d, SQI: %d!\n",
//          tmp_cnr_info, cn_est, cn_lim, cn_ndg, cnr_sqi, ber_sqi, dev_priv->rec_snr);

    *snr = dev_priv->rec_snr;
    return SUCCESS;

}

static INT32 nim_s3821_get_snr_x(struct nim_device *dev, UINT8 *snr)
{
    INT32 ret = 0;
    UINT8 cur_mode = 0;

    nim_s3821_get_cur_mode(dev, &cur_mode);
    if (MODE_DVBT == cur_mode)
    {
        ret = nim_s3821_get_t_and_isdbt_snr_x(dev,snr);
    }
    else if (MODE_DVBT2 == cur_mode)
    {

        ret = nim_s3821_get_t2_snr_x(dev,snr);
    }
    return ret;
}
/*****************************************************************************
* INT32 nim_s3821_get_snr(struct nim_device *dev, UINT8 *snr)
*
* This function returns an approximate estimation of the SNR from the NIM
*
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8 *snr
*
* Return Value: * snr
*****************************************************************************/

INT32 nim_s3821_get_snr(struct nim_device *dev, UINT8 *snr)
{
	struct nim_s3821_private *dev_priv;
	dev_priv = (struct nim_s3821_private *)dev->priv;
	//OSAL_ER result;
	UINT32 	flgptn;
	UINT32  per_tmp;	
	UINT32  ber_tmp;
	UINT8 	data[2];
	UINT32  flag_ptn = 0;
	//result = osal_flag_wait(&flgptn,dev_priv->nim_flag, NIM_3821_SCAN_END, OSAL_TWF_ANDW,0);
	//if(OSAL_E_OK == result)
			
	flag_ptn = nim_flag_read(&dev_priv->flag_lock, NIM_3821_SCAN_END, OSAL_TWF_ANDW , 0);
	if((flag_ptn & NIM_3821_SCAN_END) && (flag_ptn != OSAL_INVALID_ID))	
	{
		//check lock first.
		nim_s3821_get_lock(dev, data);
		if(0 == data[0]) //unlock.
		{
			*snr = dev_priv->rec_snr = 0;
			return SUCCESS;
		}
//joey, 20130724, for DVB-T quality indicator.
#if 1
//dvbt part.
	static const UINT8 cci_lim_table[3][5] = {{18, 35, 45, 55, 61}, \
										{72, 95, 107, 120, 129}, \
										{115, 146, 161, 176, 185}};

	static const UINT8 cci_ndg_table[3][5] = {{51, 69, 79, 89, 97}, \
										{108, 131, 146, 156, 160}, \
										{165, 187, 202, 216, 225}};

	static const UINT32 tmp_arr_0[19] = {0xffffffff, 535071, 470813, 399074, 331130, 259995, 196496, \
										138873, 90277, 53272, 27940, 12748, 4573, \
										1397, 340, 47, 10, 2, 0};

	static const UINT32 tmp_arr_1[19] = {0xffffffff, 300619, 254915, 218796, 179845, 142051, 106777, \
										75606, 49948, 29305, 15743, 7223, 2896, \
										905, 242, 59, 17, 13, 6};

	static const UINT32 tmp_arr_2[19] = {0xffffffff, 184437, 160595, 136860, 114410, 92540, 73271, \
										55176, 39019, 25661, 15613, 8460, 3973, \
										1609, 550, 151, 45, 19, 8};

	//dvbt2 part.
	static const UINT8 cci_lim_table_t2[4][6] = {{15, 27, 36, 46, 52, 57}, \
											{67, 81, 94, 105, 113, 120}, \
											{111, 129, 141, 156, 167, 173}, \
											{152, 175, 187, 207, 219, 227}};

	static const UINT8 cci_ndg_table_t2[4][6] = {{35, 47, 56, 66, 72, 77}, \
											{87, 101, 114, 125, 133, 138}, \
											{130, 148, 162, 177, 187, 194}, \
											{170, 194, 208, 229, 243, 251}};

	static const UINT32 tmp_arr_t2_0[20] = {0xffffffff, 526381, 453770, 389293, 335411, 280875, \
											234440, 187229, 145623, 106830, 74545, 47656, 27785, \
											14025, 6057, 2262, 665, 146, 28, 8};

	static const UINT32 tmp_arr_t2_1[20] = {0xffffffff, 253352, 213038, 173256, 140614, 111205, \
											87787, 67545, 51265, 36377, 25119, 16413, 10027, 5482, \
											2637, 1105, 404, 115, 42, 12};

	static const UINT32 tmp_arr_t2_2[20] = {0xffffffff, 169944, 137826, 114151, 94623, 82212, \
											63268, 51199, 40480, 31534, 24129, 18152, 12776, \
											8638, 5833, 3582, 2106, 1216, 604, 261};

	static const UINT32 tmp_arr_t2_3[20] = {0xffffffff, 105106, 87298, 74143, 58529, 47356, 37399, \
											27783, 21309, 15734, 11389, 7811, 5242, 3377, 2062, \
											1165, 639, 347, 181, 91};

	UINT8 cur_mode = 0;
	UINT8 tmp_cr, tmp_con;
	UINT8 cn_lim, cn_ndg;
	UINT16 cn_est = 0;
	UINT8 i=0;
	UINT8 j=0;
	UINT8 cnr_sqi = 0;
	UINT16 ber_sqi = 0;
	UINT32 temp_step = 0;
	UINT32 tmp_cnr_info = dev_priv->cnr_info;
	
	nim_s3821_get_cur_mode(dev, &cur_mode);
	if (MODE_DVBT == cur_mode)
	{
		//step 1: get CON, and CR.		
		nim_s3821_read(dev, 0x73, data, 2);
		tmp_cr = (data[0] & 0x38) >> 3;
		tmp_con = (data[1] & 0x0c) >> 2;
		cn_lim = cci_lim_table[tmp_con][tmp_cr];
		cn_ndg = cci_ndg_table[tmp_con][tmp_cr];
		//step 2: Estimate the CNR current input.
		switch (tmp_con)
		{
			case 0: // QPSK.
				cn_est = 0;
				for (i=0; i<18; i++)
				{
					if ((tmp_cnr_info < tmp_arr_0[i]) && (tmp_cnr_info >= tmp_arr_0[i+1]))
					{
						//cn_est = 5 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_0[i] - tmp_arr_0[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_0[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_0[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 0 + i*10 + (10-j);
						break;
					}
				}
				if (18 == i)
				{
					cn_est = 0 + i*10;
				}
				break;
			case 1: // 16Qam.
				cn_est = 0;
				for (i=0; i<18; i++)
				{
					if ((tmp_cnr_info < tmp_arr_1[i]) && (tmp_cnr_info >= tmp_arr_1[i+1]))
					{
						//cn_est = 55 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_1[i] - tmp_arr_1[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_1[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_1[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 50 + i*10 + (10-j);
						break;
					}
				}
				if (18 == i)
				{
					cn_est = 50 + i*10;
				}
				break;
			case 2: // 64Qam.
				cn_est = 0;
				for (i=0; i<18; i++)
				{
					if ((tmp_cnr_info < tmp_arr_2[i]) && (tmp_cnr_info >= tmp_arr_2[i+1]))
					{
						//cn_est = 95 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_2[i] - tmp_arr_2[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_2[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_2[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 90 + i*10 + (10-j);
						break;
					}
				}
				if (18 == i)
				{
					cn_est = 90 + i*10;
				}
				break;
			default:
				break;

		}
//step 3, calculate the cnr_sqi, with cn_est and cn_ndg.
		if ((cn_est+70) < cn_ndg)
		{
			cnr_sqi = 0;
		}
		else if ((cn_ndg + 30) < cn_est)
		{
			cnr_sqi = 100;
		}
		else
		{
			cnr_sqi = (UINT8)(70+ cn_est - cn_ndg);	
		}
		//step 4, calculate the ber_sqi, with cn_est, cn_lim.
		if ((cn_est+8) < cn_lim)
		{
			ber_sqi = 0;
		}
		else 
		{
			ber_sqi = 19 * (cn_est + 8 - cn_lim);
		}
		if (ber_sqi < 200) //because the cn related has enlarge by 10.
		{	
			ber_sqi = 0;
		}
		else if (ber_sqi > 1000)
		{
			ber_sqi = 1000;
		}
		//step 5, combine the cnr_sqi and ber_sqi as final.
		dev_priv->rec_snr = (UINT8)((ber_sqi*cnr_sqi)/1000);
		//NIM_S3821_DEBUG("CCI %d, cn_est %d, cn_lim %d, cn_ndg %d, cnr_sqi %d, ber_sqi %d, SQI: %d!\n", tmp_cnr_info, cn_est, cn_lim, cn_ndg, cnr_sqi, ber_sqi, dev_priv->rec_snr);
		*snr = dev_priv->rec_snr;
		return SUCCESS;
	}
	else if (MODE_DVBT2 == cur_mode)
	{
		//step 1: get CON, and CR.		

		nim_s3821_read(dev, 0x183, data, 1);
		tmp_cr = (data[0] & 0x07);
		tmp_con = (data[0] & 0x18) >> 3;
		cn_lim = cci_lim_table_t2[tmp_con][tmp_cr];
		cn_ndg = cci_ndg_table_t2[tmp_con][tmp_cr];
		//step 2: Estimate the CNR current input.
		switch (tmp_con)
		{
			case 0: // QPSK.
				cn_est = 0;
				for (i=0; i<19; i++)
				{
					if ((tmp_cnr_info < tmp_arr_t2_0[i]) && (tmp_cnr_info >= tmp_arr_t2_0[i+1]))
					{
						//cn_est = 5 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_t2_0[i] - tmp_arr_t2_0[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_t2_0[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_t2_0[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 0 + i*10 + (10-j);
						break;
					}
				}
				if (19 == i)
				{
					cn_est = 0 + i*10;
				}
				// for QPSK, because the start point is at "-1", so we need kick back the data.
				if (cn_est < 10)
				{
					cn_est = 0;
				}
				else
				{
					cn_est = cn_est - 10;
				}
				break;
			case 1: // 16Qam.
				cn_est = 0;
				for (i=0; i<19; i++)
				{
					if ((tmp_cnr_info < tmp_arr_t2_1[i]) && (tmp_cnr_info >= tmp_arr_t2_1[i+1]))
					{
						//cn_est = 55 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_t2_1[i] - tmp_arr_t2_1[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_t2_1[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_t2_1[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 30 + i*10 + (10-j);
						break;
					}

				}
				if (19 == i)
				{
					cn_est = 30 + i*10;
				}
				break;
			case 2: // 64Qam.
				cn_est = 0;
				for (i=0; i<19; i++)
				{
					if ((tmp_cnr_info < tmp_arr_t2_2[i]) && (tmp_cnr_info >= tmp_arr_t2_2[i+1]))
					{
						//cn_est = 95 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_t2_2[i] - tmp_arr_t2_2[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_t2_2[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_t2_2[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 80 + i*10 + (10-j);
						break;
					}
				}
				if (19 == i)
				{
					cn_est = 80 + i*10;
				}
				break;
			case 3: // 256Qam.
				cn_est = 0;
				for (i=0; i<19; i++)
				{
					if ((tmp_cnr_info < tmp_arr_t2_3[i]) && (tmp_cnr_info >= tmp_arr_t2_3[i+1]))
					{
						//cn_est = 95 + i*10;
						// calculate the step first.
						temp_step = (tmp_arr_t2_3[i] - tmp_arr_t2_3[i+1])/10;
						if (0 == temp_step)
						{
							temp_step = 1;
						}
						for (j=0; j<10; j++) // to find out the 0.1 CNR step, 10-th.
						{
							if ((tmp_cnr_info < (tmp_arr_t2_3[i+1]+(j+1)*temp_step)) && (tmp_cnr_info >= (tmp_arr_t2_3[i+1]+j*temp_step)))
							{
								break;
							}
						}
						cn_est = 120 + i*10 + (10-j);
						break;
					}
				}
				if (19 == i)
				{
					cn_est = 120 + i*10;
				}
				break;
			default:
				break;
		}
		//step 3, calculate the cnr_sqi, with cn_est and cn_ndg.
		if ((cn_est+30) < cn_ndg)
		{
			cnr_sqi = 0;
		}
		else if ((cn_ndg + 30) <= cn_est)
		{
			cnr_sqi = 100;
		}
		else
		{
			cnr_sqi = (UINT8)(30+ cn_est - cn_ndg);	
		}
		//step 4, calculate the ber_sqi, with cn_est, cn_lim.
		ber_tmp = dev_priv->snr_ber;
		if (ber_tmp > 1000) // >10^-4.
		{
			ber_sqi = 0;
		}
		else if (0 == ber_tmp) // <10^-7.
		{
			ber_sqi = 60;
		}
		else
		{
			ber_sqi = 150;
		}
		//step 5, combine the cnr_sqi and ber_sqi as final.
		if ((0 == cnr_sqi) || (100 == cnr_sqi))
		{
			dev_priv->rec_snr = cnr_sqi;
		}
		else
		{
			if (0 == ber_sqi) // because here 0 means not to dealing with the divide operation.
			{
				dev_priv->rec_snr = (UINT8)(ber_sqi*cnr_sqi);
			}
			else
			{
				dev_priv->rec_snr = (UINT8)((cnr_sqi*100)/ber_sqi);
			}
		}
		//NIM_S3821_DEBUG("CCI %d, cn_est %d, cn_lim %d, cn_ndg %d, cnr_sqi %d, ber_sqi %d, ber is %d, SQI: %d!\n", tmp_cnr_info, cn_est, cn_lim, cn_ndg, cnr_sqi, ber_sqi, ber_tmp, dev_priv->rec_snr);
		*snr = dev_priv->rec_snr;
		return SUCCESS;	

	}
#endif
		ber_tmp = dev_priv->snr_ber;
		per_tmp = dev_priv->snr_per;
		if (per_tmp >= 1500) //if exist Per, we just ref it.
		{
			dev_priv->rec_snr = 25;
		}
		else if (per_tmp > 500) //snr range in 5~40
		{
			dev_priv->rec_snr = (UINT8)(35 - (10 * (per_tmp-500)/(1500-500)));
		}
		else if (per_tmp > 0) //snr range in 5~40
		{
			dev_priv->rec_snr = (UINT8)(55 - (20 * per_tmp/500));
		}
		else // per_tmp == 0; // in ths case, we use ber to take out snr.
		{
			if (ber_tmp >= 8000) // snr = 40;
			{
				dev_priv->rec_snr = 56;
			}
			else //ber == 0 ber <2000, means signal is good.
			{
				dev_priv->rec_snr = (UINT8)(97 - (41 * ber_tmp / 8000));
			}
		}
	}
	else
    {
        if(dev_priv->log_en)
        {
            S3821_PRINTF(NIM_LOG_DBG,"Get SNR Fail, Wait ChannelChg Complete!\n");
        }
    }
	*snr = dev_priv->rec_snr;
	return SUCCESS;

}

/*****************************************************************************
* INT32 nim_s3821_monitor_ber(struct nim_device *dev, UINT32 *RsUbc)
* Reed Solomon Uncorrected block count
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT32* RsUbc
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3821_monitor_ber(struct nim_device *dev, UINT8 *ber_vld, UINT32 *m_vbber, UINT32 *m_per)
{
    struct nim_s3821_private *dev_priv = NULL;
    UINT8 data = 0;

    UINT8 data_b[3] = {0};
    UINT32 btemp = 0;
    UINT32 rec_ber = 0;


    dev_priv = (struct nim_s3821_private *)dev->priv;
    if (ber_vld)
    {
        *ber_vld = 0;
    }
    if (m_vbber)
    {
        *m_vbber = 0;
    }
    if (m_per)
    {
        *m_per = 0;
    }

    nim_s3821_read(dev, 0x67, &data, 1);
    if(data < MAGIC_CONST_0X0A )
    {
        dev_priv->snr_ber = 0;
        dev_priv->snr_per = 0;
        return SUCCESS;
    }

    nim_s3821_read(dev, 0x1b3, &data, 1);
    if(0 == (data & 0x80))
    {
        data_b[1] = data & 0x7f;
        nim_s3821_read(dev, 0x1b4, &data_b[0], 1);
        data = 0x80;
        nim_s3821_write(dev, 0x1b3, &data, 1);
        rec_ber = (UINT32)(data_b[1] << 8 | data_b[0]);
        dev_priv->snr_ber = rec_ber;
        if (ber_vld)
        {
            *ber_vld = 1;
        }

        if (m_vbber)
        {
            *m_vbber = rec_ber;
        }

        nim_s3821_read(dev, 0x1b2, &data, 1);
        if(0x00 == (data & 0x80))
        {
            btemp = (data & 0x7f) << 8;
            nim_s3821_read(dev, 0x1b1, &data, 1);
            btemp |= data;
            data = 0x80;
            nim_s3821_write(dev, 0x1b2, &data, 1);
            dev_priv->snr_per = btemp;
            if (m_per)
            {
                *m_per = btemp;
            }

            dev_priv->per_tot_cnt += 1;
        }
    }
    return SUCCESS;

}





INT32 nim_s3821_log_ifft_result_once(void)
{
    if (mon_ifft_en == FALSE)
    {
        mon_ifft_en = TRUE;
    }
    return SUCCESS;
}


INT32 nim_s3821_init_config(struct nim_device *dev)
{
    struct nim_s3821_private *priv_mem = (struct nim_s3821_private *)(dev->priv);

    UINT8 data = 0;
    UINT8 cur_mode = 0;

    //check the demod type.
    //this must be done firstly, because this will affect the memory space mapping.

    //set the demod to idle.
    data = 0x80;
    nim_s3821_write(dev, 0x00, &data, 1);

    //joey, 20130715. for adc2dma capture function, init here.
    nim_s3821_init_adc2dma(dev);

    if (ISDBT_TYPE == priv_mem->cofdm_type)
    {
        //set demod to ISDB-T mode.
        cur_mode = MODE_ISDBT;
        nim_s3821_set_cur_mode(dev, &cur_mode);
        priv_mem->cur_type = ISDBT_TYPE;

        nim_s3821_init_dvbt_isdbt(dev);

    }
    else if (DVBT_TYPE == priv_mem->cofdm_type)
    {
        //set demod to DVB-T mode.
        cur_mode = MODE_DVBT;
        nim_s3821_set_cur_mode(dev, &cur_mode);
        priv_mem->cur_type = DVBT_TYPE;

        nim_s3821_init_dvbt_isdbt(dev);
    }
    else if (DVBT2_TYPE == priv_mem->cofdm_type)
    {
        //set demod to DVB-T2 mode.
        cur_mode = MODE_DVBT2;
        nim_s3821_set_cur_mode(dev, &cur_mode);
        priv_mem->cur_type = DVBT2_TYPE;

        nim_s3821_init_dvbt2(dev);
    }
    else if (DVBT2_COMBO == priv_mem->cofdm_type) //in this type, the cur_type need set to DVBT or DVBT2.
    {
        //set demod to DVB-T mode. firstly.
        cur_mode = MODE_DVBT;
        nim_s3821_set_cur_mode(dev, &cur_mode);
        priv_mem->cur_type = DVBT_TYPE;

        nim_s3821_init_dvbt_isdbt(dev);

        //set demod to DVB-T2 mode. secondly
        cur_mode = MODE_DVBT2;
        nim_s3821_set_cur_mode(dev, &cur_mode);
        priv_mem->cur_type = DVBT2_TYPE;

        nim_s3821_init_dvbt2(dev);

    }
    else
    {
        S3821_PRINTF(NIM_LOG_DBG,"Unknow work type setting!\n");
       // ASSERT(0);
        return ERR_FAILUE;
    }

    return SUCCESS;
}



void nim_s3821_proc_test_mux(UINT8 tmp_dat)
{

/*	static UINT8 test_mux_st = 0; // disable.
	static UINT32 reg_494 = 0;
	static UINT32 reg_490 = 0;
	static UINT32 reg_074 = 0;
*/	
	UINT32 data = 0;

	if ((0 == test_mux_st) && (0x01 == (tmp_dat&0x01)))

	{

		//enable test_mux;

		reg_494=(*(volatile UINT32 *)(0xB8000494)); 
		//data = (reg_494&0xfff0ffff) | 0x01080001;
		data = 0x80000001;
		*(volatile UINT32 *)(0xB8000494) = data; 

		reg_490=(*(volatile UINT32 *)(0xB8000490)); 
		data = (reg_490 & 0xffffffc0) | 0x00001000;
		*(volatile UINT32 *)(0xB8000490) = data; 

//joey, 20140313, for C3821, no use 438.
/*
		reg_438=(*(volatile UINT32 *)(0xB8000438)); 
		data = reg_438 & 0xffffefff;
		*(volatile UINT32 *)(0xB8000438) = data; 
*/
		reg_074=(*(volatile UINT32 *)(0xB8000074)); 
		data = reg_074 | 0x00010000;
		*(volatile UINT32 *)(0xB8000074) = data; 

		//record the state.

		test_mux_st = 1;

	}

	else if ((1 == test_mux_st) && (0x00 == (tmp_dat&0x01)))

	{

		//disable test_mux;

		*(volatile UINT32 *)0xb8000494 = reg_494;

		//comm_delay(10);
		comm_sleep(10);


		*(volatile UINT32 *)0xb8000490 = reg_490;

		//comm_delay(10);
		comm_sleep(10);


//joey, 20140313, for C3821, no use 438.
		//*(volatile UINT32 *)0xb8000438 = reg_438;
		*(volatile UINT32 *)0xb8000074 = reg_074;

		//comm_delay(10);
		comm_sleep(10);
		//record the state.

		test_mux_st = 0;

	}

	return;
}

//joey, 20130927, for T2 FFT_GAIN jump cause burst mosaic issue.

INT32 nim_s3821_cci_rm_proc(struct nim_device *dev)
{
	UINT8 data_0 = 0;
	UINT8 data_1 = 0;
	static UINT8 cci_track_flag = 0;

	nim_s3821_read(dev, 0x6e, &data_0, 1);
	nim_s3821_read(dev, 0x9f, &data_1, 1);

	if ((0x00 == (data_0 & 0x10)) && (0x02 == (data_1 & 0x02))) // ACI detect. 6e[4] = 0, and 9f[1] = 1.
	{
		if (0 == cci_track_flag) // FFT_GAIN freeze stage.
		{
		//disable RG_CCI_GAIN_FREEZE. bit6->0. Let the tracking function.
			nim_s3821_read(dev, 0x71, &data_0, 1);
			data_0 = data_0 & 0xbf;
			nim_s3821_write(dev, 0x71, &data_0, 1);
			cci_track_flag = 1;
		}
	}
	else
	{
		if (1 == cci_track_flag) // FFT_GAIN release stage.
		{
		//enable RG_CCI_GAIN_FREEZE. bit6->1. Disable tracking function.
			nim_s3821_read(dev, 0x71, &data_0, 1);
			data_0 = data_0 | 0x40;
			nim_s3821_write(dev, 0x71, &data_0, 1);

			cci_track_flag = 0;
		}
	}

	return SUCCESS;
}
//joey, 20140123, for T2 without common PLP TS, DJB overflow issue.
INT32 nim_s3821_djb_busy_proc(struct nim_device *dev)
{
	UINT8 data_0 = 0;
	UINT8 data_1 = 0;
	UINT8 tmp_data = 0;
	//joey, 20150119, for SPLP FEF case DJB mosaic issue.
	UINT8 data[3];
	static UINT8 fec_clk_moved = 0;
	//check if S3821 COFDM or not.
	nim_s3821_read(dev, 0x1d2, &tmp_data, 1);
	//Branch of S3821, use the patch.
	if (0 == tmp_data)
	{
		nim_s3821_read(dev, 0x67, &tmp_data, 1);
		if((tmp_data&0x0f) < 0x0a) // FSM still unlock.
		{
			if (1 == fec_clk_moved) // means FEC clock has been changed. change to default.
			{
				nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
				tmp_data += 1;
				nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

				fec_clk_moved = 0;
			}
		}
		else
		{
			nim_s3821_read(dev, 0x15e, &data_0, 1);
			data_0 &= 0x10;

			nim_s3821_read(dev, 0x170, &data_1, 1);
			if ((0 == data_0) && (data_1 > 1)) // means without c-cplp, and plp number is larger than 1.
			{
				if (0 == fec_clk_moved)
				{
					nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
					tmp_data -= 1;
					nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

					//reset FEC part.
					nim_s3821_read(dev, 0x66, &tmp_data, 1);
					tmp_data += 1;
					nim_s3821_write(dev, 0x66, &tmp_data, 1);

					nim_s3821_read(dev, 0x66, &tmp_data, 1);
					tmp_data -= 1;
					nim_s3821_write(dev, 0x66, &tmp_data, 1);

					fec_clk_moved = 1;
				}
			}
			else //means not special case, back to default.
			{
				if (1 == fec_clk_moved)
				{
					nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
					tmp_data += 1;
					nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

					//reset FEC part.
					nim_s3821_read(dev, 0x66, &tmp_data, 1);
					tmp_data += 1;
					nim_s3821_write(dev, 0x66, &tmp_data, 1);

					nim_s3821_read(dev, 0x66, &tmp_data, 1);
					tmp_data -= 1;
					nim_s3821_write(dev, 0x66, &tmp_data, 1);

					fec_clk_moved = 0;
				}
			}
		}
	}
	else //Branch of C3821, use the patch. //joey, 20150202, for SPLP FEF case DJB mosaic issue.
	{
		nim_s3821_get_dvbt2_lock(dev, &tmp_data);
		if(0 == tmp_data) // FSM unlock.
		{
			if (1 == fec_clk_moved) // already triggered.
			{
				nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
				tmp_data = (UINT8)((tmp_data&0xe0)|0x06);
				nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

				fec_clk_moved = 0;
				S3821_PRINTF(NIM_LOG_DBG,"DJB patch to unlock!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
		}
		else // FSM locked.
		{
			nim_s3821_read(dev, 0x178, data, 3);
			data[0] = data[0] | data[1] | data[2]; // if not a "zero" value, means FEF.

			nim_s3821_read(dev, 0x185, &tmp_data, 1);
			if ((data[0] > 0) && (1 == tmp_data)) // FEF mode and SPLP.
			{
				if (0 == fec_clk_moved) // not triggered.
				{
					nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
					tmp_data = (UINT8)((tmp_data&0xe0)|0x05);
					nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

					fec_clk_moved = 1;
					S3821_PRINTF(NIM_LOG_DBG,"DJB patch to ON!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				}
			}
			else //means not special case, back to default.
			{
				if (1 == fec_clk_moved) // not triggered.
				{
					nim_s3821_read(dev, 0x1b8, &tmp_data, 1);
					tmp_data = (UINT8)((tmp_data&0xe0)|0x06);
					nim_s3821_write(dev, 0x1b8, &tmp_data, 1);

					fec_clk_moved = 0;
					S3821_PRINTF(NIM_LOG_DBG,"DJB patch to OFFFF!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				}
			}
		}
	}
	return SUCCESS;
}


//joey, 20140220, for T2 C3821 version.
INT32 nim_s3821_check_rev_id(struct nim_device *dev, UINT8 *rev_id)
{
	UINT8 tmp_data = 0;

//check if S3821 COFDM or not.
	nim_s3821_read(dev, 0x1d2, &tmp_data, 1);
	//Branch of S3821, use the patch.
	if (0 == tmp_data)
	{
		*rev_id = 0;
	}
	else
	{
		*rev_id = 1;
	}


	return SUCCESS;
}
//joey, 20140804, for T2 moscow dynamic small cci proc.
INT32 nim_s3821_dvbt2_dyn_cci_proc(struct nim_device *dev, UINT8 cci_per_vld, UINT32 cci_m_per)
{
	static UINT8 dvbt2_dyn_cci_flag = 0; // Setting cci pre on or patch triggered.
	static UINT8 dvbt2_cci_reg_lock = 1; // Setting: 0: back to unlock state. 1: now is in lock or eqaul lock stage.
	static UINT8 dvbt2_cci_unuse_cnt = 0; // Used for pass the first 2 PER value.
	//static UINT32 dvbt2_cci_per_chg = 0; // PER just before CCI setting update.
	//static UINT8 dvbt2_cci_pass_cnt = 0; // Trigger stage used for pass the first 5 PER value.
	
	//joey, 20160503, for Vietnam 506M large PER issue. 
	static UINT8 dvbt2_cci_large_per_cnt = 0; // Used for continuous large PER.
	static UINT8 dvbt2_cci_man_on = 0; // Used for man_on.
	static UINT8 dvbt2_cci_man_val_fst = 0; // Used for manual value only set once.

	UINT8 data = 0;
	UINT8 dvbt2_cci_to_unlock = 0;
	UINT8 dvbt2_cci_to_pre_on = 0;
	UINT8 dvbt2_cci_to_trigger = 0;
	UINT8 tmp_nf_bypass = 0;
	UINT8 tmp_plp_mod = 0;
	UINT8 tmp_data[4];
	UINT16 mo_cci_abn_max = 0;
	UINT16 mo_cci_abn_mean = 0;
	static UINT32 mo_cci_abn_1_pre = 0; // To record the previous abnromal number_1.
	UINT32 mo_cci_abn_1_cur = 0; // Current abnormal number_1.

	//joey, 20160503, for Vietnam 506M large PER issue.
	UINT8 tmp_reg12 = 0; 
	UINT8 tmp_reg174 = 0; 
	UINT8 tmp_reg177 = 0; 
	UINT8 tmp_reg170 = 0; 
	UINT8 tmp_reg183 = 0; 
	UINT8 tmp_reg1ad = 0; 

	//joey, 20160503, for Vietnam 506M large PER issue.
	struct nim_s3821_private *dev_priv;
	dev_priv = (struct nim_s3821_private *)dev->priv;

	nim_s3821_read(dev,0x67,&data,1);
	
	//joey, 20160503, for Vietnam 506M large PER issue.
	if ((data&0x0f) >= 0x0a) // FMS is locked.
	{
		nim_s3821_read(dev, 0x12, &tmp_reg12, 1);
		nim_s3821_read(dev, 0x174, &tmp_reg174, 1);
		nim_s3821_read(dev, 0x177, &tmp_reg177, 1);
		nim_s3821_read(dev, 0x170, &tmp_reg170, 1);
		nim_s3821_read(dev, 0x183, &tmp_reg183, 1);
		nim_s3821_read(dev, 0x1ad, &tmp_reg1ad, 1);
	}

	// a)	frequency=506MHz
	// c) 	8M bandwidth.
	
	// d)	MO_P1_S2=8 ' 0x12[7:4]=8
	// e)	MO_L1_GI_MODE=1 ' 0x174[5:3]=1
	// f)		MO_PILOT_PT=3 ' 0x177[6:4]=3
	// g)	MO_NUM_PLP=1 ' 0x170[7:0]=1
	// h)	MO_CPLP_ON=0, MO_PLP_FEC_TYPE=1, MO_PLP_ROT=1, MO_PLP_MOD=2, MO_PLP_COD=5 '0x183[7:0]=0x75
	// i)		MO_CCI_POST_FIND_FLG=0, MO_CCI_POST_FIND_FLG1=0, MO_CCI_POST_FIND_FLG2=0, MO_CCI_POST_FIND_FLG3=1 ' 0x1ad[6:3]=8

  if (((dev_priv->s3821_cur_channel_info.frequency >= 505500) && (dev_priv->s3821_cur_channel_info.frequency <= 506500)) \
  	&& (8 == dev_priv->s3821_cur_channel_info.channel_bw) \
  	&& (0x80 == (tmp_reg12 & 0xf0)) && (0x08 == (tmp_reg174 & 0x38)) && (0x30 == (tmp_reg177 & 0x70)) \
  	&& (0x01 == tmp_reg170) && (0x75 == tmp_reg183) && (0x40 == (tmp_reg1ad & 0x78)) \
  	&& ((data&0x0f) >= 0x0a))
  {
  	if (0 == dvbt2_cci_man_on) // not trigger yet
	{
	  	if (dvbt2_cci_large_per_cnt < 2)
	  	{
			// b)	Detect PER>20 twice successively 
	  		if (dev_priv->snr_per > 20)
	  		{
	  			dvbt2_cci_large_per_cnt += 1;
	  		}
			else
			{
	  			dvbt2_cci_large_per_cnt = 0;
			}
	  	}
			else
			{
				if (0 == dvbt2_cci_man_val_fst)
				{
					//0x197[6:0], 0x196[7:0]) = 0x33C2  (RG_CCI_POST_HIGH_BND=13250)
					data = 0x2c;
					nim_s3821_write(dev, 0x196, &data, 1);
					data = 0x33;
					nim_s3821_write(dev, 0x197, &data, 1);
	
					//0x199[6:0], 0x198[7:0]) = 0x31B0  (RG_CCI_POST_LOW_BND=12720)
					data = 0xb0;
					nim_s3821_write(dev, 0x198, &data, 1);
	
					nim_s3821_read(dev, 0x199, &data, 1); // include the manu-cci on.
					data = (data&0x80) | 0x31;
					nim_s3821_write(dev, 0x199, &data, 1); // include the manu-cci on.
	
					dvbt2_cci_man_val_fst = 1;
				}
				
				// 0x199[7] = 1 (RG_CCI_POST_MANU_ON=1)
				nim_s3821_read(dev, 0x199, &data, 1);
				data |= 0x80;
				nim_s3821_write(dev, 0x199, &data, 1);
				
				dvbt2_cci_reg_lock = 1;
				dvbt2_cci_man_on = 1;
				S3821_PRINTF(NIM_LOG_DBG, "Vietnam 506M case happen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
		}
  }
  else
  {
		nim_s3821_read(dev,0x67,&data,1);
		if ((data&0x0f) < 0x0a) // unlock branch.
		{
			if (1 == dvbt2_cci_reg_lock)
			{
				dvbt2_cci_to_unlock = 1; // set register to unlock stage.
				dvbt2_cci_reg_lock = 0;
				S3821_PRINTF(NIM_LOG_DBG, "patch to unlock!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
	
			if (1 == dvbt2_dyn_cci_flag)
			{
				dvbt2_dyn_cci_flag = 0;
			}
			if (dvbt2_cci_unuse_cnt >0)
			{
				dvbt2_cci_unuse_cnt = 0;
			}
	
			//joey, 20160503, for Vietnam 506M large PER issue.
			if (1 == dvbt2_cci_man_on)
			{
				dvbt2_cci_man_on = 0;
			}
			if (dvbt2_cci_large_per_cnt >0)
			{
				dvbt2_cci_large_per_cnt = 0;
			}
		}
		else // lock branch.
		{
			if (0 == dvbt2_cci_reg_lock) // record that register has changed to lock setting(CCI_PRE on / waiting trigger/or triggerd.)
			{
				dvbt2_cci_reg_lock = 1;
			}
			if (0 == dvbt2_dyn_cci_flag) // not trigger yet.
			{
				nim_s3821_read(dev,0x6e,&data,1);
				tmp_nf_bypass = (data & 0x10) >> 4;
	
				if (0 == tmp_nf_bypass) // means CCI pre on. also treat as another patch stage trigger.
				{
					dvbt2_cci_to_pre_on = 1;
					dvbt2_dyn_cci_flag = 1;
					S3821_PRINTF(NIM_LOG_DBG, "patch to CCI pre on!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				}
				else
				{
					if (dvbt2_cci_unuse_cnt <= 1)
					{
						if (1 == dvbt2_cci_unuse_cnt)
						{
							nim_s3821_read(dev,0x19e,tmp_data,4);
							mo_cci_abn_1_pre = (UINT32)(((tmp_data[3]&0x7f)<<24) |((tmp_data[2])<<16) | ((tmp_data[1])<<8) |tmp_data[0]);						
						}
						dvbt2_cci_unuse_cnt += 1;
					}
					else
					{
						//joey, 20141027, for DVB-T2 SQI cal update.
						nim_s3821_read(dev,0x1aa,tmp_data,2);
						mo_cci_abn_max = (UINT16)(((tmp_data[1]&0x07)<<8) |tmp_data[0]);
	
						nim_s3821_read(dev,0x1ac,tmp_data,2);
						mo_cci_abn_mean = (UINT16)(((tmp_data[1]&0x07)<<8) |tmp_data[0]);						
							
						nim_s3821_read(dev,0x6e,&data,1);
						tmp_nf_bypass = (data & 0x10) >> 4;
	
						nim_s3821_read(dev,0x183,&data,1);
						tmp_plp_mod = (data & 0x18) >> 3;
	
						nim_s3821_read(dev,0x19e,tmp_data,4);
						mo_cci_abn_1_cur = (UINT32)(((tmp_data[3]&0x7f)<<24) |((tmp_data[2])<<16) | ((tmp_data[1])<<8) |tmp_data[0]);					
						
						if (((mo_cci_abn_1_pre < 2000) && (mo_cci_abn_1_cur > 6000)) && (1 == tmp_nf_bypass) && (tmp_plp_mod > 1) \
							&& ((mo_cci_abn_max > (4*mo_cci_abn_mean)) && (mo_cci_abn_mean > 20)))
						{
							//joey, 20141028, for DVB-T2 SQI cal update, only check once trigger.
							dvbt2_cci_to_trigger = 1;
							dvbt2_dyn_cci_flag = 1;
							S3821_PRINTF(NIM_LOG_DBG, "patch to assert!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
						
						}
						mo_cci_abn_1_pre = mo_cci_abn_1_cur;
					}	
				}
			}
		}
		
		if (1 == dvbt2_cci_to_unlock)
		{
			//0x18b: 0x61 -> 0x6d
			//0x18f: 0xc9 -> 0x49
			//0x191: 0x51 -> 0x31
			                   
			//0x187: 0x85 -> 0x89
			//0x189: 0x89 -> 0xa1
			//0x18d: 0x45 -> 0x49
			                   
			//0x18a: 0x80 -> 0x31
			//0x18c: 0x80 -> 0x36
			//0x192: 0x80 -> 0x54
			//0x194: 0x80 -> 0x4a
			data = 0x6d;
			nim_s3821_write(dev, 0x18b, &data, 1);
			data = 0x49;
			nim_s3821_write(dev, 0x18f, &data, 1);
			data = 0x31;
			nim_s3821_write(dev, 0x191, &data, 1);
		
			data = 0x89;
			nim_s3821_write(dev, 0x187, &data, 1);
			data = 0xa1;
			nim_s3821_write(dev, 0x189, &data, 1);
			data = 0x49;
			nim_s3821_write(dev, 0x18d, &data, 1);
		
			data = 0x31;
			nim_s3821_write(dev, 0x18a, &data, 1);
			data = 0x36;
			nim_s3821_write(dev, 0x18c, &data, 1);
			data = 0x54;
			nim_s3821_write(dev, 0x192, &data, 1);
			data = 0x4a;
			nim_s3821_write(dev, 0x194, &data, 1);
		
			//fts part. CR30 = 0x0a, CR31 = 0x0a.
			data = 0x0a;
			nim_s3821_write(dev, 0x30, &data, 1);
		
			data = 0x0a;
			nim_s3821_write(dev, 0x31, &data, 1);
		
			//joey, 20160503, for Vietnam 506M large PER issue. Clear the manu-cci mode, CR199[7] = 0.
			nim_s3821_read(dev, 0x199, &data, 1);
			data &= 0x7f;
			nim_s3821_write(dev, 0x199, &data, 1);
		}
		
		if (1 == dvbt2_cci_to_pre_on)
		{
			//0x18b: 0x61 -> 0x6d
			//0x18f: 0xc9 -> 0x49
			//0x191: 0x51 -> 0x31
			                   
			//0x187: 0x85 -> 0x89
			//0x189: 0x89 -> 0xa1
			//0x18d: 0x45 -> 0x49
			                   
			//0x18a: 0x80 -> 0x31
			//0x18c: 0x80 -> 0x36
			//0x192: 0x80 -> 0x54
			//0x194: 0x80 -> 0x4a
		
			data = 0x6d;
			nim_s3821_write(dev, 0x18b, &data, 1);
			data = 0x49;
			nim_s3821_write(dev, 0x18f, &data, 1);
			data = 0x31;
			nim_s3821_write(dev, 0x191, &data, 1);
		
			data = 0x89;
			nim_s3821_write(dev, 0x187, &data, 1);
			data = 0xa1;
			nim_s3821_write(dev, 0x189, &data, 1);
			data = 0x49;
			nim_s3821_write(dev, 0x18d, &data, 1);
		
			data = 0x31;
			nim_s3821_write(dev, 0x18a, &data, 1);
			data = 0x36;
			nim_s3821_write(dev, 0x18c, &data, 1);
			data = 0x54;
			nim_s3821_write(dev, 0x192, &data, 1);
			data = 0x4a;
			nim_s3821_write(dev, 0x194, &data, 1);			
		}
		
		if (1 == dvbt2_cci_to_trigger)
		{
			//fts part. CR30 = 0x04, CR31 = 0x07.
			data = 0x04;
			nim_s3821_write(dev, 0x30, &data, 1);
		
			data = 0x07;
			nim_s3821_write(dev, 0x31, &data, 1);
						
			//0x18f: 0x49 -> 0xc9
			//0x191: 0x31 -> 0x51
			
			//0x187: 0x89 -> 0x85
			//0x189: 0xa1 -> 0x89
			//0x18d: 0x49 -> 0x45
			
			//0x18a: 0x31 -> 0x80
			//0x18c: 0x36 -> 0x80
			//0x192: 0x54 -> 0x80
			//0x194: 0x4a -> 0x80
			//0x18b: 0x6d -> 0x61
		
			data = 0xc9;
			nim_s3821_write(dev, 0x18f, &data, 1);
			data = 0x51;
			nim_s3821_write(dev, 0x191, &data, 1);
		
			data = 0x85;
			nim_s3821_write(dev, 0x187, &data, 1);
			data = 0x89;
			nim_s3821_write(dev, 0x189, &data, 1);
			data = 0x45;
			nim_s3821_write(dev, 0x18d, &data, 1);
		
			data = 0x80;
			nim_s3821_write(dev, 0x18a, &data, 1);
			data = 0x80;
			nim_s3821_write(dev, 0x18c, &data, 1);
			data = 0x80;
			nim_s3821_write(dev, 0x192, &data, 1);
			data = 0x80;
			nim_s3821_write(dev, 0x194, &data, 1);
		
			data = 0x61;
			nim_s3821_write(dev, 0x18b, &data, 1);			
		}
	}
	return SUCCESS;
}

//joey, 20150521, for Vietnam chan_type selection processing. 
INT32 nim_s3821_chan_type_proc(struct nim_device *dev)
{
	static UINT8 dvbt2_cr121_reg_chg = 0; // Setting: 0: default value. 1: now is modified value.

	UINT8 data = 0;

	nim_s3821_read(dev,0x67,&data,1);
	if ((data&0x0f) < 0x0a) // unlock branch.
	{
		if (1 == dvbt2_cr121_reg_chg)
		{
			nim_s3821_read(dev, 0x121, &data, 1);
			data = (data & 0xf8) | 0x04;
			nim_s3821_write(dev, 0x121, &data, 1);

			dvbt2_cr121_reg_chg = 0;
		}
	}
	else
	{
		if (0 == dvbt2_cr121_reg_chg)
		{
			nim_s3821_read(dev, 0x127, &data, 1);
			{
				if (0x02 == (data & 0x03)) // means chan_type == 2.
				{
					nim_s3821_read(dev, 0x121, &data, 1);
					data = (data & 0xf8) | 0x05;
					nim_s3821_write(dev, 0x121, &data, 1);

					dvbt2_cr121_reg_chg = 1;
				}
			}
		}
	}

	return SUCCESS;
}