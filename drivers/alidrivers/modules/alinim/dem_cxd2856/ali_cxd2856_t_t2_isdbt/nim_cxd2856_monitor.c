/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/04/06          leo.liu           v0.1                  create monitor function      
----------------------------------------------------------------------------*/
#include "nim_cxd2856_common.h"
INT32 nim_cxd2856_get_lock(struct nim_device *dev, UINT8 *lock)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	INT32 				result = SONY_RESULT_OK;
	UINT8 				tun_lock = 0;
	sony_demod_lock_result_t demod_lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;//DEMOD_LOCK_RESULT_NOTDETECT
	
	if(dev == NULL || lock == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	
	*lock = 0;
	if(user->tuner_control.nim_tuner_status != NULL)
	{
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* Enable the I2C repeater */			
			if(sony_demod_I2cRepeaterEnable (pdemod, 0x01) !=SONY_RESULT_OK) 
			{
				goto ERROR_EXIT;
			}
		}
		
		user->tuner_control.nim_tuner_status(user->tuner_id,&tun_lock);
		
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* disable the I2C repeater */
			if(sony_demod_I2cRepeaterEnable (pdemod, 0x00) !=SONY_RESULT_OK) 
			{
				goto ERROR_EXIT;
			}
		}		
		
	}
	else
	{
		goto ERROR_EXIT;
	}

	if(tun_lock == 0)
	{
		goto ERROR_EXIT;
	}
	else
	{
		pr_debug("tuner lock ok\n");
	}
	
	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
		    result =  sony_demod_dvbt_CheckTSLock(pdemod, &demod_lock);//sony_dvb_demodT_CheckTSLock
		    break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
		    result =  sony_demod_dvbt2_CheckTSLock(pdemod, &demod_lock);//sony_dvb_demodT2_CheckTSLock
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
			result =  sony_demod_isdbt_CheckTSLock(pdemod, &demod_lock);//sony_dvb_demodT2_CheckTSLock
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			demod_lock = SONY_DEMOD_LOCK_RESULT_UNLOCKED;
			break;
    }
    if ( SONY_DEMOD_LOCK_RESULT_LOCKED == demod_lock && SONY_RESULT_OK == result)
    {
        *lock = 1;
    }else
    {
    	*lock = 0;
    }
	return SUCCESS;
ERROR_EXIT:
	*lock = 0;
	return SUCCESS;
}
INT32 nim_cxd2856_get_SSI(struct nim_device *dev, UINT8 *ssi)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	sony_result_t CalcResult = SONY_RESULT_ERROR_OTHER;
	__u32 rfLevel;
	UINT32 rf_level = 0;
	
	if(dev == NULL || ssi == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	nim_cxd2856_get_rf_level(dev, &rf_level);
	rfLevel = 0 - rf_level*100;//-0.1dbm -> 0.001dbm
	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
		    CalcResult = sony_integ_dvbt_monitor_SSI (priv, ssi,rfLevel);//unit of rfLevel is 0.001dbm
		    break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
		    CalcResult =  sony_integ_dvbt2_monitor_SSI(priv,ssi,rfLevel);
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			break;
	}
	if(SONY_RESULT_OK != CalcResult)
	{
		*ssi = 0;
		pr_debug("Call SSI failure\n");
		return ERR_FAILUE;
	}
	else
	{
		pr_debug(" SSI                     | Signal Strength | %u\n", *ssi);
		return SUCCESS;
		
	}
	
}
INT32 nim_cxd2856_get_SQI(struct nim_device *dev, UINT8 *sqi)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	sony_result_t CalcResult = SONY_RESULT_OK;
	
	if(dev == NULL || sqi == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	
	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
		    CalcResult = sony_demod_dvbt_monitor_Quality(pdemod,sqi);
		    break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
		    CalcResult =  sony_demod_dvbt2_monitor_Quality(pdemod,sqi);
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
			//
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			break;
	}
	if(SONY_RESULT_OK != CalcResult)
	{
		*sqi = 0;
		pr_debug("Call SQI failure\n");
		return ERR_FAILUE;
	}
	else
	{
		pr_debug(" SQI                     | Signal Strength | %u\n", *sqi);
		return SUCCESS;
	}
}
/******************    USAGE NOTE    *************************
	nim_cxd2856_get_per() is a special api,sony_demod_dvbt_monitor_PER() or 
	sony_demod_dvbt2_monitor_PER will return fail when demod unlock.
	so,we let the nim_cxd2856_get_per() return a special value:INVALID_VALUE,
	indicating that the error was returned because of unlock.
**********************************************************/

