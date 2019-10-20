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
*    File:    nim_mxl214c.c
*
*    Description:    Source file in LLD.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.20140717       alan      Ver 0.1       Create file.
*****************************************************************************/
#include "nim_mxl214c.h"

#include "MxL_HRCLS_FW.h"



static NIM_MXL214C_AP_CALLBACK        g_ptr_callback = NULL;



/**************************************************************************************
  Example code to wait for tuner lock
 *************************************************************************************/
static MXL_STATUS_E _mxl_waitfortunerlock(UINT8 devid,MXL_HRCLS_TUNER_ID_E tunerid/*TODO: timeout*/)
{
  MXL_STATUS_E             status = 0;
  MXL_HRCLS_TUNER_STATUS_E lockstatus = MXL_HRCLS_TUNER_DISABLED;
  
  do
  {
    MxLWare_HRCLS_OEM_DelayUsec(1000); // TODO: interval to be determined by BE

    status = MxLWare_HRCLS_API_ReqTunerLockStatus(devid, tunerid, &lockstatus);
  } while ((status == MXL_SUCCESS) && (lockstatus != MXL_HRCLS_TUNER_LOCKED));      // TODO: add timeout checking

  return ((status == MXL_SUCCESS) && (lockstatus == MXL_HRCLS_TUNER_LOCKED))?MXL_SUCCESS:MXL_FAILURE;
}

/**************************************************************************************
  Example code to wait for DFE channel lock
 *************************************************************************************/
static MXL_STATUS_E _mxl_waitforchannellock(UINT8 devid,MXL_HRCLS_CHAN_ID_E chanid/*TODO: timeout*/)
{
  MXL_STATUS_E status = 0;
  MXL_HRCLS_CHAN_STATUS_E lockstatus = MXL_HRCLS_CHAN_DISABLED;
  
  do
  {
    MxLWare_HRCLS_OEM_DelayUsec(1000);   // TODO: interval to be determined by BE

    status = MxLWare_HRCLS_API_ReqTunerChanStatus(devid, chanid, &lockstatus);
  } while ((status == MXL_SUCCESS) && (lockstatus != MXL_HRCLS_CHAN_LOCKED));      // TODO: add timeout checking

  return ((status == MXL_SUCCESS) && (lockstatus == MXL_HRCLS_CHAN_LOCKED))?MXL_SUCCESS:MXL_FAILURE;
}



