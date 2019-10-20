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
 *    Description: cxd2837 nim driver for linux api
 */


#include "porting_cxd2837_linux.h"
#include "sony_demod.h"
#include "sony_demod_integ.h"
#include "nim_cxd2837.h"


#define sony_demod_GPIOSetConfig cxd2837_demod_GPIOSetConfig
#define sony_demod_SetConfig     cxd2837_demod_SetConfig

//static UINT8  t2_only  = 0;     //ony search T2 Signal flag
static UINT8  t2_lite_support = 0;
BOOL play_program, NeedToInitSystem = FALSE, NeedToConfigTuner = FALSE;

void cxd2837_log_i2c(sony_demod_t* param, UINT8 err, UINT8 write, UINT8 slv_addr, UINT8 *data, int len)
{
    int i = 0;
    if ( ! (param->output_buffer && param->fn_output_string) )
        return;

    if (write)
        CXD2837_PRINTF(NIM_LOG_DBG,"I2C_Write,0x%02X", (slv_addr&0xFE));
    else
        CXD2837_PRINTF(NIM_LOG_DBG,"I2C_Read,0x%02X", (slv_addr|1));

    for ( i=0; i<len; ++i )
    {
        CXD2837_PRINTF(NIM_LOG_DBG,",0x%02X", data[i]);
    }

    if (err)
        CXD2837_PRINTF(NIM_LOG_DBG,"\terror");
    CXD2837_PRINTF(NIM_LOG_DBG,"\r\n");
}

void nim_cxd2837_switch_lock_led(struct nim_device *dev, BOOL On)
{
    if(((sony_demod_t *)dev->priv)->tuner_control.nim_lock_cb)
        ((sony_demod_t *)dev->priv)->tuner_control.nim_lock_cb(On);
}

#if 0
static INT32 config_tuner(struct nim_device *dev, UINT32 freq, UINT32 bandwidth, UINT8 *ptr_lock)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	UINT32 tuner_id = priv->tuner_id;
	UINT8 lock = 0, Tuner_Retry = 0;

	//set_bypass_mode(dev, TRUE);
	do
	{
		Tuner_Retry++;

		if(priv->tuner_control.nim_tuner_control(tuner_id, freq, bandwidth, 0, (UINT8*)&(priv->system),0) == ERR_FAILUE)
		{
			CXD2837_PRINTF(NIM_LOG_DBG,"Config tuner failed, I2c failed!\r\n");
		}

		if(priv->tuner_control.nim_tuner_status(tuner_id, &lock) != SUCCESS)
		{
			//if i2c read failure, no lock state can be report.
			lock = 0;
			CXD2837_PRINTF(NIM_LOG_DBG,"Config tuner failed, I2c failed!\r\n");
		}
		CXD2837_PRINTF(NIM_LOG_DBG,"Tuner Lock Times=0x%d,Status=0x%d\r\n",Tuner_Retry,lock);

		if(Tuner_Retry > 5)
			break;
	}while(0 == lock);

	if(ptr_lock != NULL)
		*ptr_lock = lock;

	if(Tuner_Retry > 5)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"ERROR! Tuner Lock Fail\r\n");
		return ERR_FAILUE;
	}
	return SUCCESS;
}
#endif

static BOOL need_to_config_tuner(struct nim_device *dev, UINT32 freq, UINT32 bandwidth)
{
    sony_demod_t * param = (sony_demod_t *)dev->priv;    
    UINT8 lock = 0;
	nim_cxd2837_get_lock(dev, &lock);	//add the judge for tuner wthh the help of the demod
   /* return ! ( param->Frequency == freq \
            && param->bandwidth == bandwidth \
            && param->tuner_control.nim_tuner_status(tuner_id, &lock) == SUCCESS);*/
    return !( param->Frequency == freq \
            && param->bandwidth == bandwidth \
            && lock);
}

BOOL need_to_lock_DVBT_signal(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para, BOOL NeedToConfigTuner)
{
    sony_demod_t * param = (sony_demod_t *)dev->priv;

    if ( change_para->usage_type == USAGE_TYPE_AUTOSCAN \
        || change_para->usage_type == USAGE_TYPE_CHANSCAN \
        || change_para->usage_type == USAGE_TYPE_AERIALTUNE )
        return TRUE;    //Auto detect signal type for Auto Scan and Channel Scan.

    if ( change_para->usage_type == USAGE_TYPE_NEXT_PIPE_SCAN )
        return FALSE;

    //USAGE_TYPE_CHANCHG: for play program.
    //if (change_para->t2_signal) //Do nothing for play DVB-T2 program.
    //    return FALSE;
    //if (param->system != SONY_DTV_SYSTEM_DVBT) //DEM is not be DVB-T mode now.
    //    return TRUE;
    if (param->priority != change_para->priority) //DVB-T Hierarchy mode: HP/LP.
        return TRUE;

    return NeedToConfigTuner;
}
INT32 need_to_lock_DVBT2_signal(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para,BOOL NeedToConfigTuner, BOOL *p_play_program)
{
    sony_demod_t * param = (sony_demod_t *)dev->priv;
    *p_play_program = FALSE;

	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,change_para->usage_type:%d,change_para->t2_signal=%d,NeedToConfigTuner=%d,param->system=%d,\n",__FUNCTION__,__LINE__, \
		                          change_para->usage_type,change_para->t2_signal,NeedToConfigTuner,param->system);

    if ( change_para->usage_type == USAGE_TYPE_AUTOSCAN   \
        || change_para->usage_type == USAGE_TYPE_CHANSCAN \
        || change_para->usage_type == USAGE_TYPE_AERIALTUNE \
        || change_para->usage_type == USAGE_TYPE_NEXT_PIPE_SCAN )
        return TRUE;    //Auto detect signal type for Auto Scan and Channel Scan.

   //USAGE_TYPE_CHANCHG: for play program.
   // if ( !change_para->t2_signal ) //Do nothing for play DVB-T program.
   //     return FALSE;

    *p_play_program = TRUE;
    //if (param->system != SONY_DTV_SYSTEM_DVBT2) //DEM is not be DVB-T2 mode now.
    //    return TRUE;

    if (param->plp_index != change_para->plp_index || param->plp_id != change_para->plp_id) //Current selected PLP is different with the target PLP.
        return TRUE;

    if (param->t2_profile != change_para->t2_profile) //DVB-T2 profile: base or lite.
        return TRUE;
	 
    return NeedToConfigTuner;
}

