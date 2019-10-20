/*******************************************************************************
 *
 * FILE NAME          : example.hermod.254_214.c
 *
 * AUTHOR             : Mariusz Murawski
 *
 * DATE CREATED       : 03/24/2014
 *
 * DESCRIPTION        : Example code of MxL_HRCLS MxLWare API
 *
 *******************************************************************************
 *                Copyright (c) 2011, MaxLinear, Inc.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <semaphore.h>

#include "MxL_HRCLS_CommonApi.h"
#include "MxL_HRCLS_OEM_Drv.h"



// #define _EXAMPLE_214_

/************************************************************************************
  Device ID used in this sample code.
  If more Hercules devices are used at the same time, they should have
  consecutive.
  This is customer's application responsibility to enumerate devices.
 ************************************************************************************/
#define HRCLS_DEVICE_ID 0

#define HRCLS_EXAMPLE_CODE_DEVICE "MxL254"
#define HRCLS_EXAMPLE_CODE_VERSION "1.10"

/************************************************************************************
  Default carystal capacitors value. This value is platform dependent.
 ************************************************************************************/
#define HRCLS_XTAL_CAP_VALUE 0 
#define HRCLS_I2C_ADDRESS 80

/************************************************************************************
  Firmware filename.
 ************************************************************************************/
#define HRCLS_FIRMWARE_FILENAME "MxL_HRCLS_FW.mbin"

#ifdef MXL_HRCLS_254_ENABLE

static MXL_STATUS_E mxl_waitForTunerLock(MXL_HRCLS_TUNER_ID_E tunerId);
static MXL_STATUS_E mxl_waitForChannelLock(MXL_HRCLS_CHAN_ID_E chanId);


// sample oemData type. Create your own

typedef struct
{
  UINT8 i2cAddress;
  sem_t i2cSem;
} oem_data_t;

oem_data_t oemData;

/**************************************************************************************
  Example code for firmware download
 *************************************************************************************/
static MXL_STATUS_E mxl_downloadFirmware(void)
{
  MXL_STATUS_E result = MXL_FAILURE;
  FILE * file_id;

 /************************************************************************************
   Open firmware file in binary mode
  ************************************************************************************/
  file_id = fopen(HRCLS_FIRMWARE_FILENAME, "rb");
  if (file_id)
  {
    UINT16 firmwareSize;
    UINT8 * firmwareBuf;

    fseek(file_id, 0, SEEK_END);
    firmwareSize = (UINT16) ftell(file_id);
    fseek(file_id, 0, SEEK_SET);

  /************************************************************************************
    Allocate memory buffer of the firmware size and transfer whole firmware image
    to the buffer.
   ************************************************************************************/
    firmwareBuf = (UINT8 *) malloc(firmwareSize);
    if (firmwareBuf)
    {
      if (fread(firmwareBuf, sizeof(UINT8), firmwareSize, file_id) == firmwareSize)
      {
        MXL_STATUS_E status;
       /************************************************************************************
         Download firmware. This is a blocking function. Downloading firmware may take
         a while. Actual execution depends also on target platform's I2C speed.
         Define callback function that will be called after every segment is downloaded
         if progress information is required.
        ************************************************************************************/
        status = MxLWare_HRCLS_API_CfgDevFirmwareDownload(HRCLS_DEVICE_ID, firmwareSize, firmwareBuf, NULL);
        if (status == MXL_SUCCESS)
        {
          MXL_HRCLS_DEV_VER_T devVerInfo;
          printf("[HRCLS][%d] {%s} Firmware download OK\n", HRCLS_DEVICE_ID, __FUNCTION__);

         /************************************************************************************
           After firmware is successfully downloaded, read its version.
          ************************************************************************************/
          if (MxLWare_HRCLS_API_ReqDevVersionInfo(HRCLS_DEVICE_ID, &devVerInfo) == MXL_SUCCESS)
          {
            result = MXL_SUCCESS;
            printf("[HRCLS][%d] {%s} Firmware ver. %d.%d.%d.%d-RC%d\n", HRCLS_DEVICE_ID, __FUNCTION__,
              devVerInfo.firmwareVer[0], devVerInfo.firmwareVer[1],
              devVerInfo.firmwareVer[2], devVerInfo.firmwareVer[3],
              devVerInfo.firmwareVer[4]);
          }
          else
          {
            printf("[HRCLS][%d] {%s} Firmware version read FAILED\n", HRCLS_DEVICE_ID, __FUNCTION__);
          }
        }
        else
        {
          printf("[HRCLS][%d] {%s} Firmware download FAILED (error code: %d)\n", HRCLS_DEVICE_ID, __FUNCTION__, status);
        }
      }
      else
      {
        printf("[HRCLS][%d] {%s} Cannot read %d bytes from successfully open file %s\n", HRCLS_DEVICE_ID, __FUNCTION__, firmwareSize, HRCLS_FIRMWARE_FILENAME);
      }
      free(firmwareBuf);
    }
    else
    {
      printf("[HRCLS][%d] {%s} Error allocating %d bytes of memory\n", HRCLS_DEVICE_ID, __FUNCTION__, firmwareSize);
    }
    fclose(file_id);
  }
  else
  {
    printf("[HRCLS][%d] {%s} Error opening file %s.\n", HRCLS_DEVICE_ID, __FUNCTION__, HRCLS_FIRMWARE_FILENAME);
  }

  return result;
}