INT32 nim_mxl214c_hw_init(struct nim_device *dev)
{
    MXL_STATUS_E    status = MXL_SUCCESS;
    UINT8           dev_id = 0; 
    MXL_HRCLS_DEV_VER_T devverinfo;
    UINT32          pre_tick = 0;
	UINT32          cur_tick = 0;
    UINT8           chan_id = 0;
    MXL_BOOL_E      demod_enable = FALSE;
	struct nim_mxl214c_private *priv = NULL;
    
	if(dev == NULL)
    {
	    return RET_FAILURE;
	}	
    priv = (struct nim_mxl214c_private *) dev->priv;
	
  	
    pre_tick = osal_get_tick();
    if(dev == NULL)
    {
        status = MXL_INVALID_PARAMETER;
        goto F_EXIT;
    }
    dev_id = priv->dev_idx;
	chan_id = priv->dev_idx;
 

    /************************************************************************************
    Driver initialization. Call this API function before any other function.
    Pass NULL as oemDataPtr as no extra data needs to be provided to OEM function.
    Device (devid) must be enumerated by customer software. devid enumaration must 
    always start from 0.    
    ************************************************************************************/
    //oemData.i2cAddress = HRCLS_I2C_ADDRESS; 
    
    status = MxLWare_HRCLS_API_CfgDrvInit(dev_id, NULL, MXL_HRCLS_DEVICE_214);

	if(status == MXL_ALREADY_INITIALIZED)
	{
	   MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Driver already initialization\n", dev_id);
       return SUCCESS;
	}
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Driver initialization FAILED\n", dev_id);
        goto F_EXIT;
    }

    /************************************************************************************
    Perform hardware reset of the device.
    ************************************************************************************/
    status = MxLWare_HRCLS_API_CfgDevReset(dev_id);
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Device reset FAILED\n", dev_id);
        goto F_EXIT;
    }

  /************************************************************************************
    Capacitor value is don't care.
   ************************************************************************************/
    status = MxLWare_HRCLS_API_CfgDevXtalSetting(dev_id, HRCLS_XTAL_CAP_VALUE);
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Xtal setting FAILED\n", dev_id);
        goto F_EXIT;
    }

    status = MxLWare_HRCLS_API_CfgTunerDsCalDataLoad(dev_id);
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Calibration data not available. Power reporting will not be accurate\n", dev_id);
    }

    /************************************************************************************
    Check MxLWare, Firmware and Bootloader version.
    ************************************************************************************/
    status = nim_mxl214c_checkversion(dev_id,&devverinfo);
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Version checking FAILED!\n", dev_id);
        goto F_EXIT;
    }

    /************************************************************************************
    Make sure firmware is not already downloaded
    ************************************************************************************/
    if (devverinfo.firmwareDownloaded == MXL_TRUE)
    {
        // Something must be wrong, because we just went through reset
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Firmware already running. Forgot about h/w reset?\n", dev_id);
        status = MXL_FAILURE;
        goto F_EXIT;
    }

    /************************************************************************************
    Download firmware
    ************************************************************************************/
    cur_tick = osal_get_tick();
    status = nim_mxl214c_downloadfirmware(dev_id);
    
    MXL214C_PRINTF(NIM_LOG_DBG,"down FW time:%d ms\n",osal_get_tick() - cur_tick);    
    
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Firmware download FAILED\n", dev_id);
        goto F_EXIT;
    }

  /************************************************************************************
    Enable fullband tuner 
   ************************************************************************************/
    status = nim_mxl214c_enablefbtuner(dev_id);
    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Enable FB tuner FAILED\n", dev_id);
        goto F_EXIT;
    }
  
    // This step can be skipped as NO_MUX_4 is a default mode
    status = MxLWare_HRCLS_API_CfgXpt(dev_id, MXL_HRCLS_XPT_MODE_NO_MUX_4); 

    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Cfg XPT mux mode FAILED\n", dev_id);
        goto F_EXIT;
    }

    // Only needed for 4-wire TS mode, skip for 3-wire mode

    status = MxLWare_HRCLS_API_CfgDemodMpegOutGlobalParams(dev_id, 
        MXL_HRCLS_MPEG_CLK_NEGATIVE, MXL_HRCLS_MPEG_DRV_MODE_1X, MXL_HRCLS_MPEG_CLK_56_21MHz);

    if (status != MXL_SUCCESS)
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Global MPEG params setting FAILED\n", dev_id);
        //return status;
        goto F_EXIT;
    }
    
F_EXIT:
    MxLWare_HRCLS_API_ReqDemodEnable(dev_id,chan_id,&demod_enable);
    if(demod_enable == MXL_FALSE)
    {
        MxLWare_HRCLS_API_CfgDemodEnable(dev_id, chan_id, MXL_TRUE);
    }
    cur_tick = osal_get_tick();
    MXL214C_PRINTF(NIM_LOG_DBG,"Open time:%d ms\n", cur_tick - pre_tick);
    return (INT32)status;
	
}




/**************************************************************************************
  Example code for FB tuner config 
 *************************************************************************************/
MXL_STATUS_E nim_mxl214c_enablefbtuner(UINT8 devid)
{
  MXL_STATUS_E status = 0;

  /************************************************************************************
    Enable Fullband tuner
   ************************************************************************************/
  status = MxLWare_HRCLS_API_CfgTunerEnable(devid, MXL_HRCLS_FULLBAND_TUNER);
  if (status != MXL_FAILURE)
  {
    /************************************************************************************
      Wait for FB tuner to lock
     ************************************************************************************/
    status = _mxl_waitfortunerlock(devid,MXL_HRCLS_FULLBAND_TUNER);  
    if (status != MXL_SUCCESS)
    {
      MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Fullband tuner synth lock TIMEOUT!\n", devid, __FUNCTION__);
    }
  }
  else
  {
    MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Cannot enable fullband tuner\n", devid, __FUNCTION__);
  }

  return MXL_SUCCESS;
}
/**************************************************************************************
  Example code for driver (mxlware and firmware) version checking
 *************************************************************************************/