INT32 try_to_lock_DVBT_signal(struct nim_device *dev, BOOL NeedToInitSystem,BOOL NeedToConfigTuner,NIM_CHANNEL_CHANGE_T *change_para)
{

	sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
	sony_dvbt_tune_param_t TuneParam;
	sony_result_t tuneResult;

    if(SONY_DEMOD_BW_1_7_MHZ == change_para->bandwidth)    
    {        
        CXD2837_PRINTF(NIM_LOG_DBG,"DVB-T not support 1.7MHZ!\n");
        return ERR_FAILUE;
    }
	memset(&TuneParam,0,sizeof(TuneParam));
	TuneParam.centerFreqKHz = change_para->freq;
	TuneParam.bandwidth = change_para->bandwidth;
	TuneParam.profile = change_para->priority; //SONY_DVBT_PROFILE_HP;

	if( NeedToInitSystem || (pDemod->system != SONY_DTV_SYSTEM_DVBT))
	{
		pDemod->t2_signal = 1;//dvbt
		//to do...
	}

	tuneResult = sony_integ_dvbt_Tune(pDemod, &TuneParam,NeedToConfigTuner);
	switch(tuneResult){
	case SONY_RESULT_OK:
		 CXD2837_PRINTF(NIM_LOG_DBG,"[SONY]DVB-T TS Locked.\n");
		//(("DVB-T TS Locked.\n"));
		return SUCCESS;
	case SONY_RESULT_ERROR_UNLOCK:
		CXD2837_PRINTF(NIM_LOG_DBG,"[SONY]DVB-T TS Unlocked.\n");
		//(("DVB-T TS Unlocked.\n"));
		return ERR_FAILUE;
	case SONY_RESULT_ERROR_TIMEOUT:
		CXD2837_PRINTF(NIM_LOG_DBG,("[SONY]DVB-T Wait TS Lock but Timeout.\n"));
		//(("DVB-T Wait TS Lock but Timeout.\n"));
		return ERR_FAILUE;
	default:
		CXD2837_PRINTF(NIM_LOG_DBG,"[SONY] default: in sony_integ_dvbt_Tune\n");
		//(("Error in sony_integ_dvbt_Tune (%s)\n", sony_FormatResult(sony_result)));
		return ERR_FAILUE;
	}
}

INT32 DVBT2_change_PLP(struct nim_device *dev, UINT8 plp_id, BOOL NeedToInitSystem, BOOL NeedToConfigTuner, BOOL AUTOSCAN)
{
	sony_result_t tuneResult = SONY_RESULT_OK;
	sony_demod_t* pDemod = (sony_demod_t *)dev->priv;
	sony_dvbt2_tune_param_t tuneParam;

	pDemod->t2_signal = 2;//dvbt2
	pDemod->plp_id = plp_id;
	memset(&tuneParam,0,sizeof(tuneParam));
	tuneParam.centerFreqKHz = pDemod->Frequency;
	tuneParam.bandwidth= pDemod->bandwidth;
	tuneParam.dataPlpId = plp_id; //selected data PLP ID. 
	tuneParam.profile = pDemod->t2_profile;
	tuneParam.tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_OK;

	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:PLP_id(%d),autoScan(%d).\n",__FUNCTION__,plp_id,AUTOSCAN);

	if(0 == AUTOSCAN)
		tuneResult = sony_integ_dvbt2_Tune(pDemod, &tuneParam,NeedToConfigTuner);
	else if(1 == AUTOSCAN)
	{
		tuneResult = sony_integ_dvbt2_BlindTune(pDemod, &tuneParam,NeedToConfigTuner,t2_lite_support);	//By SONY AEC 20130701
	}
	switch(tuneResult){
        case SONY_RESULT_OK:
	        CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:DVB-T2 TS Locked.\n",__FUNCTION__);
	        return SUCCESS;
        case SONY_RESULT_ERROR_UNLOCK:
	        CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:DVB-T2 TS Unlocked.\n",__FUNCTION__);
	        return ERR_FAILUE;
        case SONY_RESULT_ERROR_TIMEOUT:
            CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:DVB-T2 Wait TS Lock but Timeout.\n",__FUNCTION__);
	        return ERR_FAILUE;
        default:
	        CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:Error in sony_integ_dvbt2_Tune (%d)\n",__FUNCTION__,tuneResult);
	        return ERR_FAILUE;
    }

}

INT32 try_to_lock_next_data_plp(struct nim_device *dev, BOOL NeedToInitSystem, BOOL NeedToConfigTuner)
{
    INT32 result = ERR_FAILUE;
    sony_demod_t * param = (sony_demod_t *)dev->priv;
    sony_dvbt2_l1pre_t L1Pre;
    sony_dvbt2_plp_t plpInfo;
    UINT8 retry = 0; 
    UINT8 plp_index = 0; 

    for (plp_index = param->plp_index+1; plp_index < param->plp_num; ++plp_index )
    {
        if (plp_index >= param->plp_num)
            return ERR_FAILUE;
        result = DVBT2_change_PLP(dev, param->all_plp_id[plp_index], FALSE, FALSE, 0);
        
        for ( retry=0; retry < 30; ++retry )
        {
            SONY_SLEEP (30);
            if (param->autoscan_stop_flag)
                return SONY_RESULT_OK;    
            if (param->do_not_wait_t2_signal_locked)
                return SONY_RESULT_OK;
            
            result = sony_demod_dvbt2_monitor_L1Pre(param, &L1Pre);//sony_dvb_demod_monitorT2_L1Pre
            if (result != SONY_RESULT_OK) 
            {
                CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d) error: sony_dvb_demod_monitorT2_L1Pre()=%d \r\n", __FUNCTION__,NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index],result);
                continue;
				//return result;
            }
        
            // Get Active PLP information. 
            result = sony_demod_dvbt2_monitor_ActivePLP(param, SONY_DVBT2_PLP_DATA, &plpInfo);//sony_dvb_demod_monitorT2_ActivePLP
            if (result != SONY_RESULT_OK) 
            {
				CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d) error: sony_dvb_demod_monitorT2_ActivePLP()=%d \r\n", __FUNCTION__,NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index],result);
				continue;
				//return result;
            }
            
            if (result == SONY_RESULT_OK) 
            {
                if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
                {
                    if (plpInfo.id != param->all_plp_id[plp_index])
                    {
                        CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d), plpInfo.id=%d, error PLP locked: retry %d times.\r\n", __FUNCTION__, NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index], plpInfo.id, retry);
                        continue;
                    }
                    else
                        break; //correct PLP is locked.
                }
            }
        }

        if (result == SONY_RESULT_OK  && (plpInfo.id == param->all_plp_id[plp_index]) ) 
        {
            if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
            {
                param->plp_id = plpInfo.id;
                param->t2_system_id = L1Pre.systemId;
                param->plp_index = plp_index;
                
                if (retry!=0)
                    CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d), ok: retry %d times.\r\n", __FUNCTION__, NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index], retry);
                return SONY_RESULT_OK;
            }
            else
            {
                    CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d), ok: retry %d times. error: Not DataPLP: (type=%d, id=%d)\r\n", __FUNCTION__, NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index], retry, plpInfo.type, plpInfo.id);
            }
        }
        else
        {
            CXD2837_PRINTF(NIM_LOG_DBG,"%s(%d,%d)(plp_num=%d, all_plp_id[%d]=%d), error: fail to lock the PLP.\r\n", __FUNCTION__, NeedToInitSystem,NeedToConfigTuner, param->plp_num, plp_index, param->all_plp_id[plp_index], retry);
        }
    }
    return ERR_FAILUE;
}

