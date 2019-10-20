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
#include "tmUnitParams.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "System_Manage_HPMTO.h"
#include "tmddTDA18250.h"
#include "tmbslTDA18250.h"
#include "tmbslTDA18250InstanceCustom.h"
#include "tmbslTDA18250local.h"


/*============================================================================*/
/* FUNCTION:    tmSystem_Manage_HPMTO:                                        */
/*                                                                            */
/* DESCRIPTION: Set the HP MTO on the Master if Slave is analogue             */
/*              and its RF is superior to 250 Mhz                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t tmSystem_Manage_HPMTO
(
    tmUnitSelect_t  TunerUnit   /* I: Tuner unit number */
)
{
    tmErrorCode_t               err  = TM_OK;
    UInt32                      uIndex0,uIndex2,uIndex3,uIndex;
    UInt32                      uRF;
    tmTDA18250StandardMode_t    StandardMode;
    UInt32                      TunerType;

    TunerType = UNIT_PATH_TYPE_GET (TunerUnit);
    uIndex0 = GET_INDEX_TUNIT(0)|UNIT_PATH_TYPE_VAL(TunerType);
    uIndex2 = GET_INDEX_TUNIT(2)|UNIT_PATH_TYPE_VAL(TunerType);
    uIndex3 = GET_INDEX_TUNIT(3)|UNIT_PATH_TYPE_VAL(TunerType);

    /* stop AGC1 loop */
    if (err == TM_OK)
    {
        err = TDA18250SetAGC1_loop_off(uIndex0, tmTDA18250_ON);
    }

    /* Manage HP MTO on Master if SEtRF for Slave2&3 in analogue with RF >250 MHz */
    if (err == TM_OK)
    {
        for (uIndex=uIndex2; uIndex<=uIndex3; uIndex++)
        {
            err = tmbslTDA18250GetRf (uIndex,&uRF);
            
            if (err == TM_OK)
            {
                err = tmbslTDA18250GetStandardMode (uIndex,&StandardMode);
            }

            if (err == TM_OK)
            {
                if (  (uRF >= 250000000) )   //kent:(StandardMode<=tmTDA18250_ANA_9MHz) &&
                {
                    err = TDA18250SetMTOClose(uIndex0, (UInt8) GET_INDEX_TUNIT(uIndex) );
                }
                else
                {
                    err = TDA18250SetMTOOpen(uIndex0, (UInt8) GET_INDEX_TUNIT(uIndex) );
                }
            }
        }
    }

    /* restart AGC1 loop */
    if (err == TM_OK)
    {
        err = TDA18250SetAGC1_loop_off(uIndex0, tmTDA18250_OFF);
    }

    return err;
}