MXL_STATUS_E nim_mxl214c_checkversion(UINT8 devid,/*@out@*/ MXL_HRCLS_DEV_VER_T* verinfoptr)
{
  /************************************************************************************
    Read MxLWare, Firmware and Bootloader version.
   ************************************************************************************/
  MXL_STATUS_E status = MxLWare_HRCLS_API_ReqDevVersionInfo(devid, verinfoptr);

  if (status == MXL_SUCCESS)
  {
    MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] chipVersion=%d MxLWare: %d.%d.%d.%d-RC%d Firmware:%d.%d.%d.%d-RC%d Bootloader:%d.%d.%d.%d fwDownloaded=%d\n",
        devid,
        verinfoptr->chipVersion,
        verinfoptr->mxlWareVer[0],
        verinfoptr->mxlWareVer[1],
        verinfoptr->mxlWareVer[2],
        verinfoptr->mxlWareVer[3],
        verinfoptr->mxlWareVer[4],
        verinfoptr->firmwareVer[0],
        verinfoptr->firmwareVer[1],
        verinfoptr->firmwareVer[2],
        verinfoptr->firmwareVer[3],
        verinfoptr->firmwareVer[4],
        verinfoptr->bootLoaderVer[0],
        verinfoptr->bootLoaderVer[1],
        verinfoptr->bootLoaderVer[2],
        verinfoptr->bootLoaderVer[3],
        verinfoptr->firmwareDownloaded
        );
  }
  return status;
}

/**************************************************************************************
  Example code for firmware download
 *************************************************************************************/
MXL_STATUS_E nim_mxl214c_downloadfirmware(UINT8 devid)
{
    MXL_STATUS_E result = MXL_FAILURE;
	
    /************************************************************************************
     Download firmware. This is a blocking function. Downloading firmware may take
     a while. Actual execution depends also on target platform's I2C speed.
     Define callback function that will be called after every segment is downloaded
     if progress information is required.
    ************************************************************************************/
    result = MxLWare_HRCLS_API_CfgDevFirmwareDownload(devid, sizeof(firmware_array), (UINT8*)&firmware_array[0], NULL);
    if (result == MXL_SUCCESS)
    {
      MXL_HRCLS_DEV_VER_T devverinfo;
      MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Firmware download OK\n", devid, __FUNCTION__);

     /************************************************************************************
       After firmware is successfully downloaded, read its version.
      ************************************************************************************/
      if (MxLWare_HRCLS_API_ReqDevVersionInfo(devid, &devverinfo) == MXL_SUCCESS)
      {
        result = MXL_SUCCESS;
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Firmware ver. %d.%d.%d.%d-RC%d\n", devid, __FUNCTION__,
          devverinfo.firmwareVer[0], devverinfo.firmwareVer[1],
          devverinfo.firmwareVer[2], devverinfo.firmwareVer[3],
          devverinfo.firmwareVer[4]);
      }
      else
      {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Firmware version read FAILED\n", devid, __FUNCTION__);
      }
    }
    else
    {
      MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] {%s} Firmware download FAILED (error code: %d)\n", devid, __FUNCTION__, result);
    }
     
  return result;
}


