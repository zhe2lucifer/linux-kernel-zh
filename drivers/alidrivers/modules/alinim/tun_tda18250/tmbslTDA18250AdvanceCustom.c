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
#include "tmbslFrontEndTypes.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmddTDA18250.h"

#include "tmbslTDA18250.h"
#include "tmbslTDA18250InstanceCustom.h"
#include "tmbslTDA18250local.h"
#include "tmbslTDA18250Instance.h"
#include "tmbslTDA18250AdvanceCustom.h"


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC1:                                         */
/*                                                                            */
/* DESCRIPTION: Sets the AGC1 gain.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC1
(
    tmUnitSelect_t           tUnit,     /* I: Unit number */
    tmddTDA18250_AGC1_Gain_t agc1Gain   /* I: AGC1 gain   */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;


    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        /* stop AGC1 loop */
        err = TDA18250SetAGC1_loop_off(tUnit, tmTDA18250_ON);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250SetAGC1_loop_off(0x%08X) failed.", tUnit));

        if (err == TM_OK)
        {
            /* AGC1 frozen */
            err = tmddTDA18250_SetAGC1_gain_Force(tUnit, tmddTDA18250_AGC_Mode_forced);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250_SetAGC1_gain_Force(0x%08X) failed.", tUnit));
        }

        if (err == TM_OK)
        {
            err = tmddTDA18250SetAGC1_Gain(tUnit, (UInt8)agc1Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC1_Gain(0x%08X) failed.", tUnit));
        }
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250GetAGC1:                                         */
/*                                                                            */
/* DESCRIPTION: Gets the AGC1 gain.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250GetAGC1
(
    tmUnitSelect_t            tUnit,     /* I: Unit number */
    tmddTDA18250_AGC1_Gain_t* pAgc1Gain  /* O: AGC1 gain   */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;


    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        err = tmddTDA18250GetAGC1_Gain(tUnit, (UInt8*)pAgc1Gain);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetAGC1_Gain(0x%08X) failed.", tUnit));
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC2b_Gain:                                   */
/*                                                                            */
/* DESCRIPTION: Sets the AGC2b gain.                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:       decimal value=(Gain + 12dB)/0.2, decimal value: [0..142]      */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC2b_Gain
(
    tmUnitSelect_t  tUnit,     /* I: Unit number    */
    UInt8           agc2Gain   /* I: AGC2 gain (dB) */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uRdAgc2Gain;
    UInt8               AGC2b = 0;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        /* Get and Set back the same gain to refresh internal Tuner memory */
        err = tmddTDA18250GetAGC2b_Gain(tUnit, &uRdAgc2Gain);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetAGC2b_Gain(0x%08X) failed.", tUnit));

        if (err == TM_OK)
        {
            err = tmddTDA18250SetAGC2b_Gain(tUnit, uRdAgc2Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
        }
        
        if (err == TM_OK)
        {
            /* AGC2 frozen */
            err = tmddTDA18250SetAGC2_gain_Force(tUnit, tmddTDA18250_ON);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2_gain_Force(0x%08X) failed.", tUnit));
        }
        
        if (err == TM_OK)
        {
            AGC2b = 5*(agc2Gain + 12);
            err = tmddTDA18250SetAGC2b_Gain(tUnit, (UInt8)AGC2b);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
        }
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250GetAGC2b_Gain:                                   */
/*                                                                            */
/* DESCRIPTION: Gets the AGC2b gain.                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:       Gain =(decimal value)*0.2-12 dB, decimal value: [0..142]      */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250GetAGC2b_Gain
(
    tmUnitSelect_t  tUnit,     /* I: Unit number    */
    UInt8*          pAgc2Gain  /* O: AGC2 gain (dB) */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               RdAgc2Gain = 0;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        err = tmddTDA18250GetAGC2b_Gain(tUnit, &RdAgc2Gain);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetAGC2b_Gain(0x%08X) failed.", tUnit));
    }
    if (err == TM_OK)
    {
        *pAgc2Gain = (RdAgc2Gain / 5) - 12;

        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC2b_Raw:                                    */
/*                                                                            */
/* DESCRIPTION: Sets the AGC2b raw value                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC2b_Raw
(
    tmUnitSelect_t  tUnit,     /* I: Unit number    */
    UInt8           Agc2Raw    /* I: AGC2 gain (dB) */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uRdAgc2Gain;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {

        /* Get and Set back the same gain to refresh internal Tuner memory */
        err = tmddTDA18250GetAGC2b_Gain(tUnit, &uRdAgc2Gain);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetAGC2b_Gain(0x%08X) failed.", tUnit));

        if (err == TM_OK)
        {
            err = tmddTDA18250SetAGC2b_Gain(tUnit, uRdAgc2Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
        }
        
        if (err == TM_OK)
        {
            /* AGC2 frozen */
            err = tmddTDA18250SetAGC2_gain_Force(tUnit, tmddTDA18250_ON);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2_gain_Force(0x%08X) failed.", tUnit));
        }
        
        if (err == TM_OK)
        {
             err = tmddTDA18250SetAGC2b_Gain(tUnit, Agc2Raw);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
         }

        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18250GetAGC2b_Raw:                                    */
/*                                                                            */
/* DESCRIPTION: Gets the AGC2b raw value                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250GetAGC2b_Raw
(
    tmUnitSelect_t  tUnit,     /* I: Unit number    */
    UInt8           *pAgc2Raw    /* I: AGC2 gain (dB) */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {

        err = tmddTDA18250GetAGC2b_Gain(tUnit, pAgc2Raw);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC1Freeze:                                   */
/*                                                                            */
/* DESCRIPTION: Freezes the AGC1 gain                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC1Freeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;


    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        /* stop AGC1 loop */
        err = TDA18250SetAGC1_loop_off(tUnit, tmTDA18250_ON);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250SetAGC1_loop_off(0x%08X) failed.", tUnit));
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC1Unfreeze:                                 */
/*                                                                            */
/* DESCRIPTION: Unfreezes the AGC1 gain                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC1Unfreeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;


    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {

        /* AGC1 free */
        err = tmddTDA18250_SetAGC1_gain_Force(tUnit, tmddTDA18250_AGC_Mode_Free);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250_SetAGC1_gain_Force(0x%08X) failed.", tUnit));
        
        if (err == TM_OK)
        {
        /* stop AGC1 loop */
        err = TDA18250SetAGC1_loop_off(tUnit, tmTDA18250_OFF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250SetAGC1_loop_off(0x%08X) failed.", tUnit));
        }
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC2Freeze:                                   */
/*                                                                            */
/* DESCRIPTION: Freezes the AGC2b gain                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC2Freeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uAgc2Gain;


    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        /* Get and Set back the same gain to freeze AGC2 gain */
        err = tmddTDA18250GetAGC2b_Gain(tUnit, &uAgc2Gain);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetAGC2b_Gain(0x%08X) failed.", tUnit));

        if (err == TM_OK)
        {
            err = tmddTDA18250SetAGC2b_Gain(tUnit, uAgc2Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2b_Gain(0x%08X) failed.", tUnit));
        }
 
        if (err == TM_OK)
        {
            /* AGC2 frozen */
            err = tmddTDA18250SetAGC2_gain_Force(tUnit, tmddTDA18250_ON);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2_gain_Force(0x%08X) failed.", tUnit));
        }
 
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetAGC2Unfreeze:                                 */
/*                                                                            */
/* DESCRIPTION: Unfreezes the AGC2b gain                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetAGC2Unfreeze
(
    tmUnitSelect_t      tUnit,  /* I: Unit number */
    tmTDA18250_State_t  uState  /* I: State value */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        /* AGC2 frozen */
        err = tmddTDA18250SetAGC2_gain_Force(tUnit, tmddTDA18250_OFF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetAGC2_gain_Force(0x%08X) failed.", tUnit));

        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18250SetLoFreq:                                       */
/*                                                                            */
/* DESCRIPTION: Sets the the MAIN fractional PLL settings.                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:       RF (in Hz) as parameter                                       */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250SetLoFreq
(
    tmUnitSelect_t  tUnit,      /* I: Unit number      */
    UInt32          uRF         /* I: Frequency in Hz  */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt32              uLO = 0;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        uLO = uRF + pObj->Std_Array[pObj->StandardMode].IF;
        err = tmddTDA18250SetLOFreq(tUnit, uLO);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250SetLOFreq(0x%08X) failed.", tUnit));
        
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    tmbslTDA18250GetVCO:                                          */
/*                                                                            */
/* DESCRIPTION: Gets VCO in KHz                                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18250GetVCO
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Int32*          puVCO       /* O: VCO in KHz  */
)
{
    ptmTDA18250Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uPrescTemp = 0;
    UInt8               uPostDivTemp = 0;
    UInt32              uLO = 0;

    /* Get a driver instance */
    err = TDA18250GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA18250GetInstance(0x%08X) failed.", tUnit));

    if (err == TM_OK)
    {
        /* Try to acquire driver mutex */
        err = TDA18250MutexAcquire(pObj, TDA18250_MUTEX_TIMEOUT);
    }

    if (err == TM_OK)
    {
        uLO = pObj->uRF + pObj->Std_Array[pObj->StandardMode].IF;

        /* Retrieve Prescaler and PostDiv */
        err = tmddTDA18250GetMainPLL(tUnit, uLO, &uPrescTemp, &uPostDivTemp);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA18250GetMainPLL(0x%08X) failed.", tUnit));

        if (err == TM_OK)
        {
            /* return VCO (in kHz) */
            *puVCO = (uLO/1000) * uPrescTemp * uPostDivTemp;
        }
        else
        {
            *puVCO = 0;
        }
            
        /* Release driver mutex */
        (void)TDA18250MutexRelease(pObj);
    }

    return err;
}