INT32 try_to_lock_DVBT2_signal(struct nim_device *dev, BOOL NeedToInitSystem, BOOL NeedToConfigTuner)
{	
	UINT16 waitTime = 0;
	INT32 result = ERR_FAILUE;
	sony_demod_t * param = (sony_demod_t *)dev->priv;
	
	param->plp_num = 0;
	param->plp_id = 0;
	param->plp_index = 0;
	
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d\n ", __FUNCTION__,__LINE__);

	if(NULL == dev)
	{
		return ERR_FAILUE;
	}
	else
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,NeedToInitSystem=%d,NeedToConfigTuner=%d,\n ", __FUNCTION__,__LINE__,NeedToInitSystem,NeedToConfigTuner);
	}
	
	result = DVBT2_change_PLP(dev, 0, NeedToInitSystem, NeedToConfigTuner,1); //for lock signal first.
	//get_the_first_data_PLP_info
	do
	{
		result = sony_demod_dvbt2_monitor_DataPLPs(param, (uint8_t *)&(param->all_plp_id),(uint8_t *)&(param->plp_num));
		if (result == SONY_RESULT_OK) 
		{
			UINT8 plp_idx;
			CXD2837_PRINTF(NIM_LOG_DBG,"\t[%s]: plp_num=%d\n ", __FUNCTION__, param->plp_num);
			for (plp_idx=0; plp_idx < param->plp_num; ++plp_idx)
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"\t[plp_id=%d]\n", param->all_plp_id[plp_idx]);
			}
			param->plp_id = param->all_plp_id[0];//get first plp_id
			break;
		}
		else if (result == SONY_RESULT_ERROR_HW_STATE) 
		{
			if (waitTime >= DTV_DEMOD_TUNE_T2_L1POST_TIMEOUT)
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"%s() error: timeout for get the first data_PLP\r\n", __FUNCTION__);
				param->plp_num = 0;
				return ERR_FAILUE;
			}
			else 
			{
				SONY_SLEEP (DEMOD_TUNE_POLL_INTERVAL); //10
				waitTime += DEMOD_TUNE_POLL_INTERVAL; 
			}
		} 
		else 
		{
			CXD2837_PRINTF(NIM_LOG_DBG,"%s()=%d error: Fail to get the first data_PLP\r\n", __FUNCTION__, result);
			param->plp_num = 0;
			return ERR_FAILUE; // Other (fatal) error.
		}
	}while (1);
    
	return result;
}
INT32 nim_cxd2387_lock_T_signal(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{

	sony_demod_t * param = (sony_demod_t *)dev->priv;
	if ((!(TRUE == param->search_t2_only)) && need_to_lock_DVBT_signal(dev, change_para, NeedToConfigTuner))
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,try_to_lock_DVBT_signal!\n",__FUNCTION__,__LINE__);
		param->priority = change_para->priority;
		return try_to_lock_DVBT_signal(dev, NeedToInitSystem, NeedToConfigTuner, change_para);
	}
	else
	{
		CXD2837_LOG(NIM_LOG_DBG, "error: [%s][%d]para repetition\r\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
}
INT32 nim_cxd2387_lock_T2_signal(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{
	INT32 result = ERR_FAILUE;
	sony_demod_t * param = (sony_demod_t *)dev->priv;
	if (need_to_lock_DVBT2_signal(dev, change_para, NeedToConfigTuner, &play_program) )
	{
		    
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,try_to_lock_DVBT2_signal play_program =%d \n",__FUNCTION__,__LINE__,play_program);
		if (play_program)
		{
			param->do_not_wait_t2_signal_locked = 1;
			CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,do_not_wait_t2_signal_locked = %d!\n",__FUNCTION__,__LINE__,param->do_not_wait_t2_signal_locked);
			param->plp_index = change_para->plp_index;
			param->t2_profile = change_para->t2_profile;
			result = DVBT2_change_PLP(dev, change_para->plp_id, NeedToInitSystem, NeedToConfigTuner,0);
		}
		else
		{
			if (change_para->usage_type == USAGE_TYPE_NEXT_PIPE_SCAN)
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,USAGE_TYPE_NEXT_PIPE_SCAN \n",__FUNCTION__,__LINE__);
				result = try_to_lock_next_data_plp(dev, NeedToInitSystem, NeedToConfigTuner);
			}
			else
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);
				result = try_to_lock_DVBT2_signal(dev, NeedToInitSystem, NeedToConfigTuner);
				change_para->plp_num = param->plp_num;
				change_para->t2_profile = param->t2_profile;
			}
			change_para->plp_index = param->plp_index;
			change_para->plp_id = param->plp_id;
			change_para->t2_system_id = param->t2_system_id;
		}
	}
	else
	{
		CXD2837_LOG(NIM_LOG_DBG, "error: [%s][%d]para repetition\r\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
	return result;
}
INT32 nim_cxd2387_lock_combo_signal(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{
	INT32 result = ERR_FAILUE;
	sony_demod_t * param = (sony_demod_t *)dev->priv;
	//first lock the T2 signal
	if(SUCCESS ==  nim_cxd2387_lock_T2_signal(dev,change_para))
	{
		return SUCCESS;
	}
	else if (param->autoscan_stop_flag)
	{
		return ERR_FAILUE;
	}
	else if(SUCCESS == nim_cxd2387_lock_T_signal(dev,change_para))
	{
		return SUCCESS;
	}
	else
	{
		CXD2837_LOG(NIM_LOG_DBG, "[%s,line:%d]all signal type lock fail when signal type is combo \r\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
	return result;
}
INT32 nim_cxd2837_channel_change_smart(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{	
	INT32 result = ERR_FAILUE;

	sony_demod_t * param = (sony_demod_t *)dev->priv;
	if ((change_para->freq <= 40000) || (change_para->freq >= 900000))
	{
		return ERR_FAILUE;
	}
	
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);

	if (change_para->bandwidth != SONY_DEMOD_BW_1_7_MHZ && change_para->bandwidth != SONY_DEMOD_BW_5_MHZ
        && change_para->bandwidth != SONY_DEMOD_BW_6_MHZ && change_para->bandwidth != SONY_DEMOD_BW_7_MHZ
        && change_para->bandwidth != SONY_DEMOD_BW_8_MHZ)
	{
	    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d error:bandwidth=%d \n", __FUNCTION__,__LINE__,change_para->bandwidth);
		return ERR_FAILUE;
	}
	
	CXD2837_PRINTF(NIM_LOG_DBG,"%s: usage_type %d, freq %d, bandwidth %d, priority %d, t2_signal %d, plp_index %d, plp_id  %d, profile %d\r\n",
                __FUNCTION__,change_para->usage_type, change_para->freq,change_para->bandwidth, change_para->priority, 
                change_para->t2_signal, change_para->plp_index, change_para->plp_id, change_para->t2_profile);

	//change_para->usage_type = 0;
	param->autoscan_stop_flag = 0;
	param->do_not_wait_t2_signal_locked = ( change_para->usage_type == USAGE_TYPE_AERIALTUNE ? 1:0 );

	if ( need_to_config_tuner(dev, change_para->freq, change_para->bandwidth) )
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,need_to_config_tuner!\r\n",__FUNCTION__,__LINE__);
		if(param->bandwidth != change_para->bandwidth)
			NeedToInitSystem = TRUE;
		param->Frequency = change_para->freq;
		param->bandwidth = change_para->bandwidth;
		NeedToConfigTuner = TRUE;
	}
	//all signal mode just be  decided by this para: param->sys_signal_type
	switch(change_para->t2_signal)
	{
		case DVBT_TYPE:
			result = nim_cxd2387_lock_T_signal(dev,change_para);
			break;
		case DVBT2_TYPE:
			result = nim_cxd2387_lock_T2_signal(dev,change_para);
			break;
		case DVBT2_COMBO:
			result = nim_cxd2387_lock_combo_signal(dev,change_para);
			break;
		default:
			CXD2837_PRINTF(NIM_LOG_DBG,"%d is unknown signal mode\n",__FUNCTION__,__LINE__,change_para->t2_signal);
			break;
	}
	CXD2837_PRINTF(NIM_LOG_DBG,"%s %d the current signal_type = %d\n",__FUNCTION__,__LINE__,param->t2_signal);
    change_para->t2_signal = param->t2_signal;
	
    return result;
}

/*INT32 nim_cxd2837_channel_change_smart(struct nim_device *dev,NIM_CHANNEL_CHANGE_T *change_para)
{	
	INT32 result = ERR_FAILUE;

	sony_demod_t * param = (sony_demod_t *)dev->priv;
	BOOL play_program, NeedToInitSystem = FALSE, NeedToConfigTuner = FALSE;
	
	
	if ((change_para->freq <= 40000) || (change_para->freq >= 900000))
	{
		return ERR_FAILUE;
	}
	
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);

	if (change_para->bandwidth != SONY_DEMOD_BW_1_7_MHZ && change_para->bandwidth != SONY_DEMOD_BW_5_MHZ
        && change_para->bandwidth != SONY_DEMOD_BW_6_MHZ && change_para->bandwidth != SONY_DEMOD_BW_7_MHZ
        && change_para->bandwidth != SONY_DEMOD_BW_8_MHZ)
	{
	    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d error:bandwidth=%d \n", __FUNCTION__,__LINE__,change_para->bandwidth);

		return ERR_FAILUE;
	}
	
	#if 0
	result = osal_flag_wait(&flgptn, param->flag_id, NIM_SCAN_END, OSAL_TWF_ANDW|OSAL_TWF_CLR, 2000); //OSAL_WAIT_FOREVER_TIME
	if(OSAL_E_OK != result)
	{
		return ERR_FAILUE;
	}
    #endif
	CXD2837_PRINTF(NIM_LOG_DBG,"%s: usage_type %d, freq %d, bandwidth %d, priority %d, t2_signal %d, plp_index %d, plp_id  %d, profile %d\r\n",
                __FUNCTION__,change_para->usage_type, change_para->freq,change_para->bandwidth, change_para->priority, 
                change_para->t2_signal, change_para->plp_index, change_para->plp_id, change_para->t2_profile);

	//change_para->usage_type = 0;
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]line: %d,search_t2_only = %d\n",__FUNCTION__,__LINE__,param->search_t2_only);

	do
	{
		//osal_mutex_lock(param->demodMode_mutex_id, OSAL_WAIT_FOREVER_TIME);
		param->autoscan_stop_flag = 0;
		param->do_not_wait_t2_signal_locked = ( change_para->usage_type == USAGE_TYPE_AERIALTUNE ? 1:0 );

		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);

		if ( need_to_config_tuner(dev, change_para->freq, change_para->bandwidth) )
		{
		    
			CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,need_to_config_tuner!\r\n",__FUNCTION__,__LINE__);

			if(param->bandwidth != change_para->bandwidth)
				NeedToInitSystem = TRUE;
			param->Frequency = change_para->freq;
			param->bandwidth = change_para->bandwidth;
			NeedToConfigTuner = TRUE;
		}
	    //search T2 signal only when search_t2_only is true
		if ((!(TRUE == param->search_t2_only)) && need_to_lock_DVBT_signal(dev, change_para, NeedToConfigTuner))
		{
		   CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,try_to_lock_DVBT_signal!\n",__FUNCTION__,__LINE__);

			param->priority = change_para->priority;
			result = try_to_lock_DVBT_signal(dev, NeedToInitSystem, NeedToConfigTuner, change_para);
			if (result == SUCCESS)
			{
			    break;
			}
			else
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,try_to_lock_DVBT_signal failed!\n",__FUNCTION__,__LINE__);
			}
		}

		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);

		if (param->autoscan_stop_flag)
		{
		    break;
		}

		if (need_to_lock_DVBT2_signal(dev, change_para, NeedToConfigTuner, &play_program) )
		{
		    
			CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,try_to_lock_DVBT2_signal play_program =%d \n",__FUNCTION__,__LINE__,play_program);

			if (play_program)
			{
				param->do_not_wait_t2_signal_locked = 1;

				CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,do_not_wait_t2_signal_locked = %d!\n",__FUNCTION__,__LINE__,param->do_not_wait_t2_signal_locked);

				param->plp_index = change_para->plp_index;
				param->t2_profile = change_para->t2_profile;
				result = DVBT2_change_PLP(dev, change_para->plp_id, NeedToInitSystem, NeedToConfigTuner,0);
			}
			else
			{
				if (change_para->usage_type == USAGE_TYPE_NEXT_PIPE_SCAN)
				{
				    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,USAGE_TYPE_NEXT_PIPE_SCAN \n",__FUNCTION__,__LINE__);
					result = try_to_lock_next_data_plp(dev, NeedToInitSystem, NeedToConfigTuner);
				}
				else
				{
				    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);
					result = try_to_lock_DVBT2_signal(dev, NeedToInitSystem, NeedToConfigTuner);
					change_para->plp_num = param->plp_num;
					change_para->t2_profile = param->t2_profile;
				}
				change_para->plp_index = param->plp_index;
				change_para->plp_id = param->plp_id;
				change_para->t2_system_id = param->t2_system_id;
			}
		}
    }while (0);
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,param->system);
    change_para->t2_signal = param->t2_signal;
	
    return result;
}
*/
UINT8 cxd2837_modulation_map_to_ali_modulation(sony_dtv_system_t system, UINT8 modulation)
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

INT32 nim_cxd2837_get_modulation(struct nim_device *dev, UINT8 *modulation)
{
    sony_result_t result;
    sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_dvbt2_l1pre_t L1Pre;
    sony_dvbt2_plp_t plpInfo;
    sony_dvbt_tpsinfo_t tps;
    *modulation = 0;
	

    switch (pDemod->system) {
	    case SONY_DTV_SYSTEM_DVBT2:
	        result = sony_demod_dvbt2_CheckDemodLock(pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        result = sony_demod_dvbt_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBC:
	        result = sony_demod_dvbc_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED){
        return ERR_FAILUE;
    }
    if (pDemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
        sony_demod_dvbt2_monitor_L1Pre(pDemod, &L1Pre);//sony_dvb_demod_monitorT2_L1Pre
        if (result != SONY_RESULT_OK){ 
            return ERR_FAILUE;
        }
        // Get Active PLP information. 
        result = sony_demod_dvbt2_monitor_ActivePLP(pDemod, SONY_DVBT2_PLP_DATA, &plpInfo);//sony_dvb_demod_monitorT2_ActivePLP
        if (result != SONY_RESULT_OK || plpInfo.constell > SONY_DVBT2_QAM256 ){
            return ERR_FAILUE;
        }
        if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
        {
            if (plpInfo.id != pDemod->plp_id)
            {
                CXD2837_PRINTF(NIM_LOG_DBG,"%s(): plp_id=%d, plpInfo.id=%d, error PLP locked.\r\n", __FUNCTION__,  pDemod->plp_id, plpInfo.id);
				return ERR_FAILUE;
            }

            *modulation = cxd2837_modulation_map_to_ali_modulation(pDemod->system, plpInfo.constell);
            return SONY_RESULT_OK;
        }
    }
    else
    {
        result = sony_demod_dvbt_monitor_TPSInfo(pDemod, &tps);//sony_dvb_demod_monitorT_TPSInfo
        if (result != SONY_RESULT_OK || tps.constellation >= SONY_DVBT_CONSTELLATION_RESERVED_3 ){ 
            return ERR_FAILUE;
        }
        *modulation = cxd2837_modulation_map_to_ali_modulation(pDemod->system, tps.constellation);
    }
    return SUCCESS;
}


UINT8 cxd2837_FEC_map_to_ali_FEC(sony_dtv_system_t system, UINT8 fec)
{
    //T_NODE:	UINT16 FEC_inner			: 4;
    //T: 0:1/2, 1:2/3, 2:3/4, 3:5/6, 4:7/8  //T2: 5:3/5, 6:4/5 //0xF:unknown
    
   if (system == SONY_DTV_SYSTEM_DVBT)
    {
        switch (fec)
        {
            case SONY_DVBT_CODERATE_1_2:
                return 0;
            case SONY_DVBT_CODERATE_2_3:
                return 1;
            case SONY_DVBT_CODERATE_3_4:
                return 2;
            case SONY_DVBT_CODERATE_5_6:
                return 3;
            case SONY_DVBT_CODERATE_7_8:
                return 4;
            default:
                return 0xF;   //unknown.
        }
    }
    else
    {
        switch (fec)
        {
			case SONY_DVBT2_R1_2:
				return 0;
			case SONY_DVBT2_R2_3:
				return 1;
			case SONY_DVBT2_R3_4:
				return 2;
			case SONY_DVBT2_R5_6:
				return 3;
			case SONY_DVBT2_R3_5:
				return 5;
			case SONY_DVBT2_R4_5:
				return 6;
			case SONY_DVBT2_R1_3: 
				return 7;
			case SONY_DVBT2_R2_5:
				return 8;
			default:
			return 0xF;   //unknown.
        }
    }
}

INT32 nim_cxd2837_get_FEC(struct nim_device *dev, UINT8* FEC)
{
    sony_result_t result;
    sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_dvbt2_l1pre_t L1Pre;
    sony_dvbt2_plp_t plpInfo;
    sony_dvbt_tpsinfo_t tps;

    *FEC = 0;
	

    switch (pDemod->system) {
	    case SONY_DTV_SYSTEM_DVBT2:
	        result = sony_demod_dvbt2_CheckDemodLock(pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        result = sony_demod_dvbt_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBC:
	        result = sony_demod_dvbc_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED){
        return ERR_FAILUE;
    }
    if (pDemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
        sony_demod_dvbt2_monitor_L1Pre(pDemod, &L1Pre);//sony_dvb_demod_monitorT2_L1Pre
        if (result != SONY_RESULT_OK){ 
            return ERR_FAILUE;
        }
        // Get Active PLP information. 
        result = sony_demod_dvbt2_monitor_ActivePLP(pDemod, SONY_DVBT2_PLP_DATA, &plpInfo);//sony_dvb_demod_monitorT2_ActivePLP
        if (result != SONY_RESULT_OK){ 
            return ERR_FAILUE;
        }
        if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
        {
            if (plpInfo.id != pDemod->plp_id)
            {
                CXD2837_PRINTF(NIM_LOG_DBG,"%s(): plp_id=%d, plpInfo.id=%d, error PLP locked.\r\n", __FUNCTION__,  pDemod->plp_id, plpInfo.id);
				return ERR_FAILUE;
            }

            *FEC = cxd2837_FEC_map_to_ali_FEC(pDemod->system, plpInfo.plpCr);
            return SUCCESS;
        }
    }
    else
    {
        result = sony_demod_dvbt_monitor_TPSInfo(pDemod, &tps);//sony_dvb_demod_monitorT_TPSInfo
        if (result != SONY_RESULT_OK || tps.constellation >= SONY_DVBT_CONSTELLATION_RESERVED_3 ){
             return ERR_FAILUE;
        }
        *FEC = cxd2837_FEC_map_to_ali_FEC(pDemod->system, tps.rateHP);
    }
    return SUCCESS;
}

UINT8 cxd2837_gi_map_to_ali_gi(sony_dtv_system_t system, UINT8 guard_interval)
{
    //T_NODE:	UINT32 guard_interval : 8; 	
    //4: 1/4, 8: 1/8, 16: 1/16, 32:1/32  //T2: 128:1/128, (19+128):19/128, 19:19/256

    if (system == SONY_DTV_SYSTEM_DVBT)
    {
        switch (guard_interval)
        {
            case SONY_DVBT_GUARD_1_32:
                return 32;
            case SONY_DVBT_GUARD_1_16:
                return 16;
            case SONY_DVBT_GUARD_1_8:
                return 8;
            case SONY_DVBT_GUARD_1_4:
                return 4;
            default:
                return 0xFF;   //unknown.
        }
    }
    else
    {
        switch (guard_interval)
        {
            case SONY_DVBT2_G1_32:
                return 32;
            case SONY_DVBT2_G1_16:
                return 16;
            case SONY_DVBT2_G1_8:
                return 8;
            case SONY_DVBT2_G1_4:
                return 4;
            case SONY_DVBT2_G1_128:
                return 128;
            case SONY_DVBT2_G19_128:
                return (19+128);
            case SONY_DVBT2_G19_256:
                return 19;
            default:
                return 0xFF;   //unknown.
        }
    }
}

INT32 nim_cxd2837_get_GI(struct nim_device *dev, UINT8 *guard_interval)
{
    sony_result_t result;
    sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_dvbt2_ofdm_t ofdm;
    sony_dvbt_mode_t fft_mode_t;
    sony_dvbt_guard_t gi_t;
    
    *guard_interval = 0;

    switch (pDemod->system) {
	    case SONY_DTV_SYSTEM_DVBT2:
	        result = sony_demod_dvbt2_CheckDemodLock(pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        result = sony_demod_dvbt_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBC:
	        result = sony_demod_dvbc_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED){
        return ERR_FAILUE;
    }

    if (pDemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
        result = sony_demod_dvbt2_monitor_Ofdm(pDemod, &ofdm);//sony_dvb_demodT2_OptimizeMISO
        if (result != SONY_RESULT_OK) 
        {
            return ERR_FAILUE;
        }
        *guard_interval = cxd2837_gi_map_to_ali_gi(pDemod->system, ofdm.gi);
    }
    else
    {
        result = sony_demod_dvbt_monitor_ModeGuard(pDemod, &fft_mode_t, &gi_t);//sony_dvb_demod_monitorT_ModeGuard       
        if (result != SONY_RESULT_OK) 
        {
            return ERR_FAILUE;
        }
        *guard_interval = cxd2837_gi_map_to_ali_gi(pDemod->system, gi_t);;
    }
	 return SUCCESS;
}

UINT8 cxd2837_fft_mode_map_to_ali_fft_mode(sony_dtv_system_t system, UINT8 fft_mode)
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
                return ERR_FAILUE;   //unknown.
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
                return ERR_FAILUE;   //unknown.
        }
    }
	return SUCCESS;
}

INT32 nim_cxd2837_get_fftmode(struct nim_device *dev, UINT8 *fft_mode)
{
    sony_result_t result;
    sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_dvbt2_ofdm_t ofdm;
    sony_dvbt_mode_t fft_mode_t;
    sony_dvbt_guard_t gi_t;
  
    *fft_mode = 0;

    switch (pDemod->system) {
	    case SONY_DTV_SYSTEM_DVBT2:
	        result = sony_demod_dvbt2_CheckDemodLock(pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBT:
	        result = sony_demod_dvbt_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_DVBC:
	        result = sony_demod_dvbc_CheckDemodLock (pDemod, &lock);
	        break;
	    case SONY_DTV_SYSTEM_UNKNOWN:
	    default:
	        result = SONY_RESULT_OK;
    }
    if (result != SONY_RESULT_OK || lock != SONY_DEMOD_LOCK_RESULT_LOCKED){
        return ERR_FAILUE;
    }

    if (pDemod->system == SONY_DTV_SYSTEM_DVBT2) 
    {
        result = sony_demod_dvbt2_monitor_Ofdm(pDemod, &ofdm);//sony_dvb_demodT2_OptimizeMISO
        if (result != SONY_RESULT_OK) 
        {
            return ERR_FAILUE;
        }
        *fft_mode = cxd2837_fft_mode_map_to_ali_fft_mode(pDemod->system, ofdm.mode);
    }
    else
    {
        result = sony_demod_dvbt_monitor_ModeGuard(pDemod, &fft_mode_t, &gi_t);//sony_dvb_demod_monitorT_ModeGuard       
        if (result != SONY_RESULT_OK) 
        {
            return ERR_FAILUE;
        }
        *fft_mode = cxd2837_fft_mode_map_to_ali_fft_mode(pDemod->system, fft_mode_t);
    }
	return SUCCESS;
}

INT32 nim_cxd2837_get_freq(struct nim_device *dev, UINT32 *freq)
{
    sony_demod_t * priv = (sony_demod_t *)dev->priv;
    *freq = priv->Frequency;
    return SUCCESS;
}

INT32 nim_cxd2837_channel_change(
	struct nim_device *dev, 
	UINT32 freq, 
	UINT32 bandwidth, 
	UINT8 guard_interval, 
	UINT8 fft_mode, 
	UINT8 modulation, 
	UINT8 fec, 
	UINT8 usage_type, 
	UINT8 inverse,
	UINT8 priority)
{
	NIM_CHANNEL_CHANGE_T change_para;

	memset(&change_para, 0, sizeof(NIM_CHANNEL_CHANGE_T));

	change_para.freq = freq;

	change_para.bandwidth = bandwidth;

	change_para.guard_interval = guard_interval;
		
	change_para.fft_mode = fft_mode;
		
	change_para.modulation = modulation;

	change_para.fec = fec;

	change_para.usage_type = usage_type;

	change_para.inverse = inverse;

	change_para.inverse = priority;		
	
	return nim_cxd2837_channel_change_smart(dev, &change_para);
}

INT32 nim_cxd2837_get_SSI(struct nim_device *dev, UINT8 *data)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	sony_result_t CalcResult = SONY_RESULT_OK;
	uint32_t  ssi=0;
	static uint32_t PreSSI =0;

	uint32_t rfLevel;
	
	//result = osal_flag_wait(&flgptn, priv->flag_id, NIM_SCAN_END, OSAL_TWF_ANDW, 0);

	//if(OSAL_E_OK==result)
		//!get rfLevel first
	if (priv->tuner_control.nim_tuner_command != NULL)
	{
		if (priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_GET_RF_POWER_LEVEL, (UINT32)&rfLevel) == ERR_FAILUE )
		{
			CXD2837_PRINTF(NIM_LOG_DBG,"error: Tuner does not support command 'NIM_TUNER_GET_RF_POWER_LEVEL'.\r\n"); 
			return ERR_FAILUE;
		}
	}
	//!Get SSI Value
	CalcResult = sony_integ_CalcSSI(priv, &ssi, rfLevel);
	if(SONY_RESULT_OK == CalcResult){
		*data = ssi;
		PreSSI = ssi;//save this data
		//CXD2837_PRINTF(NIM_LOG_DBG,"CalcSSI: ssi=%d\r\n",ssi); 
	}
	else{
		*data = PreSSI;//use the pre-value
		PreSSI = PreSSI/2;
		CXD2837_PRINTF(NIM_LOG_DBG,"error: CalcSSI failure,use pre value!\r\n");
	}
	return SUCCESS;
}

INT32 nim_cxd2837_get_SQI(struct nim_device *dev, UINT8 *snr)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	sony_result_t CalcResult = SONY_RESULT_OK;
	uint8_t  quality=0;
	static uint32_t PreQuality =0;

	
	CalcResult = sony_integ_CalcSQI(priv, &quality);
	if(SONY_RESULT_OK == CalcResult)
	{
		*snr = quality;
		PreQuality = quality;//save this data
		CXD2837_PRINTF(NIM_LOG_DBG,"CalcSQI: sqi=%d\r\n",quality);
	}
	else
	{
		*snr = PreQuality;//use the pre-value
		PreQuality = PreQuality / 2;
		CXD2837_PRINTF(NIM_LOG_DBG,"error: CalcSQI failure,use pre value!\r\n");
	}

	return SUCCESS;

}

INT32 nim_cxd2837_get_BER(struct nim_device *dev, UINT32 *BER)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	sony_result_t CalcResult = SONY_RESULT_OK;
	uint32_t  ber;
	static uint32_t PreBer;
	
	{
		//osal_mutex_lock(priv->demodMode_mutex_id, OSAL_WAIT_FOREVER_TIME);
		CalcResult = sony_integ_CalcBER(priv, &ber);
		if(SONY_RESULT_OK == CalcResult){
			*BER = ber;
			PreBer = ber;
			CXD2837_PRINTF(NIM_LOG_DBG,"CalcBER: BER=%d\r\n",ber);
		}
		else{
			*BER = PreBer;
			PreBer =  PreBer/2;
			CXD2837_PRINTF(NIM_LOG_DBG,"error: CalcBER failure, use pre value!\r\n");
		}

	}
	return SUCCESS;
}

INT32 nim_cxd2837_get_lock(struct nim_device *dev, UINT8 *lock)
{
    sony_demod_t * priv = (sony_demod_t *)dev->priv;
    sony_demod_lock_result_t demod_lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;//DEMOD_LOCK_RESULT_NOTDETECT
    sony_result_t s_ret;
    *lock = 0;
    s_ret = sony_integ_demod_CheckTSLock(priv, &demod_lock);
    if ( SONY_DEMOD_LOCK_RESULT_LOCKED == demod_lock )
    {
        *lock = 1;
    }
	
    return SUCCESS;
}
/*****************************************************************************
* INT32 nim_cxd2834_close(struct nim_device *dev)
* Description: cxd2834 close
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_cxd2837_close(struct nim_device *dev)
{
    //UINT8 data;
    sony_demod_t * priv = (sony_demod_t *)dev->priv;
    INT32 result = SUCCESS;

    // tuner power off
    if (priv->tuner_control.nim_tuner_command != NULL)
    {
    	priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);
    }
    //tun_cxd_ascot3_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);

    result = cxd2837_demod_Shutdown(priv);//sony_dvb_demod_Finalize
    if (result != SONY_RESULT_OK)
        result = ERR_FAILUE;

    nim_cxd2837_switch_lock_led(dev, FALSE);
    return result;
}


__ATTRIBUTE_REUSE_
INT32 nim_cxd2837_open(struct nim_device *dev)
{
    sony_demod_t * priv = (sony_demod_t *)dev->priv;
    struct COFDM_TUNER_CONFIG_API * config_info = &(priv->tuner_control);
    //sony_dvbt2_tune_param_t  TuneParam;
    sony_result_t result = SONY_RESULT_OK;
    if((NULL == dev) || (NULL == config_info))
    {
		CXD2837_PRINTF(NIM_LOG_DBG,"nim_device is null\n");
		return SONY_RESULT_ERROR_ARG;
	}
	// Initialize the demod. 
    result = sony_demod_InitializeT_C(priv);//sony_demod_InitializeT_C
    if (result != SONY_RESULT_OK)
    {
        CXD2837_PRINTF(NIM_LOG_DBG,"sony_dvb_demod_Initialize() error: %d.\r\n", result);
        return result;
    }
    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,priv->system);
    //Demod can be config now.
    //Demod shall can access tuner through the I2C gateway.
    // Initialize the tuner.
    if (priv->tuner_control.nim_tuner_command != NULL)
    {
    	priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, FALSE); //tuner power on
    }
    //tun_cxd_ascot3_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, FALSE); //tuner power on


    //Config TS output mode: SSI/SPI. Default is SPI.	
	//result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_PARALLEL_SEL, ((NIM_COFDM_TS_SSI == priv->tc.ts_mode)?0:1) );
	result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_PARALLEL_SEL, 0 );
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure DEMOD_CONFIG_PARALLEL_SEL. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}
    //Confif Data output pin: 0:TS0(pin13), 1:TS7(pin20), default:1.
	//result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_SER_DATA_ON_MSB, ((NIM_COFDM_TS_SSI == priv->tc.ts_mode)?0:1) );
	result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_SER_DATA_ON_MSB, 0 );
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure DEMOD_CONFIG_PARALLEL_SEL. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}

    //TS Error output.
	result = sony_demod_SetConfig(priv,SONY_DEMOD_CONFIG_TSERR_ACTIVE_HI, 1);//DEMOD_CONFIG_TSERR_ENABLE
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure DEMOD_CONFIG_TSERR_ENABLE. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}

	/* IFAGC setup. Modify to suit connected tuner. */
	/* IFAGC: 0 for positive and 1 for negtive*/
#ifdef TUNER_IFAGCPOS
	result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_IFAGCNEG, 0);
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure IFAGCNEG. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}
#else
	result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_IFAGCNEG, 1);
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure IFAGCNEG. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}
#endif

	/*IFAGC ADC range[0-2]     0 : 1.4Vpp, 1 : 1.0Vpp, 2 : 0.7Vpp*/   
	result = sony_demod_SetConfig(priv,SONY_DEMOD_CONFIG_IFAGC_ADC_FS, 0);//DEMOD_CONFIG_TSERR_ENABLE
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure SONY_DEMOD_CONFIG_IFAGC_ADC_FS. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}

	//Ben Debug 140221#1
	//add by AEC for TS error enable 2013-09-09
	 // TSERR output enable from GPIO2 pin
	result = sony_demod_GPIOSetConfig(priv, 2, 1, SONY_DEMOD_GPIO_MODE_TS_ERROR);
	if(result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error in sony_demod_GPIOSetConfig for TS error.\n");
		return result;
	}
	//end for TS error enable 2013-09-09  
	
	/* Spectrum Inversion setup. Modify to suit connected tuner. */
	/* Spectrum inverted, value = 1. */