/**************************************************************************************
  Example code for driver (mxlware and firmware) version checking
 *************************************************************************************/
static MXL_STATUS_E mxl_checkVersion(/*@out@*/ MXL_HRCLS_DEV_VER_T* verInfoPtr)
{
  /************************************************************************************
    Read MxLWare, Firmware and Bootloader version.
   ************************************************************************************/
  MXL_STATUS_E status = MxLWare_HRCLS_API_ReqDevVersionInfo(HRCLS_DEVICE_ID, verInfoPtr);

  if (status == MXL_SUCCESS)
  {
    printf("[HRCLS][%d] chipVersion=%d MxLWare: %d.%d.%d.%d-RC%d Firmware:%d.%d.%d.%d-RC%d Bootloader:%d.%d.%d.%d fwDownloaded=%d\n",
        HRCLS_DEVICE_ID,
        verInfoPtr->chipVersion,
        verInfoPtr->mxlWareVer[0],
        verInfoPtr->mxlWareVer[1],
        verInfoPtr->mxlWareVer[2],
        verInfoPtr->mxlWareVer[3],
        verInfoPtr->mxlWareVer[4],
        verInfoPtr->firmwareVer[0],
        verInfoPtr->firmwareVer[1],
        verInfoPtr->firmwareVer[2],
        verInfoPtr->firmwareVer[3],
        verInfoPtr->firmwareVer[4],
        verInfoPtr->bootLoaderVer[0],
        verInfoPtr->bootLoaderVer[1],
        verInfoPtr->bootLoaderVer[2],
        verInfoPtr->bootLoaderVer[3],
        verInfoPtr->firmwareDownloaded
        );
  }
  return status;
}