INT32 nim_cxd2856_get_per(struct nim_device *dev, UINT32 *per)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	sony_result_t CalcResult = SONY_RESULT_OK;
	
	if(dev == NULL || per == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;

	*per = 0; //clear data
	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
		    CalcResult = sony_demod_dvbt_monitor_PER(pdemod,(uint32_t *)per);
		    break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
		    CalcResult =  sony_demod_dvbt2_monitor_PER(pdemod,(uint32_t *)per);
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
			//
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			break;
	}
	if(SONY_RESULT_OK != CalcResult)
	{
		*per = INVALID_VALUE;//special value
		pr_debug("Call per failure\n");
		return ERR_FAILUE;
	}
	else
	{
		pr_debug(" per = %lu\n", *per);
		return SUCCESS;
	}
		
}
INT32 nim_cxd2856_get_ber(struct nim_device *dev, UINT32 *ber)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	sony_result_t CalcResult = SONY_RESULT_OK;
	
	if(dev == NULL || ber == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;

	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
			
		    CalcResult = sony_demod_dvbt_monitor_PreRSBER(pdemod,(uint32_t *)ber);
		    break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
		    CalcResult =  sony_demod_dvbt2_monitor_PreBCHBER(pdemod,(uint32_t *)ber);
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
			//
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			break;
	}
	if(SONY_RESULT_OK != CalcResult)
	{
		*ber = 0;
		pr_debug("Call ber failure\n");
		return ERR_FAILUE;
	}
	else
	{
		pr_debug(" ber = %lu\n", *ber);
		return SUCCESS;
	}
		
}

INT32 nim_cxd2856_get_cn(struct nim_device *dev, UINT32 *cn)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	sony_result_t CalcResult = SONY_RESULT_OK;
	INT32 cn_tmp = 0;
	
	if(dev == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	switch (pdemod->system)
	{
		case SONY_DTV_SYSTEM_DVBT:
		{
		    CalcResult =  sony_demod_dvbt_monitor_SNR(pdemod,(int32_t *)&cn_tmp);
			break;
		}
		case SONY_DTV_SYSTEM_DVBT2:
		{
			CalcResult = sony_demod_dvbt2_monitor_SNR(pdemod,(int32_t *)&cn_tmp);//0.001db,23100 = 23.1db
		    break;
		}
		case SONY_DTV_SYSTEM_ISDBT:
		{
			//
		    break;
		}
		case SONY_DTV_SYSTEM_UNKNOWN:
		default:
			break;
	}
	if(SONY_RESULT_OK != CalcResult)
	{
		*cn = 0;
		pr_debug("Call Cn failure \n");
		return ERR_FAILUE;
	}
	else
	{
		*cn = (UINT32)(cn_tmp / 10);//0.01db,23100 /10= 2310 = 2310*0.01 = 23.1 
		pr_debug(" cn = %lu\n", *cn);
		return SUCCESS;
	}
		
}
INT32 nim_cxd2856_get_rf_level(struct nim_device *dev, UINT32 *rf_level)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	__u32 rf_temp;
	int ret = 0;
	if(dev == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	
	
	//if tuner is mxl608/603,can get rfLevel from tuner 
	if(user->tuner_control.nim_tuner_command)
	{
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* Enable the I2C repeater */			
			if(sony_demod_I2cRepeaterEnable (pdemod, 0x01) !=SONY_RESULT_OK) 
			{
				goto ERROR_EXIT;
			}
		}
		
		ret = user->tuner_control.nim_tuner_command(user->tuner_id,NIM_TUNER_GET_RF_POWER_LEVEL,(__u32)&rf_temp);
		
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* disable the I2C repeater */
			if(sony_demod_I2cRepeaterEnable (pdemod, 0x00) !=SONY_RESULT_OK) 
			{
				goto ERROR_EXIT;
			}
		}
	}
	else
	{
		pr_err("[err] nim_tuner_command == NULL\n");
		goto ERROR_EXIT;
	}
	if(ret != SUCCESS)
	{
		*rf_level = 0;
		pr_debug(" rf = %lu\n", *rf_level);
	}
	*rf_level = (UINT32)rf_temp;// 0.1dbm;462 = 462*0.1= 46.2 dbm
	return SUCCESS;

