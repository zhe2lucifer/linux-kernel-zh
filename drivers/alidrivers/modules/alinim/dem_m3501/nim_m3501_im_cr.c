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


#define  TDATA_START      7
#define  SYM_3000 3000
#define  SYM_40000 40000
#define  PACKET_NUM_662 662
#define  FFT_VAL_127 127
#define  FFT_VAL_128 128
#define  CODE_RATE_4 4
#define  CRYSTAL_FREQ_15000 15000
#define  MAP_TYPE_3 3
#define  MAP_TYPE_5 5
#define  GET_SNR_LEN 16
#define  SUM_VALUE_255 255

static UINT8 S3501ID[10] =
{
    0x47, 0x55, 0x33, 0x35, 0x30, 0x31, 0X44, 0x41, 0x54, 0x41
};


static UINT8 flag0 = 1;
static UINT8 flag1 = 0;

/*****************************************************************************
* INT32 nim_s3501_get_ldpc(struct nim_device *dev, UINT32 *rsubc)
* Get LDPC average iteration number
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* rsubc
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_ldpc(struct nim_device *dev, UINT32 *rsubc)
{
    UINT8 data = 0;

    if((dev == NULL) || (rsubc == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, RAA_S2_FEC_ITER, &data, 1);
    *rsubc = (UINT32) data;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_tdata(struct nim_device *dev, UINT8 *snr)
*
* This function returns an approximate estimation of the SNR from the NIM
*  The Eb No is calculated using the SNR from the NIM, using the formula:
*     Eb ~     13312- M_SNR_H
*     -- =    ----------------  dB.
*     NO           683
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* rsubc
*
* Return Value: INT32
*****************************************************************************/
#define	SIGNAL_INDICATOR 0
UINT32 nim_s3501_get_tdata(UINT8 data)
{
    UINT32 tdata = 0;

#if (SIGNAL_INDICATOR == 3)
    tdata = 0;
    if (0x01 == (data & 0x01)) //agc1
    {
    tdata += 2;
    }

    if (0x02 == (data & 0x02)) //agc2
    {
    tdata += 10;
    }

    if (0x04 == (data & 0x04)) //cr
    {
    tdata += 16;
    }

    if (0x08 == (data & 0x08)) //tr
    {
    tdata += 16;
    }

    if (0x10 == (data & 0x10)) //frame sync
    {
    tdata += 20;
     }

    if (0x20 == (data & 0x20))
    {
    tdata += 26;
    }
    if (0x40 == (data & 0x40))
    {
    tdata += 26;
    }

    if (0x80 == (data & 0x80))
    {
    tdata += 26;
    }
#else
    tdata = TDATA_START;
    if (0x01 == (data & 0x01))
    {
    tdata += 1;
     }

    if (0x02 == (data & 0x02))
    {
    tdata += 3;
    }

    if (0x04 == (data & 0x04))
    {
    tdata += 3;
     }

    if (0x08 == (data & 0x08))
    {
    tdata += 2;
    }

    if (0x10 == (data & 0x10))
    {
    tdata += 2;
     }

    if (0x20 == (data & 0x20))
    {
    tdata += 2;
     }

    if (0x40 == (data & 0x40))
    {
    tdata += 2;
    }

    if (0x80 == (data & 0x80))
    {
    tdata += 2;
    }
#endif

    return tdata;

}