static MXL_STATUS_E mxl_lockDemod(MXL_HRCLS_CHAN_ID_E chanId, MXL_HRCLS_DMD_ID_E demodId, 
                                  UINT32 freqkHz, UINT8 bwMHz, MXL_HRCLS_ANNEX_TYPE_E annexType, 
                                  MXL_HRCLS_QAM_TYPE_E qamType, UINT16 symbolRatekSps, MXL_HRCLS_IQ_FLIP_E iqFlip)
{
  MXL_STATUS_E status = MXL_SUCCESS;
  
  // XPT/MPEG configuration for MPEG (not OOB) output only
  if (annexType !=  MXL_HRCLS_ANNEX_OOB)
  {
    MXL_HRCLS_XPT_MPEGOUT_PARAM_T mpegXptParams;

    // sample MPEG configuration
    mpegXptParams.enable = MXL_ENABLE;
    mpegXptParams.lsbOrMsbFirst = MXL_HRCLS_MPEG_SERIAL_LSB_1ST;
    mpegXptParams.mpegSyncPulseWidth = MXL_HRCLS_MPEG_SYNC_WIDTH_BIT;
    mpegXptParams.mpegValidPol = MXL_HRCLS_MPEG_ACTIVE_HIGH;
    mpegXptParams.mpegSyncPol = MXL_HRCLS_MPEG_ACTIVE_HIGH;
    mpegXptParams.mpegClkPol = MXL_HRCLS_MPEG_CLK_POSITIVE;
    mpegXptParams.clkFreq = MXL_HRCLS_MPEG_CLK_56_21MHz;               // in 4-wire mode, this value has to be the same as the value in MxLWare_HRCLS_API_CfgDemodMpegOutGlobalParams 
    mpegXptParams.mpegPadDrv.padDrvMpegSyn = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;
    mpegXptParams.mpegPadDrv.padDrvMpegDat = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;
    mpegXptParams.mpegPadDrv.padDrvMpegVal = MXL_HRCLS_MPEG_DRV_MODE_2X;  // default setting;

    // In MxL254 and NO_MUX_4 mode, outputId = demodId
    // Check MxLWare API User Guide for other modes' mappings
    status = MxLWare_HRCLS_API_CfgXptOutput(HRCLS_DEVICE_ID, (MXL_HRCLS_XPT_OUTPUT_ID_E) demodId, &mpegXptParams);
  }

  if (status == MXL_SUCCESS)
  {
    status = MxLWare_HRCLS_API_CfgTunerChanTune(HRCLS_DEVICE_ID, chanId, bwMHz, freqkHz * 1000);
    if (status == MXL_SUCCESS)
    {
      status = mxl_waitForChannelLock(chanId);
      if (status == MXL_SUCCESS)
      {
        status = MxLWare_HRCLS_API_CfgDemodEnable(HRCLS_DEVICE_ID, demodId, MXL_TRUE);
        if (status == MXL_SUCCESS)
        {
          // AdcIqFlip configuration for MPEG (not OOB) output only
          if (annexType !=  MXL_HRCLS_ANNEX_OOB)
          {
            status = MxLWare_HRCLS_API_CfgDemodAdcIqFlip(HRCLS_DEVICE_ID, demodId, iqFlip);
          }
          if (status == MXL_SUCCESS)
          {
            status = MxLWare_HRCLS_API_CfgDemodAnnexQamType(HRCLS_DEVICE_ID, demodId, annexType, qamType);
            if (status == MXL_SUCCESS)
            {
              if (annexType != MXL_HRCLS_ANNEX_OOB)
              {
                status = MxLWare_HRCLS_API_CfgDemodSymbolRate(HRCLS_DEVICE_ID, demodId, symbolRatekSps * 1000, symbolRatekSps * 1000);
              }
              else
              {
                status = MxLWare_HRCLS_API_CfgDemodSymbolRateOOB(HRCLS_DEVICE_ID, demodId, (MXL_HRCLS_OOB_SYM_RATE_E) symbolRatekSps);
              }
              if (status == MXL_SUCCESS)
              {
                status = MxLWare_HRCLS_API_CfgDemodRestart(HRCLS_DEVICE_ID, demodId);
                if (status != MXL_SUCCESS)
                {
                  printf("[HRCLS][%d] Demod[%d] restart FAILED \n", HRCLS_DEVICE_ID, demodId);
                }
              } else printf("[HRCLS][%d] Demod[%d] Cfg SymbolRate FAILED \n", HRCLS_DEVICE_ID, demodId);
            } else printf("[HRCLS][%d] Demod[%d] Cfg AnnexQamType FAILED \n", HRCLS_DEVICE_ID, demodId);
          } else printf("[HRCLS][%d] Demod[%d] Cfg IQFlip FAILED \n", HRCLS_DEVICE_ID, demodId);
        } else printf("[HRCLS][%d] Demod[%d] enable FAILED \n", HRCLS_DEVICE_ID, demodId);
      } else printf("[HRCLS][%d] Chan[%d] lock FAILED \n", HRCLS_DEVICE_ID, chanId);
    } else printf("[HRCLS][%d] Chan[%d] Tune FAILED \n", HRCLS_DEVICE_ID, chanId);
  } else printf("[HRCLS][%d] %s\n", HRCLS_DEVICE_ID, (annexType != MXL_HRCLS_ANNEX_OOB)?"MPEG output cfg FAILED":"");

  if ((status == MXL_SUCCESS) && (annexType ==  MXL_HRCLS_ANNEX_OOB))
  {
    MXL_HRCLS_OOB_CFG_T oobParams; 

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

    status = MxLWare_HRCLS_API_CfgDemodOutParamsOOB(HRCLS_DEVICE_ID, demodId, &oobParams);
    if (status != MXL_SUCCESS)
    {
      printf("[HRCLS][%d] OOB Cfg FAILED \n", HRCLS_DEVICE_ID);
    }
  }

  return status;
}

