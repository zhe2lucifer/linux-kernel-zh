/* 
* NXP TDA10025 silicon tuner driver 
* 
* Copyright (C) 2016 <insert developer name and email> 
* 
*    This program is free software; you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation; either version 2 of the License, or 
*    (at your option) any later version. 
* 
*    This program is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
* 
*    You should have received a copy of the GNU General Public License along 
*    with this program; if not, write to the Free Software Foundation, Inc., 
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
*/ 

#pragma once
#ifndef _TMBSL_HCDP_H
#define _TMBSL_HCDP_H

#ifdef __cplusplus
extern "C" {
#endif  /*__cplusplus*/


/*============================================================================*/
/* HCDP Error Codes                                                           */
/*============================================================================*/

#define HCDP_ERR_BASE                       (CID_COMP_TUNER | CID_LAYER_BSL)
#define HCDP_ERR_COMP                       (CID_COMP_TUNER | CID_LAYER_BSL | TM_ERR_COMP_UNIQUE_START)

#define HCDP_ERR_BAD_UNIT_NUMBER            (HCDP_ERR_BASE + TM_ERR_BAD_UNIT_NUMBER)
#define HCDP_ERR_ERR_NO_INSTANCES           (HCDP_ERR_BASE + TM_ERR_NO_INSTANCES)
#define HCDP_ERR_NOT_INITIALIZED            (HCDP_ERR_BASE + TM_ERR_NOT_INITIALIZED)
#define HCDP_ERR_ALREADY_SETUP              (HCDP_ERR_BASE + TM_ERR_ALREADY_SETUP)
#define HCDP_ERR_INIT_FAILED                (HCDP_ERR_BASE + TM_ERR_INIT_FAILED)
#define HCDP_ERR_BAD_PARAMETER              (HCDP_ERR_BASE + TM_ERR_BAD_PARAMETER)
#define HCDP_ERR_NOT_SUPPORTED              (HCDP_ERR_BASE + TM_ERR_NOT_SUPPORTED)
#define HCDP_ERR_NULL_CONTROLFUNC           (HCDP_ERR_BASE + TM_ERR_NULL_CONTROLFUNC)
#define HCDP_ERR_HW_FAILED                  (HCDP_ERR_COMP + 0x0001)
#define HCDP_ERR_NOT_READY                  (HCDP_ERR_COMP + 0x0002)
#define HCDP_ERR_BAD_VERSION                (HCDP_ERR_COMP + 0x0003)
#define HCDP_ERR_STD_NOT_SET                (HCDP_ERR_COMP + 0x0004)
#define HCDP_ERR_RF_NOT_SET                 (HCDP_ERR_COMP + 0x0005)

/*============================================================================*/
/* Types and defines:                                                         */
/*============================================================================*/

typedef enum _HCDP_HwCdpSample_t
{
    HCDP_SAMPLE_ES1 = 0,
    HCDP_SAMPLE_ES2,
    HCDP_SAMPLE_UNKNOWN
}HCDP_HwCdpSample_t;

typedef struct _HCDP_LockInd
{
    Bool bCagcSat;
    Bool bDagcSat;
    Bool bAagc;
    Bool bStl;
    Bool bCtl;
    Bool bSctl;
    Bool bDemod;
    Bool bFEC;
    UInt16 uDemodLockTime;
    UInt16 uFecLockTime;
} HCDP_LockInd_t;

typedef enum _HCDP_BERWindow
{
    HCDP_BERWindow_Unknown = 0,
    HCDP_BERWindow_1E5,       /*1e5 bits*/
    HCDP_BERWindow_1E6,       /*1e6 bits*/
    HCDP_BERWindow_1E7,       /*1e7 bits*/
    HCDP_BERWindow_1E8,       /*1e8 bits*/
    HCDP_BERWindow_Max
} HCDP_BERWindow_t;

typedef enum _HCDP_ExtendSymbolRateMode_t /* for extended symbol rate recovery range */
{
    HCDP_ExtendSymbolRateModeDisable,
    HCDP_ExtendSymbolRateModeEnable700ppm,
    HCDP_ExtendSymbolRateModeEnable1500ppm, /* costly in lock time, till 1 sec */
    HCDP_ExtendSymbolRateModeInvalid
} HCDP_ExtendSymbolRateMode_t;

/*============================================================================*/
/* Exported functions:                                                        */
/*============================================================================*/

tmErrorCode_t
tmbslHCDP_Init(
    tmUnitSelect_t tUnit,
    tmbslFrontEndDependency_t *psSrvFunc
);

tmErrorCode_t
tmbslHCDP_DeInit(
    tmUnitSelect_t tUnit
);

tmErrorCode_t     
tmbslHCDP_GetSWVersion(
    ptmSWVersion_t pSWVersion
);

tmErrorCode_t
tmbslHCDP_GetCdpHwSample(
    tmUnitSelect_t tUnit,
    HCDP_HwCdpSample_t *peCdpHwSample
);

tmErrorCode_t
tmbslHCDP_Reset(
    tmUnitSelect_t tUnit
);

tmErrorCode_t
tmbslHCDP_StartLock(
    tmUnitSelect_t tUnit
);

tmErrorCode_t
tmbslHCDP_GetLockIndicators(
    tmUnitSelect_t tUnit,
    HCDP_LockInd_t *psLockInd
);

tmErrorCode_t
tmbslHCDP_GetLockStatus(
    tmUnitSelect_t tUnit,
    tmbslFrontEndState_t *peLockStatus
);

tmErrorCode_t
tmbslHCDP_GetAAGCAcc(
    tmUnitSelect_t tUnit,
    UInt16 *puIfAGC,
    UInt16 *puRFAGC
);

tmErrorCode_t
tmbslHCDP_SetMod(
    tmUnitSelect_t tUnit,
    tmFrontEndModulation_t eMod
);

tmErrorCode_t
tmbslHCDP_GetMod(
    tmUnitSelect_t tUnit,
    tmFrontEndModulation_t *peMod
);

tmErrorCode_t
tmbslHCDP_SetSR(
    tmUnitSelect_t tUnit,
    UInt32 uSR
);

tmErrorCode_t
tmbslHCDP_GetSR(
    tmUnitSelect_t tUnit,
    UInt32 *puSR
);

tmErrorCode_t
tmbslHCDP_GetConfiguredSR(
    tmUnitSelect_t tUnit,
    UInt32 *puSR
);

tmErrorCode_t
tmbslHCDP_SetFECMode(
    tmUnitSelect_t tUnit,
    tmFrontEndFECMode_t eFECMode
);

tmErrorCode_t
tmbslHCDP_GetFECMode(
    tmUnitSelect_t tUnit,
    tmFrontEndFECMode_t *peFECMode
);

tmErrorCode_t
tmbslHCDP_SetSI(
    tmUnitSelect_t tUnit,
    tmFrontEndSpecInv_t eSI
);

tmErrorCode_t
tmbslHCDP_GetSI(
    tmUnitSelect_t tUnit,
    tmFrontEndSpecInv_t *peSI
);

tmErrorCode_t
tmbslHCDP_GetConfiguredSI(
    tmUnitSelect_t tUnit,
    tmFrontEndSpecInv_t *peSI
);

tmErrorCode_t
tmbslHCDP_GetChannelEsNo(
    tmUnitSelect_t tUnit,
    tmFrontEndFracNb32_t  *psEsno
);

tmErrorCode_t
tmbslHCDP_GetBER(
    tmUnitSelect_t tUnit,
    tmFrontEndFracNb32_t *psBER,
    UInt32 *puUncors
);

tmErrorCode_t
tmbslHCDP_SetBERWindow(
    tmUnitSelect_t tUnit,
    HCDP_BERWindow_t eBERWindow
);

tmErrorCode_t
tmbslHCDP_GetBERWindow(
    tmUnitSelect_t tUnit,
    HCDP_BERWindow_t *peBERWindow
);

tmErrorCode_t
tmbslHCDP_GetLockTime(
    tmUnitSelect_t tUnit,
    UInt16 *uDemodLockTime,
    UInt16 *uFecLockTime
);

tmErrorCode_t
tmbslHCDP_GetSignalToISI(
    tmUnitSelect_t tUnit,
    UInt16 *puSignalToISI,
    UInt16 *puEqCenterTapGain
);

tmErrorCode_t
tmbslHCDP_GetSignalToInterference(
    tmUnitSelect_t tUnit,
    UInt16 *puSignalToInterference,
    UInt16 *puEqCenterTapGain
);

tmErrorCode_t
tmbslHCDP_SetIF(
    tmUnitSelect_t tUnit,
    UInt32 uIF
);

tmErrorCode_t
tmbslHCDP_GetConfiguredIF(
    tmUnitSelect_t tUnit,
    UInt32 *puIFFreq
);

tmErrorCode_t
tmbslHCDP_GetIFOffset(
    tmUnitSelect_t tUnit,
    Int32 *plIFOffset
);

tmErrorCode_t
tmbslHCDP_ClearUncor(
    tmUnitSelect_t tUnit
);

tmErrorCode_t
tmbslHCDP_GetSignalQuality(
    tmUnitSelect_t tUnit,
    UInt32 *plSignalQuality
);

tmErrorCode_t
tmbslHCDP_SetExtendSymbolRateRange(
    tmUnitSelect_t tUnit,
    HCDP_ExtendSymbolRateMode_t eMode
);

#ifdef __cplusplus
}
#endif 

#endif /*_TMBSL_HCDP_H*/
