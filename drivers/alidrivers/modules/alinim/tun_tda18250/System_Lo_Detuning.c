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
#include "tmddTDA18250.h"
#include "tmbslTDA18250.h"
#include "tmbslTDA18250InstanceCustom.h"
#include "tmbslTDA18250local.h"
#include "System_Lo_Detuning.h"


/*============================================================================*/
/* FUNCTION:    tmLO_Detuning:                                                */
/*                                                                            */
/* DESCRIPTION: Calculate the shift of each LO of all Tuners                  */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t tmSystem_LO_Detuning
(
    tmUnitSelect_t          TunerUnit,      /* I: Tuner unit number                   */
    tmTDA18250ChannelType_t ChannelType,    /* I: Channel type                        */
    UInt32                  *uLO            /* I: local oscillator frequency in hertz */
)
{    
    tmErrorCode_t err = TM_OK;
    UInt32        uCounter = 0;
    UInt32        uIndex = 0;
    Int32         VCOs[TDA18250_MAX_UNITS];
    UInt32        uLoTemp = 0;
    UInt8         uPrescTemp = 0;
    UInt8         uPostDivTemp = 0;
    Bool          bShiftFreq = False;

    uIndex = (UNIT_PATH_INDEX_GET(TunerUnit) + 1)%TDA18250_MAX_UNITS;

    if (ChannelType == tmTDA18250AnalogChannel)
        *uLO += 40000;

    /**************************************************************************/
    /*                  Calculate VCOs of other Tuners                        */
    /**************************************************************************/
    for (uCounter=1; uCounter < TDA18250_MAX_UNITS; uCounter++)
    {

        /* Retrieve Tuner Lo */
        err = tmbslTDA18250ReadLO(UNIT_PATH_TYPE_SET(uIndex,UNIT_PATH_TYPE_GET(TunerUnit)), &uLoTemp);

        if (err == TM_OK)
        {
            /* Retrieve Prescaler and PostDiv */
            err = tmddTDA18250GetMainPLL(UNIT_PATH_TYPE_SET(uIndex,UNIT_PATH_TYPE_GET(TunerUnit)), uLoTemp, &uPrescTemp, &uPostDivTemp);

            if (err == TM_OK)
            {
                /* Calculate and store VCO (in kHz) */
                VCOs[uCounter] = (uLoTemp/1000) * uPrescTemp * uPostDivTemp;
            }
        }

        
        uIndex = (uIndex + 1)%TDA18250_MAX_UNITS;
    }

    do
    {
        /**********************************************************************/
        /*                 Calculate VCOs of current Tuner                    */
        /**********************************************************************/
        /* Retrieve Prescaler and PostDiv */
        err = tmddTDA18250GetMainPLL(TunerUnit, *uLO, &uPrescTemp, &uPostDivTemp);

        if (err == TM_OK)
        {
            /* Calculate and store VCO (in kHz) */
            VCOs[0] = ((*uLO)/1000) * uPrescTemp * uPostDivTemp;

            bShiftFreq = False;

            /* Search if we may encounter VCO pulling troubles */
            for(uCounter = 1; uCounter < TDA18250_MAX_UNITS; uCounter++)
            {
                if( abs( VCOs[0] - VCOs[uCounter] ) < 300)
                {
                    /* We need to shift frequency */
                    bShiftFreq = True;
                    *uLO += 5000;
                    break;
                }
            }
        }
        
    }
    while ((bShiftFreq) && (err == TM_OK));

    return err;
}
