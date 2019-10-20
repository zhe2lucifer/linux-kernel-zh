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

/*============================================================================*/
/* Standard include files:                                                    */
/*============================================================================*/
#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"
#include "tmUnitParams.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmbslHCDP.h"

#include "tmbslHCDP_Reg.h"
#include "tmbslHCDP_Local.h"
#include "tmbslHCDP_Instance.h"

/*============================================================================*/
/* Global data:                                                               */
/*============================================================================*/
#define HCDP_INSTANCE_MARKER    0xABCD 

#define HCDP_INSTANCE_DEFAULT \
    (tmUnitSelect_t)(-1),                   /* tUnit           */ \
    (tmUnitSelect_t)(-1),                   /* tUnitW          */ \
    HCDP_SAMPLE_UNKNOWN,                    /* eHwCdpSample    */ \
    Null,                                   /* pMutex          */ \
    False,                                  /* init            */ \
    False,                                  /* bReset          */ \
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
    },\
    {                       /*sConfig*/ \
    (UInt32)0,                  /*uSamplingClock*/ \
    (UInt32)0,                  /*uDClock*/ \
    (UInt32)0,                  /*uXtalMultiplier */ \
    (UInt16)0,                  /*uIFAGCThreshold*/ \
    (UInt16)0,                  /*uRFAGCThreshold*/ \
    },\
    {                   /*sCurrentChannel*/ \
    (UInt32)0,                  /*uIF*/ \
    (UInt32)0,                  /*uSR*/ \
    False,                       /*bAutoSpectrum*/ \
    tmFrontEndFECModeUnknown,    /*eFECMode*/ \
    tmFrontEndModulationAuto,    /* eMod*/ \
    tmFrontEndSpecInvAuto,      /*eSI*/ \
    },\
    (UInt32)0,                  /* uStoredBitErrs*/ \
    (UInt32)0,                  /* uStoredUncorrectable*/ \
    (UInt32)0,                  /* uStoredCorrected*/ \
    (UInt32)0,                  /* uStoredNonerrors*/\
    {                   /*sPreviousBER*/ \
    (Int32)0,                  /*lInteger*/ \
    (UInt32)0,                  /*uDivider*/ \
    }, \
    (UInt32)0,                  /* uBaseUncors*/ \
    (UInt32)0,                  /* uPreviousUncors*/ \
    HCDP_BERWindow_Unknown,     /* eBERWindow*/ \
    (UInt16)0,                  /* usBerWindowCount*/ \
    (UInt16)0,                  /* usPBWindow*/ \
    HCDP_PNBERWindow_1E5,       /*eWindow*/ \
    (UInt32)0,                  /* uPNReadyOld*/\
    (UInt32)0,/* usConDScale*/ \
    (UInt32)0,/* usConDMask*/ \
    {{0,0}},     /*sI[64]*/ \
    {{0,0}},     /*sQ[64]*/ \
    HCDP_ExtendSymbolRateModeInvalid, \
    HCDP_INSTANCE_MARKER


/* Driver instance */
HCDPObject_t gHCDPInstance[]= 
{
    {HCDP_INSTANCE_DEFAULT},
    {HCDP_INSTANCE_DEFAULT},
    {HCDP_INSTANCE_DEFAULT},
    {HCDP_INSTANCE_DEFAULT},
    {HCDP_INSTANCE_DEFAULT},
    {HCDP_INSTANCE_DEFAULT}
};

/*============================================================================*/
/* Internal functions:                                                        */
/*============================================================================*/

/*============================================================================*/
/* FUNCTION:    iHCDP_AllocInstance:                                          */
/*                                                                            */
/* DESCRIPTION: Allocates an instance.                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iHCDP_AllocInstance(
    tmUnitSelect_t      tUnit,      /* I: Unit number */
    ppHCDPObject_t  ppDrvObject /* I: Driver Object */
)
{
    tmErrorCode_t       err = HCDP_ERR_ERR_NO_INSTANCES;
    pHCDPObject_t   pObj = Null;
    UInt32              uLoopCounter = 0;

    /* Find a free instance */
    for (uLoopCounter = 0; uLoopCounter<HCDP_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gHCDPInstance[uLoopCounter];
        if (pObj->init == False)
        {
            err = TM_OK;

            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            err = iHCDP_ResetInstance(pObj);

            if (pObj->Marker != HCDP_INSTANCE_MARKER)
            {
                err = HCDP_ERR_NOT_INITIALIZED;
            }

            if(err == TM_OK)
            {
                *ppDrvObject = pObj;
            }
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iHCDP_DeAllocInstance:                                        */
/*                                                                            */
/* DESCRIPTION: Deallocates an instance.                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iHCDP_DeAllocInstance(
    pHCDPObject_t  pDrvObject  /* I: Driver Object */
)
{
    tmErrorCode_t   err = TM_OK;

    if(pDrvObject != Null)
    {
        pDrvObject->init = False;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iHCDP_GetInstance:                                            */
/*                                                                            */
/* DESCRIPTION: Gets an instance.                                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iHCDP_GetInstance(
    tmUnitSelect_t  tUnit,      /* I: Unit number   */
    ppHCDPObject_t  ppDrvObject /* I: Driver Object */
)
{
    tmErrorCode_t   err = HCDP_ERR_ERR_NO_INSTANCES;
    pHCDPObject_t   pObj = Null;
    UInt32          uLoopCounter = 0;

    /* Get instance */
    for (uLoopCounter = 0; uLoopCounter<HCDP_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gHCDPInstance[uLoopCounter];
        if (pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iHCDP_ResetInstance:                                          */
/*                                                                            */
/* DESCRIPTION: Resets an instance.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iHCDP_ResetInstance(
    pHCDPObject_t  pDrvObject  /* I: Driver Object */
)
{
    tmErrorCode_t   err = TM_OK ;

    pDrvObject->sConfig.uXtalMultiplier = 1;

    pDrvObject->sConfig.uIFAGCThreshold = 0x0B;

    pDrvObject->sConfig.uDClock = 54000000;
    pDrvObject->sConfig.uSamplingClock = 54000000;

    pDrvObject->uBaseUncors = 0;
    pDrvObject->uPreviousUncors = 0;

    pDrvObject->sPreviousBER.lInteger = 0;
    pDrvObject->sPreviousBER.uDivider = 0;

    pDrvObject->eBERWindow  = HCDP_BERWindow_1E7;

    /* Default configured values */
    pDrvObject->sCurrentChannel.eFECMode = tmFrontEndFECModeAnnexA;
    pDrvObject->sCurrentChannel.eMod = tmFrontEndModulationQam256;
    pDrvObject->sCurrentChannel.eSI = tmFrontEndSpecInvAuto;
    pDrvObject->sCurrentChannel.uIF = 5000000;
    pDrvObject->sCurrentChannel.uSR = 6875000;


    /* Internal functions variables */
    pDrvObject->usConDScale = 1;
    pDrvObject->usConDMask = 0x1;

    return err;
}