/**************************************************************************************
  Example code to wait for tuner lock
 *************************************************************************************/
static MXL_STATUS_E mxl_waitForTunerLock(MXL_HRCLS_TUNER_ID_E tunerId/*TODO: timeout*/)
{
  MXL_STATUS_E status;
  MXL_HRCLS_TUNER_STATUS_E lockStatus = MXL_HRCLS_TUNER_DISABLED;
  do
  {
    MxLWare_HRCLS_OEM_DelayUsec(1000); // TODO: interval to be determined by BE

    status = MxLWare_HRCLS_API_ReqTunerLockStatus(HRCLS_DEVICE_ID, tunerId, &lockStatus);
  } while ((status == MXL_SUCCESS) && (lockStatus != MXL_HRCLS_TUNER_LOCKED));      // TODO: add timeout checking

  return ((status == MXL_SUCCESS) && (lockStatus == MXL_HRCLS_TUNER_LOCKED))?MXL_SUCCESS:MXL_FAILURE;
}

/**************************************************************************************
  Example code to wait for DFE channel lock
 *************************************************************************************/
static MXL_STATUS_E mxl_waitForChannelLock(MXL_HRCLS_CHAN_ID_E chanId/*TODO: timeout*/)
{
  MXL_STATUS_E status;
  MXL_HRCLS_CHAN_STATUS_E lockStatus = MXL_HRCLS_CHAN_DISABLED;
  do
  {
    MxLWare_HRCLS_OEM_DelayUsec(1000);   // TODO: interval to be determined by BE

    status = MxLWare_HRCLS_API_ReqTunerChanStatus(HRCLS_DEVICE_ID, chanId, &lockStatus);
  } while ((status == MXL_SUCCESS) && (lockStatus != MXL_HRCLS_CHAN_LOCKED));      // TODO: add timeout checking

  return ((status == MXL_SUCCESS) && (lockStatus == MXL_HRCLS_CHAN_LOCKED))?MXL_SUCCESS:MXL_FAILURE;
}

/**************************************************************************************
  Example code for FB tuner config 
 *************************************************************************************/
static MXL_STATUS_E mxl_enableFbTuner(void)
{
  MXL_STATUS_E status;

  /************************************************************************************
    Enable Fullband tuner
   ************************************************************************************/
  status = MxLWare_HRCLS_API_CfgTunerEnable(HRCLS_DEVICE_ID, MXL_HRCLS_FULLBAND_TUNER);
  if (status != MXL_FAILURE)
  {
    /************************************************************************************
      Wait for FB tuner to lock
     ************************************************************************************/
    status = mxl_waitForTunerLock(MXL_HRCLS_FULLBAND_TUNER);  
    if (status != MXL_SUCCESS)
    {
      printf("[HRCLS][%d] {%s} Fullband tuner synth lock TIMEOUT!\n", HRCLS_DEVICE_ID, __FUNCTION__);
    }
  }
  else
  {
    printf("[HRCLS][%d] {%s} Cannot enable fullband tuner\n", HRCLS_DEVICE_ID, __FUNCTION__);
  }

  return MXL_SUCCESS;
}

#if 0
/**************************************************************************************
  Example code for NB tuner disable (also disable all connected channels, IFOUT)
 *************************************************************************************/