static MXL_STATUS_E _nim_mxl214_lock_demod(struct nim_mxl214c_private  *priv, 
                                  UINT32 freqkhz, UINT8 bwmhz, MXL_HRCLS_ANNEX_TYPE_E annextype, 
                                  MXL_HRCLS_QAM_TYPE_E qamtype, UINT16 symbolrateksps, MXL_HRCLS_IQ_FLIP_E iqflip)
{
   MXL_STATUS_E                   status = MXL_SUCCESS;
   MXL_HRCLS_XPT_MPEGOUT_PARAM_T  mpegxptparams;
   MXL_HRCLS_OOB_CFG_T            oobParams; 
   UINT32                         devid = 0;
   
   
  MXL214C_PRINTF(NIM_LOG_DBG,"frq:%d,bw:%d,annex:%d,qam:%d,sym:%d,filp:%d\n", freqkhz,bwmhz,annextype,qamtype,symbolrateksps,iqflip);

  NIM_MUTEX_ENTER(priv);

  devid =priv->dev_idx;
  
  // XPT/MPEG configuration for MPEG (not OOB) output only
  if (annextype !=  MXL_HRCLS_ANNEX_OOB)
  {
     // sample MPEG configuration
    mpegxptparams.enable = MXL_ENABLE;
    mpegxptparams.lsbOrMsbFirst = MXL_HRCLS_MPEG_SERIAL_MSB_1ST;//MXL_HRCLS_MPEG_SERIAL_LSB_1ST;
    mpegxptparams.mpegSyncPulseWidth = MXL_HRCLS_MPEG_SYNC_WIDTH_BYTE;//MXL_HRCLS_MPEG_SYNC_WIDTH_BIT;
    mpegxptparams.mpegValidPol = MXL_HRCLS_MPEG_ACTIVE_HIGH;
    mpegxptparams.mpegSyncPol = MXL_HRCLS_MPEG_ACTIVE_HIGH;
    mpegxptparams.mpegClkPol = MXL_HRCLS_MPEG_CLK_POSITIVE;//MXL_HRCLS_MPEG_CLK_NEGATIVE;//MXL_HRCLS_MPEG_CLK_POSITIVE;
    mpegxptparams.clkFreq = MXL_HRCLS_MPEG_CLK_56_21MHz;               // in 4-wire mode, this value has to be the same as the value in MxLWare_HRCLS_API_CfgDemodMpegOutGlobalParams 
    mpegxptparams.mpegPadDrv.padDrvMpegSyn = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;
    mpegxptparams.mpegPadDrv.padDrvMpegDat = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;
    mpegxptparams.mpegPadDrv.padDrvMpegVal = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;

    // In MxL254 and NO_MUX_4 mode, outputId = demodid
    // Check MxLWare API User Guide for other modes' mappings
    status = MxLWare_HRCLS_API_CfgXptOutput(devid, (MXL_HRCLS_XPT_OUTPUT_ID_E) priv->dev_idx, &mpegxptparams);
	
  }

  if (status == MXL_SUCCESS)
  {
    status = MxLWare_HRCLS_API_CfgTunerChanTune(devid, priv->dev_idx, bwmhz, freqkhz * 1000);
    if (status == MXL_SUCCESS)
    {
      status = _mxl_waitforchannellock(devid,priv->dev_idx);
      if (status == MXL_SUCCESS)
      {
        status = MxLWare_HRCLS_API_CfgDemodEnable(devid, priv->dev_idx, MXL_TRUE);
        if (status == MXL_SUCCESS)
        {
          // AdcIqFlip configuration for MPEG (not OOB) output only
          if (annextype !=  MXL_HRCLS_ANNEX_OOB)
          {
            status = MxLWare_HRCLS_API_CfgDemodAdcIqFlip(devid, priv->dev_idx, iqflip);
          }
          if (status == MXL_SUCCESS)
          {
            status = MxLWare_HRCLS_API_CfgDemodAnnexQamType(devid, priv->dev_idx, annextype, qamtype);
            if (status == MXL_SUCCESS)
            {
              if (annextype != MXL_HRCLS_ANNEX_OOB)
              {
                status = MxLWare_HRCLS_API_CfgDemodSymbolRate(devid, priv->dev_idx, symbolrateksps * 1000, symbolrateksps * 1000);
              }
              else
              {
                status = MxLWare_HRCLS_API_CfgDemodSymbolRateOOB(devid, priv->dev_idx, (MXL_HRCLS_OOB_SYM_RATE_E) symbolrateksps);
              }
              if (status == MXL_SUCCESS)
              {
                status = MxLWare_HRCLS_API_CfgDemodRestart(devid, priv->dev_idx);
                if (status != MXL_SUCCESS)
                {
                  MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Demod[%d] restart FAILED \n", devid, priv->dev_idx);
                }
              } 
              else
              { 
                MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Demod[%d] Cfg SymbolRate FAILED \n", devid, priv->dev_idx);
              }
            } 
            else
            {
                MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Demod[%d] Cfg AnnexQamType FAILED \n", devid, priv->dev_idx);
            }
          }
          else
          {
            MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Demod[%d] Cfg IQFlip FAILED \n", devid, priv->dev_idx);
          }
        }
        else
        {
            MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Demod[%d] enable FAILED \n", devid, priv->dev_idx);
        }
      } 
      else
      {
            MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Chan[%d] lock FAILED \n", devid, priv->dev_idx);
      }
    } 
    else
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] Chan[%d] Tune FAILED \n", devid, priv->dev_idx);
    }
  } 
  else 
  {
    MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] %s\n", devid, (annextype != MXL_HRCLS_ANNEX_OOB)?"MPEG output cfg FAILED":"");
  }

  if ((status == MXL_SUCCESS) && (annextype ==  MXL_HRCLS_ANNEX_OOB))
  {
    

    // sample OOB configuration
    oobParams.pn23SyncMode       = SYNC_MODE_ERROR;
    oobParams.pn23Feedback       = SYNC_MODE_ERROR;
    oobParams.syncPulseWidth     = MXL_HRCLS_OOB_SYNC_WIDTH_BYTE;
    oobParams.oob3WireModeEnable = MXL_FALSE;
    oobParams.enablePn23Const    = MXL_FALSE;
    oobParams.validPol           = MXL_HRCLS_OOB_ACTIVE_HIGH;
    oobParams.syncPol            = MXL_HRCLS_OOB_ACTIVE_HIGH;
    oobParams.clkPol             = MXL_HRCLS_OOB_CLK_POSITIVE;
    oobParams.oobOutMode         = OOB_CRX_DRX_MODE;
    oobParams.enable             = MXL_TRUE;

    status = MxLWare_HRCLS_API_CfgDemodOutParamsOOB(devid, priv->dev_idx, &oobParams);
    if (status != MXL_SUCCESS)
    {
      MXL214C_PRINTF(NIM_LOG_DBG,"[HRCLS][%d] OOB Cfg FAILED \n", devid);
    }
  }

  NIM_MUTEX_LEAVE(priv);


  return status;
}


