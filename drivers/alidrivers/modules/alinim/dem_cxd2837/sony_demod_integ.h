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
 @file    sony_demod_integ.h

          This file provides the integration layer control interface.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DEMOD_INTEG_H
#define SONY_DEMOD_INTEG_H

#include "sony_demod.h"

#include "sony_demod_dvbt.h"
#include "sony_demod_dvbt2.h"
#include "sony_demod_dvbc.h"
#include "sony_demod_dvbt2_monitor.h"
#include "sony_demod_dvbt_monitor.h"
#include "sony_demod_dvbc_monitor.h"
#include "sony_math.h"




#ifndef DEMOD_TUNE_POLL_INTERVAL
#define DEMOD_TUNE_POLL_INTERVAL    10
#endif


/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/

sony_result_t sony_integ_dvbt_Tune(sony_demod_t * pDemod, sony_dvbt_tune_param_t * pTuneParam, BOOL NeedToConfigTuner);

sony_result_t sony_integ_dvbt2_Tune(sony_demod_t * pDemod, sony_dvbt2_tune_param_t * pTuneParam,BOOL NeedToConfigTuner);

sony_result_t sony_integ_dvbt2_BlindTune(sony_demod_t * pDemod, sony_dvbt2_tune_param_t * pTuneParam, BOOL NeedToConfigTuner, UINT8 t2_lite_support_flag);

sony_result_t sony_integ_CalcSSI(sony_demod_t * pDemod, uint32_t *pSSI, uint32_t rfLevel);

sony_result_t sony_integ_CalcSQI(sony_demod_t * pDemod, uint8_t *pSQI);

sony_result_t sony_integ_CalcBER(sony_demod_t * pDemod, uint32_t *pBER);

sony_result_t sony_integ_demod_CheckTSLock (sony_demod_t * pDemod, sony_demod_lock_result_t * pLock);
/*20160721 leo.liu add*/
sony_result_t sony_integ_CalcSNR(sony_demod_t * pDemod,uint32_t *pSNR);

sony_result_t sony_integ_CalcRF_LEVEL(sony_demod_t * pDemod,int32_t *rf_level);



#endif

