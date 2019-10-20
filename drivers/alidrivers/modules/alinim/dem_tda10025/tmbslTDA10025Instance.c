/* 
* NXP TDA10025 silicon tuner driver 
* 
* Copyright (C) 2016 <V.VRIGNAUD, C.CAZETTES> 
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

#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmbslFrontEndCfgItem.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"

/* TDA10025 Driver includes */
#include "tmbslHCDP.h"

#include "tmbslTDA10025.h"
#include "tmbslTDA10025local.h"

#include "tmbslTDA10025Instance.h"
#include "tmbslTDA10025_Cfg.h"

/* define default configuration */
#define TDA10025_INSTANCE_DEFAULT \
    (tmUnitSelect_t)(-1),                   /* tUnit           */ \
    (tmUnitSelect_t)(-1),                   /* tUnitW          */ \
    (tmUnitSelect_t)(-1),                   /* tUnitCommon     */ \
    (tmUnitSelect_t)(-1),                   /* tUnitOtherStream */ \
    False,                                  /* init            */ \
    {                                       /* sIo             */ \
        Null, \
        Null, \
    }, \
    {                                       /* sTime           */ \
        Null, \
        Null, \
    }, \
    {                                       /* sDebug          */ \
        Null, \
    }, \
    {                                       /* sMutex          */ \
        Null, \
        Null, \
        Null, \
        Null, \
    }, \
    Null,                                   /* pMutex           */ \
    TDA10025_HwSampleVersion_Unknown,       /* eHwSample        */ \
    {                                       /* sIP for HCDP IP  */ \
        tmPowerStandby,                     /* ePowerState      */ \
        0,                                  /* uIFAGCThreshold  */ \
        0,                                  /* uRFAGCThreshold  */ \
        TDA10025_ConstellationSourceUnkown, /* sConstSource     */ \
        False,                              /* bUncorPresent    */ \
    }, \
    TDA10025_ChannelSelection_tUnknown,     /* eChannelSel      */ \
    tmPowerStandby,                         /* ePowerState      */ \
    {                                       /* sPllConfig       */ \
        0,                                  /* uXtal            */ \
        0,                                  /* uPLLMFactor      */ \
        0,                                  /* lPLLNFactor      */ \
        0,                                  /* bPLLPFactor      */ \
    }

/*----------------------------------------------------------------------------*/
/* Global data:                                                               */
/*----------------------------------------------------------------------------*/
TDA10025Object_t gTDA10025Instance[]= 
{
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_0}},
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_1}},
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_2}},
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_3}},
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_4}},
    {TDA10025_INSTANCE_DEFAULT, {TMBSL_TDA10025_INSTANCE_CUSTOM_5}}
};

/* PLL configuration table */
TDA10025PllConfig_t ES1PllSettings[TDA18265_ES1_PLL_CONFIG_NB]= { TDA18265_ES1_PLL_CONFIG };
TDA10025PllConfig_t ES2PllSettings[TDA18265_ES2_PLL_CONFIG_NB] = { TDA18265_ES2_PLL_CONFIG };

/*============================================================================*/
/* FUNCTION:    tmbsliTDA10025_AllocInstance:                                   */
/*                                                                            */
/* DESCRIPTION: allocate new instance                                         */
/*                                                                            */
/* RETURN:                                                                    */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA10025_AllocInstance
(
 tmUnitSelect_t       tUnit,         /* I: Unit number   */
 ppTDA10025Object_t ppDrvObject    /* I: Device Object */
 )
{ 
    tmErrorCode_t       err = TDA10025_ERR_BAD_UNIT_NUMBER;
    pTDA10025Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for(uLoopCounter = 0; uLoopCounter<TDA10025_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA10025Instance[uLoopCounter];
        if(pObj->init == False)
        {
            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            err = iTDA10025_ResetInstance(pObj);

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbsliTDA10025_DeAllocInstance                                */
/*                                                                            */
/* DESCRIPTION: deallocate instance                                           */
/*                                                                            */
/* RETURN:      always TM_OK                                                  */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA10025_DeAllocInstance
(
 tmUnitSelect_t  tUnit    /* I: Unit number */
 )
{     
    tmErrorCode_t       err = TDA10025_ERR_BAD_UNIT_NUMBER;
    pTDA10025Object_t pObj = Null;

    /* check input parameters */
    err = iTDA10025_GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK)
    {
        if (pObj == Null || pObj->init == False)
        {
            err = TDA10025_ERR_NOT_INITIALIZED;
        }
    }

    if ((err == TM_OK) && (pObj != Null)) 
    {
        pObj->init = False;
    }

    /* return value */
    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA10025_GetInstance                                         */
/*                                                                            */
/* DESCRIPTION: get the instance                                              */
/*                                                                            */
/* RETURN:      always True                                                   */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA10025_GetInstance 
(
 tmUnitSelect_t       tUnit,         /* I: Unit number   */
 ppTDA10025Object_t ppDrvObject    /* I: Device Object */
 )
{     
    tmErrorCode_t       err = TDA10025_ERR_NOT_INITIALIZED;
    pTDA10025Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for(uLoopCounter = 0; uLoopCounter<TDA10025_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA10025Instance[uLoopCounter];
        if(pObj->init == True && pObj->tUnit == tUnit)
        {
            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA10025_ResetInstance:                                      */
/*                                                                            */
/* DESCRIPTION: Resets an instance.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA10025_ResetInstance(
    pTDA10025Object_t  pDrvObject  /* I: Driver Object */
)
{
    tmErrorCode_t   err = TM_OK ;

    pDrvObject->sIP.uIFAGCThreshold = 0xD;

    pDrvObject->ePowerState = tmPowerMax; /* identify the power-up state */

    return err;
}