INT32 nim_mxl214c_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T* pst_ch_change)
{
    INT32 ret_code = SUCCESS;
    INT32 demode_mode   = MXL_HRCLS_ANNEX_A;
    INT32 qam_type      = MXL_HRCLS_QAM_AUTO;
    struct nim_mxl214c_private  *priv   = NULL;
    MXL_STATUS_E status = MXL_SUCCESS;
    

    if((dev == NULL) || (pst_ch_change == NULL))
    {
        ret_code = ERR_PARA;
        goto F_EXIT;
    }

    priv= (struct nim_mxl214c_private  *)dev->priv;

    if(priv == NULL)
    {
        ret_code = ERR_PARA;
        goto F_EXIT;
    }

    switch(pst_ch_change->modulation)
    {
        case 4:     ////16QAM
            qam_type = MXL_HRCLS_QAM16;
            break;
        case 5:     ////32QAM
            qam_type = MXL_HRCLS_QAM32;
            break;
        case 6:     ////64QAM
            qam_type = MXL_HRCLS_QAM64;
            break;
        case 7:     ////128QAM
            qam_type = MXL_HRCLS_QAM128;
            break;  
        case 8:     ////256QAM
            qam_type = MXL_HRCLS_QAM256;
            break;
        default:    //other,use auto mode.
            break;
    }

    if(NIM_DVBC_J83B_MODE == (priv->qam_mode & 0x01))
    {
        demode_mode = MXL_HRCLS_ANNEX_B;
    }
    
    status = _nim_mxl214_lock_demod(priv,pst_ch_change->freq*10,8,demode_mode,
        qam_type,pst_ch_change->sym,MXL_HRCLS_IQ_AUTO);    


    if(status != MXL_SUCCESS)
    {
        ret_code = ERR_FAILURE;
    }

   priv->lock_info.freq = pst_ch_change->freq;
   priv->lock_info.symbol_rate = pst_ch_change->sym;
   priv->lock_info.modulation = pst_ch_change->modulation;
   

F_EXIT:
    return ret_code;        
}