static MXL_STATUS_E mxl_disableFbTuner(void)
{
  MXL_STATUS_E status;

  /************************************************************************************
    Turn off FB tuner. 
    This function will disable all DFE channels. 
   ************************************************************************************/
  status = MxLWare_HRCLS_API_CfgTunerDisable(HRCLS_DEVICE_ID, MXL_HRCLS_FULLBAND_TUNER);

  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] {%s} Disable fullband tuner FAILED\n", HRCLS_DEVICE_ID, __FUNCTION__);
  }
  return status;
}

/**************************************************************************************
  Example code for NB tuner disable (also disable all connected channels, IFOUT)
 *************************************************************************************/
static MXL_STATUS_E mxl_disableNbTuner(void)
{
  MXL_STATUS_E status;

  status = MxLWare_HRCLS_API_CfgTunerDisable(HRCLS_DEVICE_ID, MXL_HRCLS_NARROWBAND_TUNER);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] {%s} Disable NB tuner FAILED\n", HRCLS_DEVICE_ID, __FUNCTION__);
  }

  return (MXL_STATUS_E)status;
}
#endif
/**************************************************************************************
   Example code for device initialization, and FB and NB tuner handover
 *************************************************************************************/
int main(int argc, char * argv[])
{
  MXL_STATUS_E status;

  printf("Example code for device: %s (version %s)\n", HRCLS_EXAMPLE_CODE_DEVICE, HRCLS_EXAMPLE_CODE_VERSION);

  MXL_HRCLS_DEV_VER_T devVerInfo;

  argc = argc;  // anti-warning
  argv = argv;  // anti-warning
  /************************************************************************************
    Driver initialization. Call this API function before any other function.
    Pass NULL as oemDataPtr as no extra data needs to be provided to OEM function.
    Device (devId) must be enumerated by customer software. devId enumaration must 
    always start from 0.    
   ************************************************************************************/
  oemData.i2cAddress = HRCLS_I2C_ADDRESS; 
  status = MxLWare_HRCLS_API_CfgDrvInit(HRCLS_DEVICE_ID, (void *) &oemData, MXL_HRCLS_DEVICE_254);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Driver initialization FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }

  /************************************************************************************
    Perform hardware reset of the device.
   ************************************************************************************/
  status = MxLWare_HRCLS_API_CfgDevReset(HRCLS_DEVICE_ID);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Device reset FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }

  /************************************************************************************
    Capacitor value is don't care.
   ************************************************************************************/
  status = MxLWare_HRCLS_API_CfgDevXtalSetting(HRCLS_DEVICE_ID, HRCLS_XTAL_CAP_VALUE);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Xtal setting FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }
  
  status = MxLWare_HRCLS_API_CfgTunerDsCalDataLoad(HRCLS_DEVICE_ID);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Calibration data not available. Power reporting will not be accurate\n", HRCLS_DEVICE_ID);
  }

  /************************************************************************************
    Check MxLWare, Firmware and Bootloader version.
   ************************************************************************************/
  status = mxl_checkVersion(&devVerInfo);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Version checking FAILED!\n", HRCLS_DEVICE_ID);
    return status;
  }

  /************************************************************************************
    Make sure firmware is not already downloaded
   ************************************************************************************/
  if (devVerInfo.firmwareDownloaded == MXL_TRUE)
  {
    // Something must be wrong, because we just went through reset
    printf("[HRCLS][%d] Firmware already running. Forgot about h/w reset?\n", HRCLS_DEVICE_ID);
    return MXL_FAILURE;
  }

  /************************************************************************************
    Download firmware
   ************************************************************************************/
  status = mxl_downloadFirmware();
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Firmware download FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }

  /************************************************************************************
    Enable fullband tuner 
   ************************************************************************************/
  status = mxl_enableFbTuner();
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Enable FB tuner FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }
  
  // This step can be skipped as NO_MUX_4 is a default mode
  status = MxLWare_HRCLS_API_CfgXpt(HRCLS_DEVICE_ID, MXL_HRCLS_XPT_MODE_NO_MUX_4); 

  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Cfg XPT mux mode FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }

  // Only needed for 4-wire TS mode, skip for 3-wire mode
  status = MxLWare_HRCLS_API_CfgDemodMpegOutGlobalParams(HRCLS_DEVICE_ID, MXL_HRCLS_MPEG_CLK_POSITIVE, MXL_HRCLS_MPEG_DRV_MODE_1X, MXL_HRCLS_MPEG_CLK_56_21MHz);
  
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Global MPEG params setting FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }

  /************************************************************************************
    Tune channel#0, lock demod#0, Annex-A, QAM-64
   ************************************************************************************/
  status = mxl_lockDemod(MXL_HRCLS_CHAN0, MXL_HRCLS_DEMOD0, 666000, 8, MXL_HRCLS_ANNEX_A, MXL_HRCLS_QAM64, 6875, MXL_HRCLS_IQ_AUTO);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Demod lock FAILED\n", HRCLS_DEVICE_ID);
    return status;
  }
  MxLWare_HRCLS_OEM_DelayUsec(100000); 
  {
    MXL_BOOL_E qamLock, fecLock, mpegLock, relock;
    status = MxLWare_HRCLS_API_ReqDemodAllLockStatus(HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD0, &qamLock, &fecLock, &mpegLock, &relock);
    if (status != MXL_SUCCESS)
    {
      printf("[HRCLS][%d] Demod[%d] Req all lock status FAILED\n", HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD0);
      return status;
    }
    if ((MXL_TRUE == qamLock) && (MXL_TRUE == fecLock) && (MXL_TRUE == mpegLock) && (MXL_FALSE == relock))
    {
      status = MxLWare_HRCLS_API_CfgUpdateDemodSettings(HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD0);
      if (status != MXL_SUCCESS)
      {
        printf("[HRCLS][%d] Demod[%d] Cfg update demod settings FAILED\n", HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD0);
        return status;
      }
    }

    printf("Demod#%d lock status: QAM:%d, FEC:%d, MPEG:%d, relock:%d\n", MXL_HRCLS_DEMOD0, qamLock, fecLock, mpegLock, relock);
  }