ERROR_EXIT:
	*rf_level = 0;
	return ERR_FAILUE;
	
}
INT32 nim_cxd2856_get_freq(struct nim_device *dev, UINT32 *freq)
{
	sony_integ_t 		* priv = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
	
	if(dev == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	
	*freq = user->Frequency;
	return SUCCESS;
}
static UINT8 cxd2856_modulation_map_to_ali_modulation(sony_dtv_system_t system, UINT8 modulation)
{
    //T_NODE:	UINT32 modulation : 8;	
    //2:DQPSK 4:QPSK, 16:16 QAM, 64:64 QAM //T2: (64+1):256 QAM

   if (system == SONY_DTV_SYSTEM_DVBT)
    {
        switch (modulation)
        {
            case SONY_DVBT_CONSTELLATION_QPSK:
                return 4;
            case SONY_DVBT_CONSTELLATION_16QAM:
                return 16;
            case SONY_DVBT_CONSTELLATION_64QAM:
                return 64;
            default:
                return 0xFF;   //unknown.
        }
    }
    else
    {
        switch (modulation)
        {
            case SONY_DVBT2_QPSK:
                return 4;
            case SONY_DVBT2_QAM16:
                return 16;
            case SONY_DVBT2_QAM64:
                return 64;
            case SONY_DVBT2_QAM256:
                return (64+1);
            default:
                return 0xFF;   //unknown.
        }
    }
}

INT32 nim_cxd2856_get_modulation(struct nim_device *dev, UINT8 *modulation)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;
    //sony_dvbt2_ofdm_t ofdm;
    sony_dvbt2_l1pre_t 	L1Pre;
    sony_dvbt2_plp_t 	plpInfo;
    sony_dvbt_tpsinfo_t tps;
	//__u32 				rf_temp;
	int 				result = SONY_RESULT_OK;
	sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
	if(dev == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
	
    *modulation = 0;
    switch (pdemod->system) 
	{
	    case SONY_DTV_SYSTEM_DVBT2:
	        //result = sony_demod_dvbt2_CheckDemodLock(pdemod, &lock);
			result =  sony_demod_dvbt2_CheckTSLock(pdemod, &lock);//sony_dvb_demodT2_CheckTSLock
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        //result = sony_demod_dvbt_CheckDemodLock (pdemod, &lock);
			result = sony_demod_dvbt_CheckTSLock(pdemod, &lock);//sony_dvb_demodT_CheckTSLock
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
			break;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED)
    {
         NIM_TRACE_RETURN_ERROR("unlock",ERR_FAILUE);
    }
    if (pdemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
        sony_demod_dvbt2_monitor_L1Pre(pdemod, &L1Pre);//sony_dvb_demod_monitorT2_L1Pre
        if (result != SONY_RESULT_OK) 
        {
            NIM_TRACE_RETURN_ERROR("T2_L1Pre info fail",ERR_FAILUE);
        }
    
        // Get Active PLP information. 
        result = sony_demod_dvbt2_monitor_ActivePLP(pdemod, SONY_DVBT2_PLP_DATA, &plpInfo);//sony_dvb_demod_monitorT2_ActivePLP
        if (result != SONY_RESULT_OK || plpInfo.constell > SONY_DVBT2_QAM256 ) 
        {
            NIM_TRACE_RETURN_ERROR("T2_ActivePLP info fail",ERR_FAILUE);
        }
        if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
        {
           /* if (plpInfo.id != user->plp_id)
            {
                pr_err( "%s(): plp_id=%d, plpInfo.id=%d, error PLP locked.\r\n", __FUNCTION__,  user->plp_id, plpInfo.id);
                return ERR_FAILUE;
            }*/

            *modulation = cxd2856_modulation_map_to_ali_modulation(pdemod->system, plpInfo.constell);
			pr_debug("get modulation is :%d\n",*modulation);
            return SONY_RESULT_OK;
        }
    }
    else
    {
        result = sony_demod_dvbt_monitor_TPSInfo(pdemod, &tps);//sony_dvb_demod_monitorT_TPSInfo
        if (result != SONY_RESULT_OK || tps.constellation >= SONY_DVBT_CONSTELLATION_RESERVED_3 )
        {
        	NIM_TRACE_RETURN_ERROR("TPS info failure",ERR_FAILUE);
        }
        *modulation = cxd2856_modulation_map_to_ali_modulation(pdemod->system, tps.constellation);
    }
	pr_debug("get modulation is :%d\n",*modulation);
    return SUCCESS;
}
static UINT8 cxd2856_fft_mode_map_to_ali_fft_mode(sony_dtv_system_t system, UINT8 fft_mode)
{
    //T_NODE:	UINT32 FFT : 8;	
    //2:2k, 8:8k //T2: 4:4k, 16:16k, 32:32k
    
    if (system == SONY_DTV_SYSTEM_DVBT)
    {
        switch (fft_mode)
        {
            case SONY_DVBT_MODE_2K:
                return 2;
            case SONY_DVBT_MODE_8K:
                return 8;
            default:
                return 0xFF;   //unknown.
        }
    }
    else
    {
        switch (fft_mode)
        {
            case SONY_DVBT2_M2K:
                return 2;
            case SONY_DVBT2_M8K:
                return 8;
            case SONY_DVBT2_M4K:
                return 4;
            case SONY_DVBT2_M1K:
                return 1;
            case SONY_DVBT2_M16K:
                return 16;
            case SONY_DVBT2_M32K:
                return 32;
            default:
                return 0xFF;   //unknown.
        }
    }
}

INT32 nim_cxd2856_get_fftmode(struct nim_device *dev, UINT8 *fft_mode)
{
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T	* user = NULL;   
    sony_dvbt2_ofdm_t 	ofdm;
    sony_dvbt_mode_t 	fft_mode_t;
    sony_dvbt_guard_t 	gi_t;
	int 				result = SONY_RESULT_OK;
	sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
  	if(dev == NULL)
	{
		NIM_TRACE_RETURN_ERROR("NULL pointer",ERR_FAILUE);
	}
	priv 	= dev->priv;
	user	= priv->user;
	pdemod	= priv->pDemod;
    *fft_mode = 0;

    switch (pdemod->system) 
	{
	    case SONY_DTV_SYSTEM_DVBT2:
	        //result = sony_demod_dvbt2_CheckDemodLock(pdemod, &lock);
			result =  sony_demod_dvbt2_CheckTSLock(pdemod, &lock);//sony_dvb_demodT2_CheckTSLock
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        //result = sony_demod_dvbt_CheckDemodLock (pdemod, &lock);
			result = sony_demod_dvbt_CheckTSLock(pdemod, &lock);//sony_dvb_demodT_CheckTSLock
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
			break;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED)
    {
         NIM_TRACE_RETURN_ERROR("unlock",ERR_FAILUE);
    }
    if (pdemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
    	
		result = sony_demod_dvbt2_monitor_OFDM(pdemod, &ofdm);//sony_dvb_demodT2_OptimizeMISO
        if (result != SONY_RESULT_OK) 
        {
             NIM_TRACE_RETURN_ERROR("T2 OFDM info fail",ERR_FAILUE);
        }
        *fft_mode = cxd2856_fft_mode_map_to_ali_fft_mode(pdemod->system, ofdm.mode);
    }
    else
    {
        result = sony_demod_dvbt_monitor_ModeGuard(pdemod, &fft_mode_t, &gi_t);//sony_dvb_demod_monitorT_ModeGuard       
        if (result != SONY_RESULT_OK) 
        {
            NIM_TRACE_RETURN_ERROR("T ModeGuard info fail",ERR_FAILUE);
        }
        *fft_mode = cxd2856_fft_mode_map_to_ali_fft_mode(pdemod->system, fft_mode_t);
    }
	pr_debug("get fftmode is :%d\n",*fft_mode);
	return SUCCESS;
}