INT32 nim_mxl214c_channel_search(struct nim_device *dev, UINT32 freq)
{
    return 0;
}

INT32 nim_mxl214c_get_ber(struct nim_device *dev, UINT32 *err_count)
{
    INT32                    ret_code = SUCCESS;
    MXL_STATUS_E             status = MXL_SUCCESS;    
    MXL_HRCLS_DMD_STAT_CNT_T dmd_stats ;
	struct nim_mxl214c_private  *priv   = NULL;
    
    if((dev != NULL) && (err_count != NULL))
    {
		priv = (struct nim_mxl214c_private  *)(dev->priv);    
		 
        *err_count = 0xFFFFFFFF;
        comm_memset(&dmd_stats,0x00,sizeof(MXL_HRCLS_DMD_STAT_CNT_T));
		
        NIM_MUTEX_ENTER(priv);
        status  = MxLWare_HRCLS_API_ReqDemodErrorStat(priv->dev_idx,priv->dev_idx,&dmd_stats);
        if(status != MXL_SUCCESS)
        {
            ret_code = ERR_FAILURE;
        }
        else
        {
          if(0 != dmd_stats.ReceivedMpeg)
          {
            if(NIM_DVBC_J83B_MODE == (priv->qam_mode & 0x01))
            {
				*err_count = (10*dmd_stats.ErrMpeg)/(188*8*dmd_stats.ReceivedMpeg);
            }
            else
            {
                *err_count = (9*dmd_stats.ErrMpeg)/(188*8*dmd_stats.ReceivedMpeg);
            }
          }
		  else
		  {
			 ret_code = ERR_FAILURE;
		  }
               
        }
        NIM_MUTEX_LEAVE(priv);
    }
    else
    {
        ret_code = ERR_PARA;
    }
    return ret_code;
}
INT32 nim_mxl214c_get_lock(struct nim_device *dev, UINT8 *lock)
{
    INT32 ret_code = SUCCESS;
    MXL_STATUS_E status = MXL_SUCCESS;
    MXL_BOOL_E qamlock = 0;
	MXL_BOOL_E feclock = 0;
	MXL_BOOL_E mpeglock = 0;
	MXL_BOOL_E relock = 0;      
	struct nim_mxl214c_private  *priv= NULL;

    if((dev != NULL) && (lock != NULL))
    {
        priv = (struct nim_mxl214c_private  *)(dev->priv);    
        NIM_MUTEX_ENTER(priv);

        status = MxLWare_HRCLS_API_ReqDemodAllLockStatus(priv->dev_idx,priv->dev_idx, &qamlock, &feclock, &mpeglock, &relock);        
        NIM_MUTEX_LEAVE(priv);
        *lock = (qamlock)?1:0;

        if(status != MXL_SUCCESS)
        {
            ret_code = ERR_FAILURE;
        }
		NIM_MUTEX_LEAVE(priv);
		
    }
    else
    {
        ret_code = ERR_PARA;
    }
    return ret_code;
}

