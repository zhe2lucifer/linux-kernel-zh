/* 
* NXP TDA18250 silicon tuner driver 
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


#ifndef _TMBSL_TDA18250_ADVANCED_CUSTOM_H
#define _TMBSL_TDA18250_ADVANCED_CUSTOM_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* Exported advanced custom functions:                                        */
/*============================================================================*/

extern tmErrorCode_t
tmbslTDA18250SetAGC1
(
    tmUnitSelect_t           tUnit,     /* I: Unit number */
    tmddTDA18250_AGC1_Gain_t agc1Gain   /* I: AGC1 gain   */
);

extern tmErrorCode_t
tmbslTDA18250GetAGC1
(
    tmUnitSelect_t            tUnit,     /* I: Unit number */
    tmddTDA18250_AGC1_Gain_t* pAgc1Gain  /* O: AGC1 gain   */
);

extern tmErrorCode_t
tmbslTDA18250SetAGC2b_Gain
(
    tmUnitSelect_t  tUnit,     /* I: Unit number */
    UInt8           agc2Gain   /* I: AGC2 gain   */
);

extern tmErrorCode_t
tmbslTDA18250GetAGC2b_Gain
(
    tmUnitSelect_t  tUnit,     /* I: Unit number */
    UInt8*          pAgc2Gain  /* O: AGC2 gain   */
);

extern tmErrorCode_t
tmbslTDA18250SetAGC2b_Raw
(
    tmUnitSelect_t  tUnit,     /* I: Unit number */
    UInt8            Agc2Raw   /* I: AGC2 gain   */
);

extern tmErrorCode_t
tmbslTDA18250GetAGC2b_Raw
(
    tmUnitSelect_t  tUnit,     /* I: Unit number */
    UInt8            *pAgc2Raw   /* I: AGC2 gain   */
);


extern tmErrorCode_t
tmbslTDA18250SetAGC1Freeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
);


extern tmErrorCode_t
tmbslTDA18250SetAGC1Unfreeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
);

extern tmErrorCode_t
tmbslTDA18250SetAGC2Unfreeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
);

extern tmErrorCode_t
tmbslTDA18250SetAGC2Freeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
);
extern tmErrorCode_t
tmbslTDA18250SetLoFreq
(
    tmUnitSelect_t  tUnit,      /* I: Unit number      */
    UInt32          uRF         /* I: Frequency in Hz  */
);

extern tmErrorCode_t
tmbslTDA18250GetVCO
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Int32*          puVCO       /* O: VCO in KHz  */
);

#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_TDA18250_ADVANCED_CUSTOM_H */