#ifndef _EXAMPLE_214_ // no OOB in 214
  /************************************************************************************
    Tune channel#4, lock demod#4, OOB
   ************************************************************************************/
  status = mxl_lockDemod(MXL_HRCLS_CHAN4, MXL_HRCLS_DEMOD4, 666000, 8, MXL_HRCLS_ANNEX_OOB, MXL_HRCLS_QPSK, (UINT16) MXL_HRCLS_SYM_RATE_0_772MHz, MXL_HRCLS_IQ_AUTO);
  if (status != MXL_SUCCESS)
  {
    printf("[HRCLS][%d] Demod lock failed\n", HRCLS_DEVICE_ID);
    return status;
  }
  MxLWare_HRCLS_OEM_DelayUsec(100000); 
  {
    MXL_BOOL_E qamLock, fecLock, mpegLock, relock;
    status = MxLWare_HRCLS_API_ReqDemodAllLockStatus(HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD4, &qamLock, &fecLock, &mpegLock, &relock);
    if (status != MXL_SUCCESS)
    {
      printf("[HRCLS][%d] Demod[%d] Req all lock status FAILED\n", HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD4);
      return status;
    }    
    if ((MXL_TRUE == qamLock) && (MXL_TRUE == fecLock) && (MXL_TRUE == mpegLock) && (MXL_FALSE == relock))
    {
      status = MxLWare_HRCLS_API_CfgUpdateDemodSettings(HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD4);
      if (status != MXL_SUCCESS)
      {
        printf("[HRCLS][%d] Demod[%d] Cfg update demod settings FAILED\n", HRCLS_DEVICE_ID, MXL_HRCLS_DEMOD4);
        return status;
      }      
    }

    printf("Demod#%d lock status: QAM:%d, FEC:%d, MPEG:%d, relock:%d\n", MXL_HRCLS_DEMOD4, qamLock, fecLock, mpegLock, relock);
  }
#endif // _EXAMPLE_214_

  return MXL_SUCCESS;
}
#endif

