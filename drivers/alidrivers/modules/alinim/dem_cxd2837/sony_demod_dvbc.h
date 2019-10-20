/*
 * Copyright 2012 Sony Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

/**
 @file    sony_demod_dvbc.h

          This file provides the demodulator control interface specific to DVB-C.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DEMOD_DVBC_H
#define SONY_DEMOD_DVBC_H

#include "sony_common.h"
#include "sony_demod.h"

/*------------------------------------------------------------------------------
 Structs
------------------------------------------------------------------------------*/
/**
 @brief The tune parameters for a DVB-C signal
*/
typedef struct sony_dvbc_tune_param_t{
    uint32_t centerFreqKHz;         /**< Center frequency in kHz of the DVB-C channel */

    /**
     @brief Bandwidth of the DVB-C channel
    */
    sony_demod_bandwidth_t bandwidth;

}sony_dvbc_tune_param_t;

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
/**
 @brief Enable acquisition on the demodulator for DVB-C channels.  Called from
        the integration layer ::sony_integ_dvbc_Tune API.

 @param pDemod  The demodulator instance
 @param pTuneParam The tune parameters.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_demod_dvbc_Tune (sony_demod_t * pDemod, sony_dvbc_tune_param_t * pTuneParam);

/**
 @brief Put the demodulator into ::SONY_DEMOD_STATE_SLEEP_T_C state.  Can be called
        from Active, Shutdown or Sleep states.  Called from the integration layer
        ::sony_integ_SleepT_C API.

 @param pDemod  The demodulator instance

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_demod_dvbc_Sleep (sony_demod_t * pDemod);

/**
@brief Check DVB-C demodulator lock status.

 @param pDemod The demodulator instance
 @param pLock Demod lock state

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_demod_dvbc_CheckDemodLock (sony_demod_t * pDemod, 
                                              sony_demod_lock_result_t * pLock);

#endif /* SONY_DEMOD_DVBC_H */