void nim_s3501_set_snr_status(struct nim_device *dev,UINT8 lock,UINT32 sym_rate)
{
    UINT8 data = 0;
	struct nim_s3501_private *priv =NULL;
	
    if(dev == NULL)
    {
	    return;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    if((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) && (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501B))
    {
        if((lock) && (sym_rate <= SYM_3000))
        {
            data = 0x42;
            nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
            data = 0x8a;
            nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
        }
        else if ((lock) && (priv->t_param.t_phase_noise_detected == 0) &&
              (priv->t_param.phase_noise_detect_finish == 1))
        {
            if (priv->t_param.t_snr_state == 0)
            {
                priv->t_param.t_snr_state = 1;
                data = 0x52;
                nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                data = 0xba;
                nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                S3501_PRINTF(NIM_LOG_DBG,"@@@@initial snr state = 1, reg37 = 0x52, regb5 = 0xba;\n");
            }
            if ((priv->t_param.t_snr_state == 1) && (priv->t_param.t_aver_snr > priv->t_param.t_snr_thre1))
            {
                data = 0x4e;
                nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                data = 0xaa;
                nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                S3501_PRINTF(NIM_LOG_DBG,"snr state = 2, reg37 = 0x4e, regb5 = 0xaa;\n");
                priv->t_param.t_snr_state = 2;
            }
            else if (priv->t_param.t_snr_state == 2)
            {
                if (priv->t_param.t_aver_snr > priv->t_param.t_snr_thre2)
                {
                    data = 0x42;
                    nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                    data = 0x9a;
                    nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                    S3501_PRINTF(NIM_LOG_DBG,"snr state = 3, reg37 = 0x42, regb5 = 0x9a;\n");
                    priv->t_param.t_snr_state = 3;
                }
                else if (priv->t_param.t_aver_snr < (priv->t_param.t_snr_thre1 - 5))
                {
                    data = 0x52;
                    nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                    data = 0xba;
                    nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                    S3501_PRINTF(NIM_LOG_DBG,"snr state = 1, reg37 = 0x52, regb5 = 0xba;\n");
                    priv->t_param.t_snr_state = 1;
                }
            }
            else if (priv->t_param.t_snr_state == 3)
            {
                if (priv->t_param.t_aver_snr > priv->t_param.t_snr_thre3)
                {
                    data = 0x3e;
                    nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                    data = 0x8a;
                    nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                    S3501_PRINTF(NIM_LOG_DBG,"snr state = 4, reg37 = 0x3e, regb5 = 0x8a;\n");
                    priv->t_param.t_snr_state = 4;
                }
                else if (priv->t_param.t_aver_snr < (priv->t_param.t_snr_thre2 - 5))
                {
                    data = 0x4e;
                    nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                    data = 0xaa;
                    nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                    S3501_PRINTF(NIM_LOG_DBG,"snr state = 2, reg37 = 0x4e, regb5 = 0xaa;\n");
                    priv->t_param.t_snr_state = 2;
                }
            }
            else if (priv->t_param.t_snr_state == 4)
            {
                if (priv->t_param.t_aver_snr < (priv->t_param.t_snr_thre3 - 5))
                {
                    data = 0x42;
                    nim_reg_write(dev, R33_CR_CTRL + 0x04, &data, 1);
                    data = 0x9a;
                    nim_reg_write(dev, RB5_CR_PRS_TRA, &data, 1);
                    S3501_PRINTF(NIM_LOG_DBG,"snr state = 3, reg37 = 0x42, regb5 = 0x9a;\n");
                    priv->t_param.t_snr_state = 3;
                }
            }
        }
    }
}

INT32 nim_s3501_get_bypass_buffer(struct nim_device *dev)
{
    // According to DMX IC spec, bypass buffer must locate in 8MB memory segment.
    // Bypass buffer size is (BYPASS_BUFFER_REG_SIZE * 188)
	//#define BYPASS_BUF_SIZE 0x20000     // 128 KB
	//#define BYPASS_BUF_MASK (BYPASS_BUF_SIZE - 1)

    UINT32 tmp = 0;
	struct nim_s3501_private *priv = NULL;
    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    
	priv = (struct nim_s3501_private *) dev->priv;

	//malloc in module init to solve malloc failure
	//priv->ul_status.adcdata_malloc_addr = (UINT8 *) comm_malloc(BYPASS_BUF_SIZE * 2);

    if (priv->ul_status.adcdata_malloc_addr == NULL)
    {
        return ERR_NO_MEM;
    }  

    S3501_PRINTF(NIM_LOG_DBG,"ADCdata=0x%08x\n", (unsigned int)priv->ul_status.adcdata_malloc_addr);

    comm_memset(priv->ul_status.adcdata_malloc_addr, 0, BYPASS_BUF_SIZE * 2);
    tmp = ((UINT32) (priv->ul_status.adcdata_malloc_addr)) & BYPASS_BUF_MASK;
    if (tmp)
    {
        priv->ul_status.adcdata = priv->ul_status.adcdata_malloc_addr + BYPASS_BUF_SIZE - tmp;
    }
    else
    {
        priv->ul_status.adcdata = priv->ul_status.adcdata_malloc_addr;
    }

    return SUCCESS;
}

INT32 nim_s3501_get_fft_result(struct nim_device *dev, UINT32 freq, UINT32 *start_adr)
{
    INT32 i = 0;
    INT32 index = 0;
    INT32 ave_energy = 0;
    INT32  max_est = 0;
    INT32 maxvalue = 0;

    UINT8 data = 0x10;
    UINT8 lock = 200;
	INT32 channel_freq_err = 0;
    struct nim_s3501_private *dev_priv= NULL;

    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	

    dev_priv = dev->priv;

    if (NULL == start_adr)
    {
        return ERR_ADDRESS;
    }
    //reset first
    nim_s3501_demod_ctrl(dev, NIM_DEMOD_CTRL_0X90);

    data = 0x35;
    nim_reg_write(dev, R0A_AGC1_LCK_CMD, &data, 1);

    if (nim_s3501_i2c_open(dev))
    {
        return S3501_ERR_I2C_NO_ACK;
    }
	
	if (dev_priv->nim_tuner_control != NULL)
	{		
		channel_freq_err = 0;
		if (dev_priv->is_m3031 && (dev_priv->nim_tuner_command != NULL))
		{
			dev_priv->nim_tuner_command(dev_priv->tuner_index, NIM_TUNER_SET_M3031_FREQ_ERR, &channel_freq_err);
		}
		
		dev_priv->nim_tuner_control(dev_priv->tuner_index, freq, 0);
	}

	comm_sleep(10);

	if (dev_priv->nim_tuner_status != NULL)
	{
    	dev_priv->nim_tuner_status(dev_priv->tuner_index, &lock);
	}
	
    if (nim_s3501_i2c_close(dev))
    {
        return S3501_ERR_I2C_NO_ACK;
    }

    nim_s3501_calculate_energy(dev);
    nim_s3501_smoothfilter();

    comm_memcpy(start_adr, fft_energy_1024, sizeof(fft_energy_1024));

    start_adr[511] = (start_adr[510] + start_adr[512]) / 2;
    i = 0;
    ave_energy = 0;
    max_est = 0;
    for (index = 256; index <= 767; index++)
    {
        ave_energy += start_adr[index];
        if (i == (1 << (STATISTIC_LENGTH + 1)) - 1)
        {
            if (ave_energy > max_est)
            {
               max_est = ave_energy;
            }
            i = 0;
            ave_energy = 0;
        }
        else
        {
            i++;
        }
    }
    max_est = max_est / 8;
    maxvalue = max_est;

    for (index = 256; index <= 767; index++)
    {
        if ((start_adr[index] > (UINT32) maxvalue) && (start_adr[index] < (UINT32) (max_est * 2)))
        {
            maxvalue = start_adr[index];
        }
    }

    for (index = 256; index <= 767; index++)
    {
        if (start_adr[index] > (UINT32) maxvalue)
        {
           start_adr[index] = maxvalue;
        }
        start_adr[index] = start_adr[index] * 250 / maxvalue;
    }
    return SUCCESS;
}

INT32 nim_s3501_get_type(struct nim_device *dev)
{
    UINT8 temp[4] =
    {
        0x00, 0x00, 0x00, 0x00
    };
    UINT32 m_value = 0x00;
	struct nim_s3501_private *priv =NULL;
	
    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    priv->ul_status.m_s3501_type = 0x00;
    nim_reg_read(dev, RA3_CHIP_ID, temp, 4);
    m_value = temp[1];
    m_value = (m_value << 8) | temp[0];
    m_value = (m_value << 8) | temp[3];
    m_value = (m_value << 8) | temp[2];
    priv->ul_status.m_s3501_type = m_value;
    nim_reg_read(dev, RAB_CHIP_SUB_ID, temp, 1);
    if(temp[0] != 0xcc)
    {
        nim_reg_read(dev, RC0_BIST_LDPC_REG, temp, 1);
    }
    priv->ul_status.m_s3501_sub_type = temp[0];

#ifdef C3501C_NEW_TSO
    priv->ul_status.m_tso_mode = 1;
#else
    priv->ul_status.m_tso_mode = 0;
#endif

    return SUCCESS;
}

INT32 nim_s3501_reg_get_freqoffset(struct nim_device *dev)
{
    INT32 freq_off = 0;
    UINT8 data[3] = {0};
    UINT32 tdata = 0;
    UINT32 temp = 0;
    UINT32  sample_rate = 0;

    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    nim_s3501_get_dsp_clk(dev, &sample_rate);
    temp = 0;
    nim_reg_read(dev, R69_RPT_CARRIER, data, 3);

    tdata = (data[0] & 0xff) + ((data[1] & 0xff) << 8);

    if ((data[2] & 0x01) == 1)
    {
        temp = (tdata ^ 0xffff) + 1;
     }
    else
    {
        temp = tdata & 0xffff;
     }

    if ((data[2] & 0x01) == 1)
    {
        freq_off = 0 - nim_s3501_multu64div(temp, sample_rate, 92160000); // 92160000 == 90000*1024
    }
    else
    {
        freq_off = nim_s3501_multu64div(temp, sample_rate, 92160000);
    }
    return freq_off;
}


INT32 nim_s3501_reg_get_modcod(struct nim_device *dev, UINT8 *modcod)
{
    UINT8 data = 0;

    if(dev == NULL || modcod == NULL)
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, RF8_MODCOD_RPT, &data, 1);
    *modcod = data & 0x7f;
    return SUCCESS;
}


INT32 nim_s3501_reg_get_map_type(struct nim_device *dev, UINT8 *map_type)
{
    UINT8 data = 0;

    if((dev == NULL) || (map_type == NULL))
	{
        return RET_FAILURE;
	}	
    nim_reg_read(dev, R69_RPT_CARRIER + 0x02, &data, 1);
    *map_type = ((data >> 5) & 0x07);
    return SUCCESS;

    //  Map type:
    //      0x0:    HBCD.
    //      0x1:    BPSK
    //      0x2:    QPSK
    //      0x3:    8PSK
    //      0x4:    16APSK
    //      0x5:    32APSK
}