#ifdef TUNER_SPECTRUM_INV
	result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_SPECTRUM_INV, 1);
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure SPECTRUM_INV. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}
#endif

    /* RFAIN ADC and monitor enable/disable. */
    /* Default is disabled. 1: Enable, 0: Disable. */
#ifdef RFAIN_ADC_ENABLE
        result = sony_demod_SetConfig(priv, SONY_DEMOD_CONFIG_RFAIN_ENABLE, 0);
        if (result == SONY_RESULT_OK) {
            CXD2837_PRINTF(NIM_LOG_DBG,"Demodulator configured to enable RF level monitoring.\n");
        }
        else {
            CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure RFLVMON_ENABLE. (result = %d)\n", result);
            return -1;
        }
#endif

	/* RF level monitoring (RFAIN/RFAGC) enable/disable. */
	/* Default is enabled. 1: Enable, 0: Disable. */
#ifdef TUNER_RFLVLMON_DISABLE
	result = sony_demod_SetConfig(priv, DEMOD_CONFIG_RFLVMON_ENABLE, 0);
	if (result != SONY_RESULT_OK)
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Unable to configure RFLVMON_ENABLE. (status=%d, %s)\r\n", result, FormatResult(result));
		return result;
	}
#endif
    //osal_flag_set(priv->flag_id, NIM_SCAN_END);
    return SUCCESS;
}
INT32 nim_cxd2837_get_RF_LEVEL(struct nim_device *dev, INT32 *rf_level)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	sony_result_t CalcResult = SONY_RESULT_OK;
	//UINT32 flgptn;/*clean warning unused variable*/
	int32_t   p_rflevel = 0;
	//if (priv->tuner_control.nim_tuner_command(dev, NIM_TUNER_GET_RF_POWER_LEVEL, (UINT32)&p_rflevel) == ERR_FAILUE )
	/*clean warning dev shoule be _u32 typr*/
	if (priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_GET_RF_POWER_LEVEL, (UINT32)&p_rflevel) == ERR_FAILUE )
	{
		CXD2837_LOG(priv, "error: Tuner does not support command 'NIM_TUNER_GET_RF_POWER_LEVEL'.\r\n"); 
		return ERR_FAILUE;
	}
	//,we will init this para :priv->tunerOptimize when nim initialize,just use sony tuner then deal with
	if(priv->tunerOptimize ==  SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2D  ||
       priv->tunerOptimize ==  SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2E  ||
       priv->tunerOptimize ==  SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2XR ||
       priv->tunerOptimize ==  SONY_DEMOD_TUNER_OPTIMIZE_ASCOT3)
	{
		CalcResult = sony_integ_CalcRF_LEVEL(priv, &p_rflevel);
		if(SONY_RESULT_OK == CalcResult)
		{
			*rf_level = p_rflevel;
			CXD2837_LOG(priv, ":get_RF_LEVEL = %d !\r\n",(int)*rf_level);
		}
		else
		{
			*rf_level = 0;
			CXD2837_LOG(priv, "error:get_RF_LEVEL fail 1 !\r\n");
			return ERR_FAILUE;
		}
	}
	/*sony demod don't support to get rf_level from other tuner in addition to sony tuner,so we use  direct the rf_level from tuner*/
	else
	{
		*rf_level = p_rflevel;
	}
	return SUCCESS;
}
INT32 nim_cxd2837_get_SNR(struct nim_device *dev, UINT32 *snr)
{
	sony_demod_t * priv = (sony_demod_t *)dev->priv;
	sony_result_t CalcResult = SONY_RESULT_OK;

	uint32_t p_snr;
	//UINT32 flgptn;/*clean warning unused variable*/
	CalcResult = sony_integ_CalcSNR(priv, &p_snr);
	if(SONY_RESULT_OK == CalcResult){
		*snr = p_snr;
		CXD2837_LOG(priv, "get_SNR: SNR=%d\r\n",(int)snr);
	}
	else
	{
		*snr = 0;
		CXD2837_LOG(priv, "error: get_SNR failure 1,!\r\n");
		return ERR_FAILUE;
	}
		return SUCCESS;
	
}



