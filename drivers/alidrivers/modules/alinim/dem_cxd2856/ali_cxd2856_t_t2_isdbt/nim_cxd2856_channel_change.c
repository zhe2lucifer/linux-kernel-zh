/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/04/06          leo.liu           v0.1                  create channel change function       
----------------------------------------------------------------------------*/
#include "nim_cxd2856_common.h"

static int need_to_check_t_info(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para)
{
	sony_integ_t 		*priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	*user = priv->user;
	UINT8 				lock = 0;

	NIM_TRACE_ENTER();
	if(user->Frequency == change_para->freq &&
	   user->bandwidth == change_para->bandwidth &&
	   nim_cxd2856_get_lock(dev,&lock) == SUCCESS)
	{
		if(1 == lock) //current status ok,don't channel change again
		{
			NIM_TRACE_RETURN_SUCCESS();
		}
		else
		{
			NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
		}
		
	}
	else
	{
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
	}
}
static int need_to_check_t2_info(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	
	NIM_TRACE_ENTER();
	if( change_para->usage_type == USAGE_TYPE_AUTOSCAN \
	        || change_para->usage_type == USAGE_TYPE_CHANSCAN \
	        || change_para->usage_type == USAGE_TYPE_AERIALTUNE \
	        || change_para->usage_type == USAGE_TYPE_NEXT_PIPE_SCAN \
	        || change_para->usage_type == USAGE_TYPE_CHANCHG )
	{	
		 return TRUE;    //Auto detect signal type for Auto Scan and Channel Scan.
	}
	
	else if(user->t2_profile != change_para->t2_profile)
	{
		return TRUE;
	}
    return FALSE;
}

static INT32 Dvbt2_tune_unknown_profile(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para)
{
	sony_result_t 			tuneResult = SONY_RESULT_OK;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 		* user = priv->user;
	sony_dvbt2_profile_t 	t2ProfileTuned = SONY_DVBT2_PROFILE_ANY;//blind tune only
	
	NIM_TRACE_ENTER();
	tuneResult = sony_integ_dvbt2_BlindTune(priv,change_para->freq,change_para->bandwidth,SONY_DVBT2_PROFILE_ANY,&t2ProfileTuned);
	switch(tuneResult)
	{
        case SONY_RESULT_OK:
			user->t2_profile = t2ProfileTuned; //get current profile
			
			pr_info("[%s]:DVB-T2 TS Locked and get profile is %s %d.\n",__FUNCTION__, \
			((t2ProfileTuned == SONY_DVBT2_PROFILE_BASE)? "SONY_DVBT2_PROFILE_BASE":"SONY_DVBT2_PROFILE_LITE"),t2ProfileTuned);
	        return SUCCESS;
        case SONY_RESULT_ERROR_UNLOCK:
			pr_err("[%s]:DVB-T2 TS Unlocked on unknown_profile mode.\n",__FUNCTION__);
	        return ERR_FAILUE;
        case SONY_RESULT_ERROR_TIMEOUT:
			pr_err("[%s]:DVB-T2 Wait TS Lock but Timeout on unknown_profile mode.\n",__FUNCTION__);
	        return ERR_FAILUE;
        default:
			pr_err("[%s]:Error in sony_integ_dvbt2_Tune on unknown_profile mode\n",__FUNCTION__);
	        return ERR_FAILUE;
    }
}

static INT32 Dvbt2_tune_plp(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	sony_result_t 			tuneResult = SONY_RESULT_OK;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
	sony_dvbt2_tune_param_t tuneParam;

	NIM_TRACE_ENTER();
	memset(&tuneParam,0,sizeof(tuneParam));
	
	tuneParam.centerFreqKHz = change_para->freq;
	tuneParam.bandwidth		= change_para->bandwidth;
	tuneParam.dataPlpId 	= change_para->plp_id; //selected data PLP ID. 
	tuneParam.profile 		= change_para->t2_profile;;
	tuneParam.tuneInfo 		= SONY_DEMOD_DVBT2_TUNE_INFO_OK;

	pr_info("[%s]:PLP_id(%d),tune_mode(TUNE).\n",__FUNCTION__,change_para->plp_id);
	tuneResult = sony_integ_dvbt2_Tune(priv, &tuneParam);
	switch(tuneResult){
        case SONY_RESULT_OK:
			pr_info("[%s]:DVB-T2 TS Locked on plp_ID %d.\n",__FUNCTION__,tuneParam.dataPlpId);
	        return SUCCESS;
        case SONY_RESULT_ERROR_UNLOCK:
			pr_err("[%s]:DVB-T2 TS Unlocked on plp_ID %d.\n",__FUNCTION__,tuneParam.dataPlpId);
	        return ERR_FAILUE;
        case SONY_RESULT_ERROR_TIMEOUT:
			pr_err("[%s]:DVB-T2 Wait TS Lock but Timeout on plp_ID %d.\n",__FUNCTION__,tuneParam.dataPlpId);
	        return ERR_FAILUE;
        default:
			pr_err("[%s]:Error in sony_integ_dvbt2_Tune on plp_ID %d\n",__FUNCTION__,tuneParam.dataPlpId);
	        return ERR_FAILUE;
    }

}

