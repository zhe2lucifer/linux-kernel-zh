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
#ifndef _TMBSL_HCDP_ADVANCED_H
#define _TMBSL_HCDP_ADVANCED_H

#ifdef __cplusplus
extern "C" {
#endif  /*__cplusplus*/

/*============================================================================*/
/* Types and defines:                                                         */
/*============================================================================*/

typedef enum _HCDP_ConstellationSource
{
    HCDP_ConstellationSource_ADC = 0,
    HCDP_ConstellationSource_FEDR,
    HCDP_ConstellationSource_PDF,
    HCDP_ConstellationSource_DAGC,
    HCDP_ConstellationSource_MF,
    HCDP_ConstellationSource_CAGC,
    HCDP_ConstellationSource_Equalizer,
    HCDP_ConstellationSource_BEDR /* default */
} HCDP_ConstellationSource_t;

/*============================================================================*/
/* Exported functions:                                                        */
/*============================================================================*/    
    
tmErrorCode_t
tmbslHCDP_SetClocks(
    tmUnitSelect_t tUnit,
    UInt32 uSamplingClock,
    UInt32 uDClock
);

tmErrorCode_t
tmbslHCDP_GetClocks(
    tmUnitSelect_t tUnit,
    UInt32 *puSamplingClock,
    UInt32 *puDClock
);

tmErrorCode_t
tmbslHCDP_SetIFAGCThreshold(
    tmUnitSelect_t tUnit,
    UInt16 uIFAGCThr
);

tmErrorCode_t
tmbslHCDP_GetIFAGCThreshold(
    tmUnitSelect_t tUnit,
    UInt16 *puIFAGCThr
);

tmErrorCode_t
tmbslHCDP_SetRFAGCThreshold(
    tmUnitSelect_t tUnit,
    UInt16 uRFAGCThrLow,
    UInt16 uRFAGCThrHigh
);

tmErrorCode_t
tmbslHCDP_ConstPreset(
    tmUnitSelect_t tUnit,
    HCDP_ConstellationSource_t eSource
);

tmErrorCode_t
tmbslHCDP_ConstGetIQ(
    tmUnitSelect_t tUnit,
    tmFrontEndFracNb32_t *pI,
    tmFrontEndFracNb32_t *pQ
);

tmErrorCode_t
tmbslHCDP_FFTPreset(
    tmUnitSelect_t tUnit
);

tmErrorCode_t
tmbslHCDP_FFTGetPoint(
    tmUnitSelect_t tUnit,
    tmFrontEndFracNb32_t  *pY
);

tmErrorCode_t
tmbslHCDP_EqualizerPreset(
    tmUnitSelect_t tUnit
);

tmErrorCode_t
tmbslHCDP_EqualizerGetPoint(
    tmUnitSelect_t tUnit,
    UInt8 uTapIndex,
    tmFrontEndFracNb32_t *pI,
    tmFrontEndFracNb32_t *pQ
);

tmErrorCode_t
tmbslHCDP_GetSignalLevel(
    tmUnitSelect_t tUnit,
    UInt16 *puLevel
);

#ifdef __cplusplus
}
#endif  /*__cplusplus*/

#endif /*_TMBSL_HCDP_ADVANCED_H*/