INT32 nim_mxl214c_get_freq(struct nim_device *dev, UINT32 *freq)
{
    INT32 ret_code = SUCCESS;
    struct nim_mxl214c_private  *priv= NULL;
	
    if((dev != NULL) && (freq != NULL))
    {
        priv = (struct nim_mxl214c_private  *)(dev->priv);  
        *freq = priv->lock_info.freq;
    }
    else
    {
        ret_code = ERR_FAILURE;
    }
    return ret_code;
}

INT32 nim_mxl214c_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate)
{
    INT32 ret_code = SUCCESS;
    struct nim_mxl214c_private  *priv= NULL;
	
    if((dev != NULL) && (sym_rate != NULL))
    {
	   priv = (struct nim_mxl214c_private  *)(dev->priv);        
        *sym_rate = priv->lock_info.symbol_rate;

    }
    else
    {
        ret_code = ERR_FAILURE;
    }
    return ret_code;    
}

INT32 nim_mxl214c_get_qam_order(struct nim_device *dev, UINT8 *qam_order)
{
    INT32 ret_code = SUCCESS;
    struct nim_mxl214c_private  *priv= NULL;
	
    if((dev != NULL) && (qam_order != NULL))
    {
	   priv = (struct nim_mxl214c_private  *)(dev->priv);        
        *qam_order = priv->lock_info.modulation;
    }
    else
    {
        ret_code = ERR_FAILURE;
    }
	
    return ret_code;        
}

INT32 nim_mxl214c_get_agc(struct nim_device *dev, UINT8 *agc)
{
    INT32        ret_code = SUCCESS;
    MXL_STATUS_E status = MXL_SUCCESS;
    UINT16       demo_agc = 0;
	struct nim_mxl214c_private  *priv= NULL;
    MXL_HRCLS_RX_PWR_ACCURACY_E accuracy = MXL_HRCLS_PWR_AVERAGED; 
	
    if((dev != NULL) && (agc != NULL))
    {
        priv = (struct nim_mxl214c_private  *)(dev->priv); 
        NIM_MUTEX_ENTER(priv);
        status = MxLWare_HRCLS_API_ReqTunerRxPwr(priv->dev_idx,priv->dev_idx,&demo_agc,&accuracy);
        NIM_MUTEX_LEAVE(priv);
        *agc = (UINT8)(demo_agc) ;

        if(status != MXL_SUCCESS)
        {            
            ret_code = ERR_FAILURE;
        }            
    }
    return ret_code;    
}

INT32 nim_mxl214c_get_snr(struct nim_device *dev, UINT8 *snr)
{
    INT32        ret_code = SUCCESS;
    MXL_STATUS_E status = MXL_SUCCESS;
    UINT16       demo_snr = 0;
	struct nim_mxl214c_private  *priv= NULL;
	
    if((dev != NULL) && (snr != NULL))
    {
        priv = (struct nim_mxl214c_private  *)(dev->priv); 
        NIM_MUTEX_ENTER(priv);       
        status = MxLWare_HRCLS_API_ReqDemodSnr(priv->dev_idx,priv->dev_idx,&demo_snr);
        NIM_MUTEX_LEAVE(priv);
        *snr = (UINT8)(demo_snr) ;

        if(status != MXL_SUCCESS)
        {            
            ret_code = ERR_FAILURE;
        }            
    }
    return ret_code;
}

INT32 nim_mxl214c_get_per(struct nim_device *dev, UINT32 *rsubc)
{
    return 0;
}

INT32 nim_mxl214c_get_rf_level(struct nim_device *dev, UINT16 *rflevel)
{
    return 0;
}

INT32 nim_mxl214c_get_cn_value(struct nim_device *dev, UINT16 *cnvalue)
{
    return 0;
}

void nim_mxl214c_set_qam_type(struct nim_device *dev, NIM_CHANNEL_CHANGE_T* pstchl_change)
{
    
}

void nim_mxl214c_set_retset_proc(NIM_MXL214C_AP_CALLBACK ptr_callback)
{
    g_ptr_callback = ptr_callback;
}

void nim_mxl214c_reset(UINT8 devid)
{
	if(g_ptr_callback!=NULL)
	{
       g_ptr_callback(devid);
	}  
}