static int Dvbt2_to_lock_next_data_plp(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para)
{
	sony_result_t 			tuneResult = SONY_RESULT_OK;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 		* user = priv->user;

	NIM_TRACE_ENTER();
	
	if(change_para->plp_index >= user->plp_num)
	{
		pr_err("[err] plp_index : %d, plp_nu : %d\n",change_para->plp_index,user->plp_num);
		NIM_TRACE_RETURN_ERROR("plp_index beyond plp_id sum",ERR_FAILUE);
	}
	
	change_para->plp_id = user->all_plp_id[change_para->plp_index];//back the plp_id
	tuneResult = Dvbt2_tune_plp(dev, change_para);//tune this pid for app to get the info of program
	if(SONY_RESULT_OK != tuneResult)
	{
		NIM_TRACE_RETURN_ERROR("tune plp_id err",ERR_FAILUE);
	}
	pr_info("\t[plp_index :%d, plp_id :%d]\n",change_para->plp_index,change_para->plp_id);
	NIM_TRACE_RETURN_SUCCESS();
	#if 0
    for (plp_index = user->plp_index+1; plp_index < user->plp_num; ++plp_index )
    {
        if (plp_index >= user->plp_num)
        {
            NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
        }
        result = Dvbt2_channel_change(dev, user->all_plp_id[plp_index],TUNE);
        
        for ( retry=0; retry<30; ++retry )
        {
            SONY_SLEEP (30);
            if (user->autoscan_stop_flag)
                NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);            
            if (user->do_not_wait_t2_signal_locked)
                NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
            
            result = sony_demod_dvbt2_monitor_L1Pre(pdemod, &L1Pre);//sony_dvb_demod_monitorT2_L1Pre
            if (result != SONY_RESULT_OK) 
            {
                pr_err( "%s(plp_num=%d, all_plp_id[%d]=%d) error: sony_dvb_demod_monitorT2_L1Pre()=%d \r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index],result);
                continue;
				//return result;
            }
        
            // Get Active PLP information. 
            result = sony_demod_dvbt2_monitor_ActivePLP(pdemod, SONY_DVBT2_PLP_DATA, &plpInfo);//sony_dvb_demod_monitorT2_ActivePLP
            if (result != SONY_RESULT_OK) 
            {
				pr_err( "%s(plp_num=%d, all_plp_id[%d]=%d) error: sony_dvb_demod_monitorT2_ActivePLP()=%d \r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index],result);
				continue;
				//return result;
            }
            
            if (result == SONY_RESULT_OK) 
            {
                if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
                {
                    if (plpInfo.id != user->all_plp_id[plp_index])
                    {
                        pr_err( "%s(plp_num=%d, all_plp_id[%d]=%d), plpInfo.id=%d, error PLP locked: retry %d times.\r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index], plpInfo.id, retry);
                        continue;
                    }
                    else
                        break; //correct PLP is locked.
                }
            }
        }

        if (result == SONY_RESULT_OK  && (plpInfo.id == user->all_plp_id[plp_index]) ) 
        {
            if (plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA1 || plpInfo.type == SONY_DVBT2_PLP_TYPE_DATA2)
            {
                user->plp_id = plpInfo.id;
                user->t2_system_id = L1Pre.systemId;
                user->plp_index = plp_index;
                
                if (retry!=0)
                    pr_info( "%s(plp_num=%d, all_plp_id[%d]=%d), ok: retry %d times.\r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index], retry);
                return SONY_RESULT_OK;
            }
            else
            {
                    pr_err( "%s(plp_num=%d, all_plp_id[%d]=%d), ok: retry %d times. error: Not DataPLP: (type=%d, id=%d)\r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index], retry, plpInfo.type, plpInfo.id);
            }
        }
        else
        {
            pr_err( "%s(plp_num=%d, all_plp_id[%d]=%d), error: fail to lock the PLP.\r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index]);
        }
    }
	pr_info( "%s  plp_num=%d, all_plp_id[%d]=%d\r\n", __FUNCTION__, user->plp_num, plp_index, user->all_plp_id[plp_index]);
    NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
	#endif
}
static int Dvbt2_to_search_plp_sum(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para)
{	
	UINT16 waitTime = 0;
	INT32 					result = ERR_FAILUE;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
	sony_demod_t 			* pdemod = priv->pDemod;
	ALI_CXD2856_DATA_T 		* user = priv->user;
	
	NIM_TRACE_ENTER();
	user->plp_num = 0;
	user->plp_id = 0;
	user->plp_index = 0;
        
  	result = Dvbt2_tune_unknown_profile(dev,change_para);
	memset(user->all_plp_id,0,sizeof(user->all_plp_id)/sizeof(user->all_plp_id[0])); //clear array
	//get_the_first_data_PLP_info
	do
	{
		result = sony_demod_dvbt2_monitor_DataPLPs(pdemod,(user->all_plp_id), &(user->plp_num));//sony_dvb_demod_monitorT2_DataPLPs
		if (result == SONY_RESULT_OK) 
		{
			UINT8 plp_idx;
			pr_info( "\t[%s]: plp_sum=%d\n ", __FUNCTION__, user->plp_num);
			for (plp_idx=0; plp_idx < user->plp_num; ++plp_idx)
			{
				pr_info( "\t[plp_id=%d]\n", user->all_plp_id[plp_idx]);
			}
			user->plp_id = user->all_plp_id[0];//return first valid plp_id
			break;
		}
		else if (result == SONY_RESULT_ERROR_HW_STATE) 
		{
			if (waitTime >= DTV_DEMOD_TUNE_T2_L1POST_TIMEOUT)
			{
				user->plp_num = 0;
				NIM_TRACE_RETURN_ERROR("timeout for get the first data_PLP",ERR_FAILUE);
			}
			else 
			{
				SONY_SLEEP (10); //10
				waitTime += 10; 
			}
		} 
		else 
		{
			user->plp_num = 0;
			NIM_TRACE_RETURN_ERROR("Fail to get the first data_PLP",ERR_FAILUE); // Other (fatal) error.
		}
	}while (1);
    
	return result;
}
static int nim_cxd2856_lock_t2_signal(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	int 				result = SONY_RESULT_OK;
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	
	NIM_TRACE_ENTER();
	if (need_to_check_t2_info(dev, change_para))
	{
		switch(change_para->usage_type)
		{
			case USAGE_TYPE_AERIALTUNE: //!!!!only for tds os,linux os don't call this case
				result = Dvbt2_tune_unknown_profile(dev,change_para);
				change_para->t2_profile = user->t2_profile;	
				break;
			case USAGE_TYPE_AUTOSCAN://step 1: to search the sum of plp_id and return the first plpid
			case USAGE_TYPE_CHANSCAN:
				pr_info("\n[%s]:change_para->usage_type: USAGE_TYPE_AUTOSCAN\n",__FUNCTION__);
				result = Dvbt2_to_search_plp_sum(dev,change_para);
				change_para->plp_num 	= user->plp_num;
				change_para->t2_profile = user->t2_profile;
				change_para->plp_id 	= user->plp_id;//return the first plpid
				break;
			case USAGE_TYPE_NEXT_PIPE_SCAN:// setp 2: to return all the remaining plp_id
				pr_info("\n[%s]:plp_index:%d;change_para->usage_type: USAGE_TYPE_NEXT_PIPE_SCAN\n",__FUNCTION__,change_para->plp_index);
				result = Dvbt2_to_lock_next_data_plp(dev,change_para);
				break;
			case USAGE_TYPE_CHANCHG://step 3:to channel change
				pr_info("\n[%s]:plp_id:%d;change_para->usage_type: USAGE_TYPE_CHANCHG\n",__FUNCTION__,change_para->plp_id);
				result = Dvbt2_tune_plp(dev, change_para);//only to tune current chennnel
				break;
			default:
				pr_err("\n[%s]:change_para->usage_type is error: %d\n",__FUNCTION__,change_para->usage_type);
				return ERR_FAILUE;
		}

	}
	else
	{
		NIM_TRACE_RETURN_SUCCESS();
	}
	return result;
}
static int nim_cxd2856_lock_t_signal(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	int 					result = SONY_RESULT_OK;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
	sony_dvbt_tune_param_t 	tuneParam;

	NIM_TRACE_ENTER();
	if(SUCCESS == need_to_check_t_info(dev, change_para))
	{
		pr_info( "%d %s freq is the same as last time,and still lock successfully,so return!",__LINE__,__FUNCTION__);
		NIM_TRACE_RETURN_SUCCESS();
	}
	
    /* Configure the DVBT tune parameters based on the channel requirements */
    tuneParam.centerFreqKHz = change_para->freq;    /* Channel centre frequency in KHz */
    tuneParam.bandwidth = change_para->bandwidth;   /* Channel bandwidth */
    //tuneParam.profile = SONY_DVBT_PROFILE_HP;       /* Channel profile for hierachical modes.  For non-hierachical use HP */
	tuneParam.profile = change_para->priority;

	/* Perform DVB-T Tune */
    result = sony_integ_dvbt_Tune (priv, &tuneParam);
    if (result != SONY_RESULT_OK) {
        NIM_TRACE_RETURN_ERROR("sony_integ_dvbt_Tune failed\r",ERR_FAILUE);
    }
	pr_info("[%s]:DVB-T TS Locked.\n",__FUNCTION__);
	NIM_TRACE_RETURN_SUCCESS();

}
static int nim_cxd2856_lock_isdbt_signal(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para)
{
	int 					result = SONY_RESULT_OK;
	sony_integ_t 			* priv = (sony_integ_t *)dev->priv;
    sony_isdbt_tune_param_t tuneParam;

	NIM_TRACE_ENTER();
	if(SUCCESS == need_to_check_t_info(dev, change_para))
	{
		pr_info( "%d %s freq is the same as last time,and still lock successfully,so return!",__LINE__,__FUNCTION__);
		NIM_TRACE_RETURN_SUCCESS();
	}
	
    /* Configure the isdbt tune parameters based on the channel requirements */
    tuneParam.centerFreqKHz = change_para->freq;    /* Channel centre frequency in KHz */
    tuneParam.bandwidth = change_para->bandwidth;   /* Channel bandwidth */

    /* Perform ISDB-T Tune */
    result = sony_integ_isdbt_Tune (priv, &tuneParam);
    if (result != SONY_RESULT_OK) {
		NIM_TRACE_RETURN_ERROR("sony_integ_isdbt_Tune failed\r",ERR_FAILUE);
    }
	NIM_TRACE_RETURN_SUCCESS();
}
static int nim_cxd2856_lock_combo_signal(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	//if system don't issue the signal type,need to try signal type,only t/t2 neerd to try,first try T2 type,isdbt don't need,
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;

	NIM_TRACE_ENTER();
	//according to current market,first try T2 mode
	if(SONY_RESULT_OK == nim_cxd2856_lock_t2_signal(dev,change_para))
	{
		user->t2_signal = DVBT2_TYPE;//t2
	}
	else if(SONY_RESULT_OK == nim_cxd2856_lock_t_signal(dev,change_para))
	{
		user->t2_signal = DVBT_TYPE;//t
	}
	else
	{
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);//try fail
	}
	NIM_TRACE_RETURN_SUCCESS();
}

int nim_cxd2856_channel_change(struct nim_device *dev, struct NIM_CHANNEL_CHANGE *change_para)
{
	int 				result = ERR_FAILUE;
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	NIM_TRACE_ENTER();
	
	if(NULL == dev || NULL == change_para)
	{
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);//
	}
	if ((change_para->freq <= 40000) || (change_para->freq >= 900000))
	{
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);//
	}
	if (change_para->bandwidth != SONY_DTV_BW_1_7_MHZ && change_para->bandwidth != SONY_DTV_BW_5_MHZ
        && change_para->bandwidth != SONY_DTV_BW_6_MHZ && change_para->bandwidth != SONY_DTV_BW_7_MHZ
        && change_para->bandwidth != SONY_DTV_BW_8_MHZ)
	{
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);//
	}

	pr_info("\nchange_para->t2_signal = %d freq = %d bw = %d\n",change_para->t2_signal,change_para->freq,change_para->bandwidth);
	switch(change_para->t2_signal)
	{
		case DVBT_TYPE://dvbt
			result = nim_cxd2856_lock_t_signal(dev,change_para);
			break;
		case DVBT2_TYPE://dvbt2
			result = nim_cxd2856_lock_t2_signal(dev,change_para);
			break;
		case ISDBT_TYPE://isdbt
			result = nim_cxd2856_lock_isdbt_signal(dev,change_para);
			break;
		case DVBT2_COMBO://combo
			result = nim_cxd2856_lock_combo_signal(dev,change_para);
			if(SUCCESS == result)// try success
			{
				change_para->t2_signal = user->t2_signal; //pass system signal back to the user app
			}
			break;
		default:
			break;
	}
	if(SUCCESS == result || result == SONY_RESULT_OK)
	{
		//if lock successfully,update the current info
		user->bandwidth = change_para->bandwidth;
		user->Frequency = change_para->freq;
		NIM_TRACE_RETURN_SUCCESS();
	}
	else
	{
		//if lock fail,clear saved info 
		user->bandwidth = user->bandwidth-1;
		user->Frequency = user->Frequency-1;
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);////unlock
	}
}