INT32 nim_s3501_reg_get_iqswap_flag(struct nim_device *dev, UINT8 *iqswap_flag)
{
    UINT8 data = 0;

    if((dev == NULL) || (iqswap_flag == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x02, &data, 1);
    *iqswap_flag = ((data >> 4) & 0x01);
    return SUCCESS;
}

INT32 nim_s3501_reg_get_roll_off(struct nim_device *dev, UINT8 *roll_off)
{
    UINT8 data = 0;

    if((dev == NULL) || (roll_off == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, R6C_RPT_SYM_RATE + 0x02, &data, 1);
    *roll_off = ((data >> 5) & 0x03);
    return SUCCESS;

    //  DVBS2 Roll off report
    //      0x0:    0.35
    //      0x1:    0.25
    //      0x2:    0.20
    //      0x3:    Reserved
}



INT32 nim_s3501_reg_get_work_mode(struct nim_device *dev, UINT8 *work_mode)
{
    UINT8 data = 0;

    if((dev == NULL) || (work_mode == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, R68_WORK_MODE, &data, 1);
    *work_mode = data & 0x03;
    return SUCCESS;

    //  Work Mode
    //      0x0:    DVB-S
    //      0x1:    DVB-S2
    //      0x2:    DVB-S2 HBC
}



UINT8 nim_s3501_get_crnum(struct nim_device *dev)
{
    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    return ((struct nim_s3501_private *) (dev->priv))->ul_status.m_crnum;
}



INT32 nim_s3501_tuner_lock(struct nim_device *dev, UINT8 *tun_lock)
{
	struct nim_s3501_private *priv = NULL;
    if((dev == NULL) || (tun_lock == NULL))
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

	if (nim_s3501_i2c_open(dev))
	{
		return S3501_ERR_I2C_NO_ACK;
	}
	
    /* Setup tuner */
    NIM_MUTEX_ENTER(priv);
	if (priv->nim_tuner_status != NULL)
	{
    	priv->nim_tuner_status(priv->tuner_index, tun_lock);
	}
    NIM_MUTEX_LEAVE(priv);

	if (nim_s3501_i2c_close(dev))
	{
		return S3501_ERR_I2C_NO_ACK;
	}

    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_freq(struct nim_device *dev, UINT32 *freq)
* Read S3501 frequence
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16 *sym_rate         : Symbol rate in kHz
*
* Return Value: void
*****************************************************************************/
INT32 nim_s3501_get_freq(struct nim_device *dev, UINT32 *freq)
{
    if((dev == NULL) || (freq == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_s3501_reg_get_freq(dev, freq);
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
* Read S3501 symbol rate
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16 *sym_rate         : Symbol rate in kHz
*
* Return Value: void
*****************************************************************************/
INT32 nim_s3501_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
{
    if((dev == NULL) || (sym_rate == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_s3501_reg_get_symbol_rate(dev, sym_rate);
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_code_rate(struct nim_device *dev, UINT8* code_rate)
* Description: Read S3501 code rate
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT8* code_rate
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_code_rate(struct nim_device *dev, UINT8 *code_rate)
{
    if((dev == NULL) || (code_rate == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_s3501_reg_get_code_rate(dev, code_rate);
    return SUCCESS;
}


INT32 nim_s3501_get_tune_freq(struct nim_device *dev, INT32 *freq)
{
    struct nim_s3501_private *priv =NULL;
	
    if((dev == NULL) || (freq == NULL))
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    *freq += priv->ul_status.m_step_freq;
    if (*freq <= 0)
    {
        *freq = 30;
    }
    else if (*freq > 70)
    {
        *freq = 70;
    }

    return SUCCESS;
}



/*****************************************************************************
* INT32 nim_s3501_get_lock(struct nim_device *dev, UINT8 *lock)
*
*  Read FEC lock status
*
*Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: BOOL *fec_lock
*
*Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_lock(struct nim_device *dev, UINT8 *lock)
{
    UINT8 data[3] = {0};
    UINT8 rdata = 0;
    UINT8 h8psk_lock = 0;
    struct nim_s3501_private *priv =NULL;
	
    if((dev == NULL) || (lock == NULL))
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    comm_delay(150);
    nim_reg_read(dev, R02_IERR, data, 3);


    if ((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) &&
         ( (priv->tsk_status.m_lock_flag != NIM_LOCK_STUS_NORMAL)))
    {
        if((priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501C) && (priv->ul_status.m_tso_mode == 1))
        {
            if((data[0] & 0x02) != 0)
            {
                nim_m3501c_fec_ts_off(dev);
                nim_m3501c_reset_tso(dev);
                nim_m3501c_fec_ts_on(dev);
                nim_reg_read(dev, R02_IERR, &rdata, 1);
                rdata = rdata & 0xfd;
                nim_reg_write(dev, R02_IERR, &rdata, 1);
                S3501_PRINTF(NIM_LOG_DBG,"            Unlock and reset tso \n");
            }
        }
    }

    if ((data[2] & 0x80) == 0x80)
    {
        h8psk_lock = 1;
    }
    else
    {
        h8psk_lock = 0;
    }
    if ((h8psk_lock & ((data[2] & 0x2f) == 0x2f)) || ((h8psk_lock == 0) && ((data[2] & 0x3f) == 0x3f)))
    {
        *lock = 1;
        if ((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) &&
        ( (priv->tsk_status.m_lock_flag != NIM_LOCK_STUS_NORMAL)))
        {
            if((priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501C) && (priv->ul_status.m_tso_mode == 1))
            {
                if((priv->ul_status.m_tso_status != NIM_TSO_STUS_LOCK))
                {
#ifdef C3501C_ERRJ_LOCK
                    nim_m3501c_recover_moerrj(dev);
#endif
                    priv->ul_status.m_tso_status = NIM_TSO_STUS_LOCK;
                }
            }
        }
    }
    else
    {
        *lock = 0;
        if ((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) &&
           ( (priv->tsk_status.m_lock_flag != NIM_LOCK_STUS_NORMAL)))
        {
            if((priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501C) &&
                (priv->ul_status.m_tso_mode == 1))
            {
                if(priv->ul_status.m_tso_status != NIM_TSO_STUS_SETTING)
                {
#ifdef C3501C_ERRJ_LOCK
                    nim_m3501c_invert_moerrj(dev);
#endif
                    priv->ul_status.m_tso_status = NIM_TSO_STUS_SETTING;
                }
            }
        }
    }
    comm_delay(150);
    return SUCCESS;
}


/*****************************************************************************
* INT32 nim_s3501_get_ber(struct nim_device *dev, UINT32 *rsubc)
* Get bit error ratio
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* rsubc
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_ber(struct nim_device *dev, UINT32 *rsubc)
{
    UINT8 data = 0;
    UINT8 ber_data[3]={0};
    UINT32 u32ber_data[3]={0};
    UINT32 uber_data = 0;
    struct nim_s3501_private *priv =NULL;
	
    if(dev == NULL || rsubc == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    //CR78
    nim_reg_read(dev, R76_BIT_ERR + 0x02, &data, 1);
    if (0x00 == (0x80 & data))
    {
        //CR76
        nim_reg_read(dev, R76_BIT_ERR, &ber_data[0], 1);
        u32ber_data[0] = (UINT32) ber_data[0];
        //CR77
        nim_reg_read(dev, R76_BIT_ERR + 0x01, &ber_data[1], 1);
        u32ber_data[1] = (UINT32) ber_data[1];
        u32ber_data[1] <<= 8;
        //CR78
        nim_reg_read(dev, R76_BIT_ERR + 0x02, &ber_data[2], 1);
        u32ber_data[2] = (UINT32) ber_data[2];
        u32ber_data[2] <<= 16;

        uber_data = u32ber_data[2] + u32ber_data[1] + u32ber_data[0];

        uber_data *= 100;
        uber_data /= 1632;
        *rsubc = uber_data;
        priv->ul_status.old_ber = uber_data;
        //CR78
        data = 0x80;
        nim_reg_write(dev, R76_BIT_ERR + 0x02, &data, 1);
    }
    else
    {
        *rsubc = priv->ul_status.old_ber;
    }

    return SUCCESS;
}


/*****************************************************************************
* INT32 nim_s3501_get_snr_db(struct nim_device *dev, INT32 *snr_db)
* Get cn value
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: INT32 *snr_db
*  uint :0.01db
* Return Value: INT32
*****************************************************************************/

INT32 nim_s3501_get_snr_db(struct nim_device *dev, INT32 *snr_db)
{
    UINT8 lock_status = 0, map_type = 0, work_mode = 0;
    UINT32 snr = 0;
    UINT8 data = 0;
    INT32 total_iter = 0;
    INT32 sum = 0;
    UINT32 tdata = 0;
    UINT32 iter_num = 0;
    UINT32 i = 0;
	struct nim_s3501_private *priv = NULL;
	
    // For calc snr db
    UINT8 base_snr = 0;
    UINT8 base_snr_db = 0;
    UINT8 base_step = 0;
    UINT8 base_step_acc = 0;
    UINT8 step_polar = 0;    
    
    if((dev == NULL) || (snr_db == NULL))
        return RET_FAILURE;

    //struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;//clean warning
    priv = (struct nim_s3501_private *) dev->priv;

    nim_s3501_get_lock(dev, &lock_status);

    if (1 == lock_status)
    {
        // Get snr data
        sum = 0;
        total_iter = 0;
        for (i = 0; i < GET_SNR_LEN; i++)
        {
            nim_reg_read(dev, R45_CR_LCK_DETECT + 0x01, &data, 1);
            tdata = data << 8;
            nim_reg_read(dev, R45_CR_LCK_DETECT, &data, 1);
            tdata |= data;

            if (tdata & 0x8000)
            {
                tdata = 0x10000 - tdata;
            }

            nim_s3501_get_ldpc(dev, &iter_num);
            total_iter += iter_num;
            tdata >>= 5;
            sum += tdata;
        }
        sum /= GET_SNR_LEN;
        total_iter /= GET_SNR_LEN;
        sum *= 3;
        sum /= 5;
        if (sum > SUM_VALUE_255)
        {
        sum = 255;
        }

        if (priv->t_param.t_last_snr == -1)
        {
            snr = sum;
            priv->t_param.t_last_snr = snr;
        }
        else
        {
            if (total_iter == priv->t_param.t_last_iter)
            {
                snr = priv->t_param.t_last_snr;
                if (sum > priv->t_param.t_last_snr)
                {
                     priv->t_param.t_last_snr ++;
                }
                else if (sum < priv->t_param.t_last_snr)
                {
                    priv->t_param.t_last_snr --;
                }
            }
            else if ((total_iter > priv->t_param.t_last_iter) && (sum < priv->t_param.t_last_snr))
            {
                snr = sum;
                priv->t_param.t_last_snr = sum;
            }
            else if ((total_iter < priv->t_param.t_last_iter) && (sum > priv->t_param.t_last_snr))
            {
                snr = sum;
                priv->t_param.t_last_snr = sum;
            }
            else
            {
                snr = priv->t_param.t_last_snr;
            }
        }
        priv->t_param.t_last_iter = total_iter;

        S3501_PRINTF(NIM_LOG_DBG,"snr before *100 = %d\n", snr);
        // For unit 0.01dB
        snr *= 100;
        S3501_PRINTF(NIM_LOG_DBG,"snr after *100 = %d\n", snr);

    
        //  Work Mode
        //      0x0:    DVB-S
        //      0x1:    DVB-S2
        //      0x2:    DVB-S2 HBC    
        nim_s3501_reg_get_work_mode(dev, &work_mode);

        if (0 == work_mode)
        {
            S3501_PRINTF(NIM_LOG_DBG,"Get snr dB in dvb-s mode\n");
            
            base_snr = 20;
            base_snr_db = 2;
            base_step = 7;
            base_step_acc = 1;
            step_polar = 0;
        
            if (snr <= 2000)
            {
                *snr_db = 200;
            }
            else if (snr <= 2700)
            {
                *snr_db = 200 + (snr - 2000)/7;
            }
            else if (snr <= 4300)
            {
                *snr_db = 300 + (snr - 2700)/8;
            }
            else if (snr <= 5200)
            {
                *snr_db = 500 + (snr - 4300)/9;
            }
            else if (snr <= 6800)
            {
                *snr_db = 700 + (snr - 5200)/8;
            }
            else if (snr <= 7500)
            {
                *snr_db = 800 + (snr - 6800)/7;
            }
            else if (snr <= 8100)
            {
                *snr_db = 900 + (snr - 7500)/6;
            }
            else if (snr <= 8600)
            {
                *snr_db = 1000 + (snr - 8100)/5;
            }
            else if (snr <= 9000)
            {
                *snr_db = 1100 + (snr - 8600)/4;
            }
            else if (snr <= 9300)
            {
                *snr_db = 1200 + (snr - 9000)/3;
            }
            else if (snr <= 9400)
            {
                *snr_db = 1300 + (snr - 9300)/2;
            }
            else
                *snr_db = 1400;
        }
        else
        {
            //  Map type:
            //      0x0:    HBCD.
            //      0x1:    BPSK
            //      0x2:    QPSK
            //      0x3:    8PSK
            //      0x4:    16APSK
            //      0x5:    32APSK
            nim_s3501_reg_get_map_type(dev, &map_type);

            switch (map_type)
            {
             case 2:
                 S3501_PRINTF(NIM_LOG_DBG,"Get snr dB in dvb-s2 qpsk mode\n");
                 
                 base_snr = 20;
                 base_snr_db = 2;
                 base_step = 7;
                 base_step_acc = 1;
                 step_polar = 0;
                 
                 if (snr <= 1000)
                 {
                     *snr_db = -200;
                 }
                 else if (snr <= 1200)
                 {
                     *snr_db = -200 + (snr - 1000)/2;
                 }
                 else if (snr <= 1500)
                 {
                     *snr_db = -100 + (snr - 1200)/3;
                 }
                 else if (snr <= 1900)
                 {
                     *snr_db = 0 + (snr - 1500)/4;
                 }
                 else if (snr <= 2400)
                 {
                     *snr_db = 100 + (snr - 1900)/5;
                 }
                 else if (snr <= 3000)
                 {
                     *snr_db = 200 + (snr - 2400)/6;
                 }
                 else if (snr <= 3700)
                 {
                     *snr_db = 300 + (snr - 3000)/7;
                 }
                 else if (snr <= 6900)
                 {
                     *snr_db = 400 + (snr - 3700)/8;
                 }
                 else if (snr <= 7600)
                 {
                     *snr_db = 8000 + (snr - 6900)/7;
                 }
                 else if (snr <= 8200)
                 {
                     *snr_db = 900 + (snr - 7600)/6;
                 }
                 else if (snr <= 8700)
                 {
                     *snr_db = 1000 + (snr - 8200)/5;
                 }
                 else if (snr <= 9100)
                 {
                     *snr_db = 1100 + (snr - 8700)/4;
                 }
                 else if (snr <= 9400)
                 {
                     *snr_db = 1200 + (snr - 9100)/3;
                 }
                 else if (snr <= 9600)
                 {
                     *snr_db = 1300 + (snr - 9400)/2;
                 }
                 else if (snr <= 9800)
                 {
                     *snr_db = 1400 + (snr - 9600)/1;
                 }
                    *snr_db = 1600;


                 break;
             case 3:
                S3501_PRINTF(NIM_LOG_DBG,"Get snr dB in dvb-s2 8psk mode\n");

                 base_snr = 20;
                 base_snr_db = 2;
                 base_step = 7;
                 base_step_acc = 1;
                 step_polar = 0;
                 
                 if (snr <= 1200)
                  {
                      *snr_db = 600;
                  }
                  else if (snr <= 1800)
                  {
                      *snr_db = 600 + (snr - 1200)/6;
                  }
                  else if (snr <= 2600)
                  {
                      *snr_db = 700 + (snr - 1800)/8;
                  }
                  else if (snr <= 3800)
                  {
                      *snr_db = 800 + (snr - 2600)/12;
                  }
                  else if (snr <= 5400)
                  {
                      *snr_db = 900 + (snr - 3800)/16;
                  }
                  else if (snr <= 7100)
                  {
                      *snr_db = 1000 + (snr - 5400)/17;
                  }
                  else if (snr <= 8900)
                  {
                      *snr_db = 1100 + (snr - 7100)/18;
                  }
                  else if (snr <= 11100)
                  {
                      *snr_db = 1200 + (snr - 8900)/22;
                  }
                  else if (snr <= 12900)
                  {
                      *snr_db = 1300 + (snr - 11100)/18;
                  }
                  else if (snr <= 14600)
                  {
                      *snr_db = 1400 + (snr - 12900)/17;
                  }
                  else if (snr <= 16200)
                  {
                      *snr_db = 1500 + (snr - 14600)/16;
                  }
                  else if (snr <= 17400)
                  {
                      *snr_db = 1600 + (snr - 16200)/12;
                  }
                  else if (snr <= 18200)
                  {
                      *snr_db = 1700 + (snr - 17400)/8;
                  }
                  else if (snr <= 18800)
                  {
                      *snr_db = 1800 + (snr - 18200)/6;
                  }
                  else if (snr <= 19200)
                  {
                      *snr_db = 1900 + (snr - 18800)/4;
                  }
                  else if (snr <= 19400)
                  {
                      *snr_db = 2000 + (snr - 19200)/2;
                  }
                  else if (snr <= 19500)
                  {
                      *snr_db = 2100 + (snr - 19400)/1;
                  }
                  else 
                     *snr_db = 2300;
                  
                 break;
             case 4:
                S3501_PRINTF(NIM_LOG_DBG,"Get snr dB in dvb-s2 16apsk mode\n");

                 base_snr = 20;
                 base_snr_db = 2;
                 base_step = 7;
                 base_step_acc = 1;
                 step_polar = 0;
                 
                 if (snr <= 1500)
                  {
                      *snr_db = 900;
                  }
                  else if (snr <= 2300)
                  {
                      *snr_db = 900 + (snr - 1500)/9;
                  }
                  else if (snr <= 3600)
                  {
                      *snr_db = 1000 + (snr - 2300)/13;
                  }
                  else if (snr <= 5300)
                  {
                      *snr_db = 1100 + (snr - 3600)/17;
                  }
                  else if (snr <= 7400)
                  {
                      *snr_db = 1200 + (snr - 5300)/21;
                  }
                  else if (snr <= 9800)
                  {
                      *snr_db = 1300 + (snr - 7400)/24;
                  }
                  else if (snr <= 12300)
                  {
                      *snr_db = 1400 + (snr - 9800)/25;
                  }
                  else if (snr <= 14800)
                  {
                      *snr_db = 1500 + (snr - 12300)/25;
                  }
                  else if (snr <= 17200)
                  {
                      *snr_db = 1600 + (snr - 14800)/24;
                  }
                  else if (snr <= 19300)
                  {
                      *snr_db = 1700 + (snr - 17200)/21;
                  }
                  else if (snr <= 21000)
                  {
                      *snr_db = 1800 + (snr - 19300)/17;
                  }
                  else if (snr <= 22300)
                  {
                      *snr_db = 1900 + (snr - 21000)/13;
                  }
                  else if (snr <= 23200)
                  {
                      *snr_db = 2000 + (snr - 22300)/9;
                  }
                  else if (snr <= 23900)
                  {
                      *snr_db = 2100 + (snr - 23200)/7;
                  }
                  else if (snr <= 24400)
                  {
                      *snr_db = 2200 + (snr - 23900)/5;
                  }
                  else if (snr <= 24800)
                  {
                      *snr_db = 2300 + (snr - 24400)/4;
                  }
                  else if (snr <= 25100)
                  {
                      *snr_db = 2400 + (snr - 24800)/3;
                  }
                  else if (snr <= 25300)
                  {
                      *snr_db = 2500 + (snr - 25100)/2;
                  }
                  else if (snr <= 25400)
                  {
                      *snr_db = 2600 + (snr - 25300)/1;
                  }
                  else 
                     *snr_db = 2400;

                 break;
            default:
                S3501_PRINTF(NIM_LOG_DBG,"Get snr dB in dvb-s2 unknow mode\n");
                *snr_db = 0;
             break;
            }
        }
    }
    else
    {
        S3501_PRINTF(NIM_LOG_DBG,"TP unlock!\n");
        *snr_db = 0;
    }
    S3501_PRINTF(NIM_LOG_DBG,"Get snr db = %d\n", *snr_db);
        
    return SUCCESS;

}
/*****************************************************************************
* INT32 nim_s3501_get_per(struct nim_device *dev, UINT32 *rsubc)
* Get packet error ratio
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* rsubc
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_per(struct nim_device *dev, UINT32 *rsubc)
{
    UINT8 per[2]={0};
    UINT16 percount = 0;
    UINT8 data = 0;
    UINT8 verdata = 0;
    struct nim_s3501_private *priv = NULL;
	 
    if(dev == NULL || rsubc == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    *rsubc = 1010;
    if((priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) &&
        (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501C))
    {
        nim_reg_read(dev, R04_STATUS, &data, 1);
        if (0x00 != (0x20 & data))
        {
            nim_reg_read(dev, R79_PKT_ERR + 0x01, &per[1], 1);
            per[1] = per[1] & 0x7f;
            nim_reg_read(dev, R79_PKT_ERR, &per[0], 1);
            percount = (UINT16) (per[1] * 256 + per[0]);
            *rsubc = (UINT32) percount;
            S3501_PRINTF(NIM_LOG_DBG,"current PER is  %d\n", percount);

            nim_reg_read(dev, R70_CAP_REG + 0x01, &data, 1);
            data = 0x7f & data;
            nim_reg_write(dev, R70_CAP_REG + 0x01, &data, 1);
            comm_delay(10);
            nim_reg_read(dev, R70_CAP_REG + 0x01, &data, 1);
            data = 0x80 | data;
            nim_reg_write(dev, R70_CAP_REG + 0x01, &data, 1);

            return SUCCESS;
        }
        else
        {
            return ERR_PARA;
        }
    }
    else
    {
        if(flag1 == 0)
        {
            data = 0xff;
            nim_reg_write(dev, R74_PKT_STA_NUM, &data, 1);
            verdata = 0xff;
            nim_reg_write(dev, R74_PKT_STA_NUM + 0x01, &verdata, 1);
            flag1 = 1;
        }
        comm_delay(10);
        nim_reg_read(dev, R04_STATUS, &data, 1);
        if (0x00 != (0x20 & data))
        {
            flag0 = 1;
            if( (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B) &&
             (priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501B))
            {
                nim_reg_read(dev, R9D_RPT_DEMAP_BETA + 1, &verdata, 1);
                if(0x00 == (0x80 & verdata))
                {
                    flag0 = 0;
                    return ERR_PARA;
                }
            }

            if(flag0 == 1)
            {
                nim_reg_read(dev, R79_PKT_ERR + 0x01, &data, 1);
                if (0x00 == (0x80 & data))
                {
                    per[1] = data;
                    nim_reg_read(dev, R79_PKT_ERR, &per[0], 1);
                    per[1] = per[1] & 0x7f;
                    percount = (UINT16) (per[1] * 256 + per[0]);
                    *rsubc = (UINT32) percount;
                    S3501_PRINTF(NIM_LOG_DBG,"current PER is  %d\n", percount);
                    //CR7a
                    data = 0x80;
                    nim_reg_write(dev, R79_PKT_ERR + 0x01, &data, 1);
                }
                else
                {
                    nim_reg_read(dev, R79_PKT_ERR + 0x01, &data, 1);
                    data = (0xEF & data);
                    nim_reg_write(dev, R79_PKT_ERR + 0x01, &data, 1);
                    comm_delay(10);
                    per[1] = data;
                    nim_reg_read(dev, R79_PKT_ERR, &per[0], 1);
                    per[1] = per[1] & 0x7f;
                    percount = (UINT16) (per[1] * 256 + per[0]);
                    *rsubc = (UINT32) percount;
                    S3501_PRINTF(NIM_LOG_DBG,"current PER is  %d\n", percount);
                    //CR7a
                    data = 0x80;
                    nim_reg_write(dev, R79_PKT_ERR + 0x01, &data, 1);
                }
                return SUCCESS;
            }
        }
        else
        {
            return ERR_PARA;
        }
    }
    return SUCCESS;
}

INT32 nim_s3501_get_new_ber(struct nim_device *dev, UINT32 *ber)
{
    UINT8 data = 0;
    UINT32 t_count = 0;
    UINT32 myber = 0;

    if((dev == NULL) || (ber == NULL))
    {
	    return RET_FAILURE;
	}	
    myber = 0;
    for (t_count = 0; t_count < 200; t_count++)
    {
        nim_reg_read(dev, R76_BIT_ERR + 0x02, &data, 1);
        if ((data & 0x80) == 0)
        {
            myber = data & 0x7f;
            nim_reg_read(dev, R76_BIT_ERR + 0x01, &data, 1);
            myber <<= 8;
            myber += data;
            nim_reg_read(dev, R76_BIT_ERR, &data, 1);
            myber <<= 8;
            myber += data;
            break;
        }
    }
    *ber = myber;

    return SUCCESS;
}


INT32 nim_s3501_get_new_per(struct nim_device *dev, UINT32 *per)
{
    UINT8 data = 0;
    UINT32 t_count = 0;
    UINT32 myper = 0;

    if((dev == NULL) || (per == NULL))
    {
	    return RET_FAILURE;
	}	
    myper = 0;
    for (t_count = 0; t_count < 200; t_count++)
    {
        nim_reg_read(dev, R79_PKT_ERR + 0x01, &data, 1);
        if ((data & 0x80) == 0)
        {
            myper = data & 0x7f;
            nim_reg_read(dev, R79_PKT_ERR, &data, 1);
            myper <<= 8;
            myper += data;
            break;
        }
    }
    *per = myper;
    return SUCCESS;
}

INT32 nim_s3501_get_demod_gain(struct nim_device *dev, UINT8 *data)
{
	struct nim_s3501_private *priv = NULL;
	if (dev == NULL)
    {
	    return RET_FAILURE;
	}
	priv = (struct nim_s3501_private *) dev->priv;
	
	nim_reg_read(dev, R07_AGC1_CTRL + 0x04, data, 1);
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,demod get agc value %d\n", __FUNCTION__, __LINE__, *data);
	return SUCCESS;
}

/*****************************************************************************
* static INT32 nim_s3501_get_tuner_gain(struct nim_device *dev,UINT8 agc, INT32 *agc_tuner)
*
*  This function will get the tuner total gain
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* agc
*  Parameter3: UINT16*agc_tuner
*
* Return Value: INT32
*****************************************************************************/
static INT32 nim_s3501_get_tuner_gain(struct nim_device *dev, UINT8 agc, INT32 *agc_tuner)
{
	//INT32 channel_freq_err = 0;//clean warning :unused variable
	struct nim_s3501_private *priv = NULL;

	S3501_PRINTF(NIM_LOG_DBG, "[%s %d],agc=%d, agc_tuner=0x%x\n", __FUNCTION__, __LINE__, agc, agc_tuner);
	
	if (dev == NULL)
    {
    	S3501_PRINTF(NIM_LOG_DBG, "******dev == NULL******\n");
	    return RET_FAILURE;
	}
	
	priv = (struct nim_s3501_private *)dev->priv;
	if (NULL==priv)
	{
		S3501_PRINTF(NIM_LOG_DBG, "******NULL==priv******\n");
		return RET_FAILURE;
	}
	
	if(priv->is_m3031)
	{			
		if (nim_s3501_i2c_open(dev))
    	{
    		S3501_PRINTF(NIM_LOG_DBG, "[%s %d], open I2C error!\n", __FUNCTION__, __LINE__);
        	return S3501_ERR_I2C_NO_ACK;
    	}

		S3501_PRINTF(NIM_LOG_DBG, "[%s %d],priv->tuner_index=%d\n", __FUNCTION__, __LINE__, priv->tuner_index);
		if (priv->nim_tuner_gain != NULL)
		{
			*agc_tuner = priv->nim_tuner_gain(priv->tuner_index,agc);
		}
		S3501_PRINTF(NIM_LOG_DBG, "priv->tuner_index=%d, demod get tuner gain value = %d\n", priv->tuner_index, *agc_tuner);

		if (nim_s3501_i2c_close(dev))
    	{
    		S3501_PRINTF(NIM_LOG_DBG, "[%s %d], close I2C error!\n", __FUNCTION__, __LINE__);
    		return S3501_ERR_I2C_NO_ACK;
    	}
	}
	return SUCCESS;
}


INT32 nim_s3501_get_dbm_level(struct nim_device *dev, INT32 *level_db)
{	
	INT32 level_db_tmp =0;
	INT32 agc_demod_tmp = 0 ;
	UINT8 agc_demod;
	INT32 agc_tuner;
	INT32 agc_gain_flag = 0;    //  Bit[0] = mix_lgain_status , bit[1] =  rf_coarse_gain_status, bit[2] =  dgb_gain_status, used to choose the formula for dBm calculate 
    UINT8 mix_lgain_flag = 0, rf_coarse_gain_flag = 0, dgb_gain_flag = 0;
	
	struct nim_s3501_private *priv = NULL;
	
	if (dev == NULL)
    {
    	S3501_PRINTF(NIM_LOG_DBG, "******dev == NULL******\n");
	    return RET_FAILURE;
	}
	priv = (struct nim_s3501_private *)dev->priv;

	if (NULL==priv)
	{
		S3501_PRINTF(NIM_LOG_DBG, "******NULL==priv******\n");
		return RET_FAILURE;
	}

	nim_s3501_get_demod_gain(dev, &agc_demod);
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d, agc_demod=%d\n", __FUNCTION__, __LINE__, agc_demod);

	if(priv->is_m3031)
	{		
		nim_s3501_get_tuner_gain(dev, agc_demod, &agc_tuner);
		S3501_PRINTF(NIM_LOG_DBG, "tuner agc total gain value %d\n",agc_tuner);
		// Add by paladin 2015/10/22 for agc dbm linear indication
        // Get tuner gain status
        if (priv->is_m3031 && (priv->nim_tuner_command != NULL))
        {
        	priv->nim_tuner_command(priv->tuner_index, NIM_TUNER_GET_M3031_GAIN_FLAG, &agc_gain_flag);
        }
		
        // Bit[0] =  mix_lgain status
        if(agc_gain_flag & 0x00000001)
            mix_lgain_flag = 1; // open
        else
            mix_lgain_flag = 0; // closed

        // Bit[1] =  rf_coarse_gain status
        if(agc_gain_flag & 0x00000002)
            rf_coarse_gain_flag = 1;    // saturation
        else
            rf_coarse_gain_flag = 0;    // not saturation

        // Bit[2] =  dgb_gain status
        if(agc_gain_flag & 0x00000004)
            dgb_gain_flag = 1;          // saturation
        else
            dgb_gain_flag = 0;          // not saturation

        // rf_coarse_gain not saturation
        if(0 == rf_coarse_gain_flag)
        {
            level_db_tmp = (-(agc_tuner + 202)/288 - 25);
        }
        else
        {
            
            if(0 == mix_lgain_flag)  // rf_coarse_gain saturation  && mix_lgain closed
            {
                level_db_tmp = (-(agc_tuner + 122)/244 - 24);
            }            
            else//  rf_coarse_gain saturation  && mix_lgain open
            {
                if(agc_tuner > 14000)
                {
                    level_db_tmp = -96;
                }
                else if(dgb_gain_flag)//  rf_coarse_gain saturation  && mix_lgain open && dgb_gain saturation    
                {
                	level_db_tmp = (-(agc_tuner + 173)/192 - 13);
                }
                else//  rf_coarse_gain saturation  && mix_lgain open && dgb_gain not saturation                 
                {
                	level_db_tmp = (-(agc_tuner + 26)/255 - 25);
                }
            }
        }
        //end
		
		if(level_db_tmp > 0)	// agc with the lowest gain,set 0dBm
			level_db_tmp = 0;

	}	
	else
	{
		if(agc_demod < 128)
			agc_demod_tmp = agc_demod;
		else
			agc_demod_tmp = agc_demod-256 ;

		if(agc_demod_tmp > 74)
		{
			level_db_tmp = 0 ;
		}
		else if(agc_demod_tmp > 70) 
		{
			level_db_tmp =agc_demod_tmp*32/25-95 ;
		}
		else if(agc_demod_tmp > 50)
		{
			level_db_tmp = agc_demod_tmp-76 ;
		}
		else if(agc_demod_tmp >-15)
		{
			level_db_tmp = agc_demod_tmp*2/5-48 ;
		}
		else if(agc_demod_tmp >-120)
		{
			level_db_tmp = agc_demod_tmp/4-53 ;
		}
		else
		{
			level_db_tmp = -96 ;
		}
		
		if(level_db_tmp < -96) // agc with the highest gain,set -96dBm
		{
			level_db_tmp = -96 ;
		}
	}
	
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,level_db_tmp = %d\n",__FUNCTION__, __LINE__, level_db_tmp);
	
	*level_db = level_db_tmp ;
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,level_db = %d\n",__FUNCTION__, __LINE__, *level_db);

	return SUCCESS;
}

#define    s_filter_len  2  //even is better.
static INT32 nim_s3501_get_agc_filter(struct nim_device *dev, INT32 *agc)
{
    INT32 s_sum=0;     
	INT32 k,n1,n2;
	static INT32 j=0;
	static INT32   s_data[15]={0};  
    static INT32   s_agc=0x00;
	
	struct nim_s3501_private *priv = NULL;

	if (dev == NULL)
    {
	    return RET_FAILURE;
	}
	priv = (struct nim_s3501_private *) dev->priv;

	if(*agc==-96)
	{
		j=0;
	}
	
	if (j==(s_filter_len-1)) 
	{      
		for(k=0;k<=(s_filter_len-1);k++)
		{
			s_sum=s_sum+s_data[k];
		}
		
		*agc=s_sum/s_filter_len;
		n1=s_sum-(*agc)*s_filter_len;
		
		if(n1>=(s_filter_len/2))
		{
			n1=1;
		}
		else
		{
			n1=0;
		}
		
		*agc=*agc+n1;          
		s_agc=*agc;
		j=j+1;
	}
    else if (j==s_filter_len)
	{
		n1=(s_agc*(s_filter_len-1))/s_filter_len;
		n1=(s_agc*(s_filter_len-1))-n1*s_filter_len;
		n2=*agc/s_filter_len;
		n2=*agc-n2*s_filter_len;
		n2=n1+n2;
		
		if(n2>=(3*(s_filter_len/2)))
		{
			n1=2;
		}
		else if((n2<(3*(s_filter_len/2)))&&(n2>=(s_filter_len/2)))
		{
			n1=1;
		}
		else
		{
			n1=0;          
		}
		*agc=(s_agc*(s_filter_len-1))/s_filter_len+*agc/s_filter_len+n1;
		s_agc=*agc;
	}
    else 
	{
		s_data[j]=*agc;
		*agc=*agc;
		j=j+1;
	} 
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_s3501_get_AGC(struct nim_device *dev, UINT8 *agc)
*
*  This function will access the NIM to determine the AGC feedback value
*
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: UINT16* agc
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_agc(struct nim_device *dev, UINT8 *agc)
{    
	INT32 level_db_tmp = 0;
	struct nim_s3501_private *priv = NULL;

	if (dev == NULL)
    {
	    return RET_FAILURE;
	}
	priv = (struct nim_s3501_private *) dev->priv;
	
	nim_s3501_get_dbm_level(dev, &level_db_tmp);	
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,--------------------agc gain dBm value %d\n",__FUNCTION__, __LINE__, level_db_tmp);
	
	nim_s3501_get_agc_filter(dev,&level_db_tmp);
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d,---demod agc value after filter %d\n",__FUNCTION__, __LINE__, level_db_tmp);

	*agc = (UINT8)(level_db_tmp + 100);

	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d, agc=%d\n", __FUNCTION__, __LINE__, *agc);
	
    return SUCCESS;
}

/*****************************************************************************
* static INT32 nim_s3501_get_AGC_dBm(struct nim_device *dev, INT8 *agc_dBm)
*
*  This function will access the NIM to determine the AGC feedback value
*  with unit of dBm
* Arguments:
*  Parameter1: struct nim_device *dev
*  Parameter2: INT8* agc_dBm
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_s3501_get_agc_dbm(struct nim_device *dev, INT8 *agc_dbm)
{    
	INT32 level_db_tmp = 0 ;
	struct nim_s3501_private *priv = NULL;
	if (dev == NULL)
    {
	    return RET_FAILURE;
	}
	priv = (struct nim_s3501_private *) dev->priv;
	
	nim_s3501_get_dbm_level(dev, &level_db_tmp);
	//S3501_PRINTF(NIM_LOG_DBG,"--------------------agc gain dBm value %d\n",level_db_tmp);
	
	nim_s3501_get_agc_filter(dev,&level_db_tmp);
	//S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d---demod agc value after filter %d\n",__FUNCTION__, __LINE__, level_db_tmp);

	//*agc_dbm = level_db_tmp;
	if ((level_db_tmp & 0x80000000)==0)
	{
		*agc_dbm = level_db_tmp;
	}
	else
	{
		*agc_dbm = (INT8)level_db_tmp | 0x80;
	}
	
	S3501_PRINTF(NIM_LOG_DBG,"[%s]line=%d, agc_dbm=%d\n", __FUNCTION__, __LINE__, *agc_dbm);
	
    return SUCCESS;
}


INT32 nim_s3501_reg_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
{
    UINT8 data[3] = {0};
    UINT32 sample_rate = 0;
    UINT32 symrate = 0;

    if((dev == NULL) || (sym_rate == NULL))
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, R6C_RPT_SYM_RATE, data, 3);
    symrate = (data[0] >> 1) + (data[1] << 7) + ((data[2] & 0x01) << 15); // K s/s
    nim_s3501_get_dsp_clk(dev, &sample_rate);

    *sym_rate = nim_s3501_multu64div(symrate, sample_rate, 92160);
    return SUCCESS;
}


INT32 nim_s3501_reg_get_freq(struct nim_device *dev, UINT32 *freq)
{
    INT32 freq_off = 0;
    UINT8 data[3]={0};
    UINT32 sample_rate = 0;
    UINT32 tdata = 0;
    UINT32 temp = 0;

    if((dev == NULL) || (freq == NULL))
    {
	    return RET_FAILURE;
	}	
    temp = 0;
    nim_reg_read(dev, R69_RPT_CARRIER, data, 3);
    tdata = (data[0] & 0xff)  + ((data[1] & 0xff) << 8);
    if ((data[2] & 0x01) == 1)
    {
        temp = (tdata ^ 0xffff) + 1;
    }
    else
    {
        temp = tdata & 0xffff;
     }

    nim_s3501_get_dsp_clk(dev, &sample_rate);

    if ((data[2] & 0x01) == 1)
    {
        freq_off = 0 - nim_s3501_multu64div(temp, sample_rate, 92160000);
    }
    else
    {
        freq_off = nim_s3501_multu64div(temp, sample_rate, 92160000);
    }
    *freq += freq_off;

    return SUCCESS;
}





INT32 nim_s3501_reg_get_code_rate(struct nim_device *dev, UINT8 *code_rate)
{
    UINT8 data = 0;

    if(dev == NULL || code_rate == NULL)
    {
	    return RET_FAILURE;
	}	
    nim_reg_read(dev, R69_RPT_CARRIER + 0x02, &data, 1);
    *code_rate = ((data >> 1) & 0x0f);
    return SUCCESS;

    //  Code rate list
    //  for DVBS:
    //      0x0:    1/2,
    //      0x1:    2/3,
    //      0x2:    3/4,
    //      0x3:    5/6,
    //      0x4:    6/7,
    //      0x5:    7/8.
    //  For DVBS2 :
    //      0x0:    1/4 ,
    //      0x1:    1/3 ,
    //      0x2:    2/5 ,
    //      0x3:    1/2 ,
    //      0x4:    3/5 ,
    //      0x5:    2/3 ,
    //      0x6:    3/4 ,
    //      0x7:    4/5 ,
    //      0x8:    5/6 ,
    //      0x9:    8/9 ,
    //      0xa:    9/10.
}


UINT32 nim_s3501_get_curfreq(struct nim_device *dev)
{
    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    return ((struct nim_s3501_private *) dev->priv)->ul_status.m_cur_freq;
}



INT32 nim_s3501_get_bitmode(struct nim_device *dev, UINT8 *bitmode)
{
	struct nim_s3501_private *priv = NULL;
    if(dev == NULL || bitmode == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    if ((priv->tuner_config_data.qpsk_config & 0xc0) == M3501_1BIT_MODE)
    {
        *bitmode = 0x60;
    }
    else if ((priv->tuner_config_data.qpsk_config & 0xc0) == M3501_2BIT_MODE)
    {
        *bitmode = 0x00;
    }
    else if ((priv->tuner_config_data.qpsk_config & 0xc0) == M3501_4BIT_MODE)
    {
        *bitmode = 0x20;
    }
    else if ((priv->tuner_config_data.qpsk_config & 0xc0) == M3501_8BIT_MODE)
    {
        *bitmode = 0x40;
    }
    else
    {
        *bitmode = 0x40;
    }
    return SUCCESS;
}



INT32 nim_s3501_get_dsp_clk(struct nim_device *dev, UINT32 *sample_rate)
{
    UINT8 data = 0;
	struct nim_s3501_private *priv = NULL;
	
    if(dev == NULL || sample_rate == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_s3501_private *) dev->priv;

    if (priv->ul_status.m_s3501_type == NIM_CHIP_ID_M3501B)
    {
        if(priv->ul_status.m_s3501_sub_type == NIM_CHIP_SUB_ID_M3501C)
        {
            nim_reg_read(dev, RF0_HW_TSO_CTRL, &data, 1);
            data = data & 0xc0;
            if(data == 0x00)
            {
               *sample_rate = ((CRYSTAL_FREQ * 90 * 10 + 67) / 135);  ////uint is KHz
            }
            else if(data == 0x40)
            {
               *sample_rate = ((CRYSTAL_FREQ * 98 * 10  + 67) / 135);
            }
            else if(data == 0x80)
            {
               *sample_rate = ((CRYSTAL_FREQ * 108 * 10 + 67) / 135);
            }
            else if(data == 0xc0)
            {
               *sample_rate = ((CRYSTAL_FREQ * 135 * 10 + 67) / 135);
            }
        }
        else
        {
           *sample_rate = ((CRYSTAL_FREQ * 90 * 10 + 67) / 135);
        }
    }
    else
    {
        *sample_rate = ((CRYSTAL_FREQ * 99 * 10 + 67) / 135);
    }
    return SUCCESS;
}




INT32 nim_s3501_hw_check(struct nim_device *dev)
{
    UINT8 data = 0;

    if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    S3501_PRINTF(NIM_LOG_DBG,"Enter nim_s3501_hw_check:\n");

    nim_reg_read(dev, RA3_CHIP_ID + 0x01, &data, 1);
    if (data != 0x35)
    {
        return ERR_FAILED;
    }
    else
    {
        return SUCCESS;
    }
}


void nim_s3501_calculate_energy(struct nim_device *dev)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 k = 0;
    INT32 m = 0;
    INT32 n = 0;
    INT32 energy_real = 0;
    INT32 energy_imag = 0;
    INT32 energy = 0;
    INT32 energy_tmp = 0;
    INT32 fft_i = 0;
    INT32 fft_q = 0;
    UINT8 data = 0;
    UINT8 *pfftdata= NULL;
    UINT8 *pdata = &data;
    UINT32 oldidpos = 0;
    UINT32 find11packet=0;
    UINT32 packetnum = 0;
    UINT32 no_s3501id = 0;
    UINT32 overflow_cnt = 0;
	struct nim_s3501_private *priv =NULL;
	
    if(dev == NULL)
    {
	    return;
	}	
    priv = (struct nim_s3501_private *) dev->priv;
    comm_memset(fft_energy_1024, 0, sizeof(fft_energy_1024));

#ifdef NIM_TS_PORT_CAP
    nim_s3501_fft_result_read(dev);
    pfftdata = (UINT8 *) (priv->ul_status.adcdata);
    comm_delay(100);
#endif
    oldidpos = 0;
    packetnum = 0;

#ifdef   NIM_TS_PORT_CAP
    for (k = 0; k < 20; k++)
    {
        if (priv->ul_status.s3501_autoscan_stop_flag)
        {
            return;
        }

        for(n = 0; n < 1024; n++)
        {
            fft_energy_1024_tmp[n] = 0;
        }
        find11packet = 0;
        while ((find11packet == 0) && (packetnum < PACKET_NUM_662))
        {
            no_s3501id = 1;
            for (i = 0; i < 10; i++)
            {
                data = *(pfftdata + i);
                if (S3501ID[i] != data)
                {
                    no_s3501id = 0;
                }
            }

            if (no_s3501id == 1)
            {
                if ((packetnum - oldidpos) == 11)
                {
                    pdata = pfftdata - 2068;
                    find11packet = 1;
                }
                oldidpos = packetnum;
            }
            pfftdata = pfftdata + 188;
            packetnum++;
        }
        if (find11packet == 0)
        {
            S3501_PRINTF(NIM_LOG_DBG,"cannot find 20 complete S3501 packet\n");

            for(i = 0; i < 1024; i++)
            {
                fft_energy_1024[i] = 0;
            }
            return;
        }
        comm_memcpy(priv->ul_status.adcdata, (pdata + 10), 178);
        for (i = 0; i < 10; i++)
        {
            comm_memcpy(priv->ul_status.adcdata + 178 + i * 187, pdata + (i + 1) * 188 + 1, 187);
        }
        overflow_cnt = 0;
        for (i = 0; i < 1024; i++)
        {
            fft_i = priv->ul_status.adcdata[2 * i];
            fft_q = priv->ul_status.adcdata[2 * i + 1];

            if ((fft_i == FFT_VAL_127) || (fft_i == FFT_VAL_128))
            {
                overflow_cnt++;
            }
            if ((fft_q == FFT_VAL_127) || (fft_q == FFT_VAL_128))
            {
                overflow_cnt++;
            }

            if (fft_i & 0x80)
            {
                fft_i |= 0xffffff00;
            }
            if (fft_q & 0x80)
            {
                fft_q |= 0xffffff00;
            }

            m = 0;
            for (j = 0; j < 10; j++)
            {
                m <<= 1;
                m += ((i >> j) & 0x1);
            }
            fft_i = fft_i << (FFT_BITWIDTH - 8);
            fft_q = fft_q << (FFT_BITWIDTH - 8);
            priv->ul_status.FFT_I_1024[m] = fft_i;
            priv->ul_status.FFT_Q_1024[m] = fft_q;
        }
        if(1 == (m3501_debug_flag & 0x01))
        {
            for(i = 0; i < 1024; i++)
            {
                AUTOSCAN_PRINTF("ADCdata%d:[%d,%d]\n", (int)i, (int)priv->ul_status.FFT_I_1024[i], (int)priv->ul_status.FFT_Q_1024[i]);
            }
        }
        //R2FFT
        R2FFT(priv->ul_status.FFT_I_1024, priv->ul_status.FFT_Q_1024);

#else

    for (k = 0; k < 20; k++)
    {
        if (priv->ul_status.s3501_autoscan_stop_flag)
        {
            return;
        }
        S3501_PRINTF(NIM_LOG_DBG,"fft_start\n");
        nim_s3501_fft_result_read(dev);
        S3501_PRINTF(NIM_LOG_DBG,"fft_end\n");
#endif
        for (i = 0; i < 1024; i++)
        {
            fft_i = priv->ul_status.FFT_I_1024[i];
            fft_q = priv->ul_status.FFT_Q_1024[i];

            energy_real = fft_i * fft_i;
            energy_imag = fft_q * fft_q;
            energy = energy_real + energy_imag;
            energy >>= 3;
            j = (i + 511) & 1023;
            energy_tmp =  energy;

            if ((energy_tmp >> 20) & 1)
            {
                fft_energy_1024_tmp[j] = 1048575;
            }
            else
            {
                fft_energy_1024_tmp[j] = energy_tmp;
            }
        }
        for(n = 0; n < 1024; n++)
        {
            fft_energy_1024[n] += fft_energy_1024_tmp[n];
        }

    }
}


