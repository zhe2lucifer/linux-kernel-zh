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


/*============================================================================*/
/* Standard include files:                                                    */
/*============================================================================*/
#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmUnitParams.h"
#include "tmbslFrontEndTypes.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmddTDA18250.h"
#include "tmbslTDA18250.h"
#include "tmbslTDA18250InstanceCustom.h"

#include "tmbslTDA18250local.h"
#include "tmbslTDA18250Instance.h"


/*============================================================================*/
/* Global data:                                                               */
/*============================================================================*/

/* Driver instance */
tmTDA18250Object_t gTDA18250Instance[] = 
{
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit      */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex     */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc    */
            Null,
            Null
        },
        {                                                       /* sTime      */
            Null,
            Null
        },
        {                                                       /* sDebug     */
            Null
        },
        {                                                       /* sMutex     */
            Null,
            Null,
            Null,
            Null
        },
        0,                                                      /* RF         */
        0,                                                      /* freqCut    */
        False,                                                  /* ResetCalledPreviously */
        tmTDA18250_PowerTransitionMode_Smooth,                  /* ePowerTransitionMode */
        TMBSL_TDA18250_INSTANCE_CUSTOM_0
    },
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit      */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex     */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc    */
            Null,
            Null
        },
        {                                                       /* sTime      */
            Null,
            Null
        },
        {                                                       /* sDebug     */
            Null
        },
        {                                                       /* sMutex     */
            Null,
            Null,
            Null,
            Null
        },
        0,                                                      /* RF         */
        0,                                                      /* freqCut    */
        False,                                                  /* ResetCalledPreviously */
        tmTDA18250_PowerTransitionMode_Smooth,                  /* ePowerTransitionMode */
        TMBSL_TDA18250_INSTANCE_CUSTOM_1
    },
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit      */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex     */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc    */
            Null,
            Null
        },
        {                                                       /* sTime      */
            Null,
            Null
        },
        {                                                       /* sDebug     */
            Null
        },
        {                                                       /* sMutex     */
            Null,
            Null,
            Null,
            Null
        },
        0,                                                      /* RF         */
        0,                                                      /* freqCut    */
        False,                                                  /* ResetCalledPreviously */
        tmTDA18250_PowerTransitionMode_Smooth,                  /* ePowerTransitionMode */
        TMBSL_TDA18250_INSTANCE_CUSTOM_2
    },
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit      */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex     */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc    */
            Null,
            Null
        },
        {                                                       /* sTime      */
            Null,
            Null
        },
        {                                                       /* sDebug     */
            Null
        },
        {                                                       /* sMutex     */
            Null,
            Null,
            Null,
            Null
        },
        0,                                                      /* RF         */
        0,                                                      /* freqCut    */
        False,                                                  /* ResetCalledPreviously */
        tmTDA18250_PowerTransitionMode_Smooth,                  /* ePowerTransitionMode */
        TMBSL_TDA18250_INSTANCE_CUSTOM_3
    }
};


/*============================================================================*/
/* Internal functions:                                                        */
/*============================================================================*/

/*============================================================================*/
/* FUNCTION:    TDA18250AllocInstance:                                        */
/*                                                                            */
/* DESCRIPTION: Allocates an instance.                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
TDA18250AllocInstance
(
    tmUnitSelect_t          tUnit,      /* I: Unit number   */
    pptmTDA18250Object_t    ppDrvObject /* I: Device Object */
)
{ 
    tmErrorCode_t       err = TDA18250_ERR_BAD_UNIT_NUMBER;
    ptmTDA18250Object_t pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for (uLoopCounter = 0; uLoopCounter<TDA18250_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA18250Instance[uLoopCounter];
        if (pObj->init == False)
        {
            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}

/*============================================================================*/
/* FUNCTION:    TDA18250DeAllocInstance:                                      */
/*                                                                            */
/* DESCRIPTION: Deallocates an instance.                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
TDA18250DeAllocInstance
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{     
    tmErrorCode_t       err = TDA18250_ERR_BAD_UNIT_NUMBER;
    ptmTDA18250Object_t pObj = Null;

    /* check input parameters */
    err = TDA18250GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK)
    {
        if (pObj == Null || pObj->init == False)
        {
            err = TDA18250_ERR_NOT_INITIALIZED;
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
/* FUNCTION:    TDA18250GetInstance:                                          */
/*                                                                            */
/* DESCRIPTION: Gets an instance.                                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
TDA18250GetInstance
(
    tmUnitSelect_t          tUnit,      /* I: Unit number   */
    pptmTDA18250Object_t    ppDrvObject /* I: Device Object */
)
{     
    tmErrorCode_t       err = TDA18250_ERR_NOT_INITIALIZED;
    ptmTDA18250Object_t pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* get instance */
    for (uLoopCounter = 0; uLoopCounter<TDA18250_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA18250Instance[uLoopCounter];
        if (pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            
            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}

