/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
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
 

/*
*    File:    Tun_MxL5005.h
*
*    Description:    Header file of MAXLINEAR MxL5005 TUNER.
*    History:
*           Date          Author                 Version          Reason
*	    ============	=============	=========	=================
*   4.17.2008	     David.Deng	      Ver 0.1	     Create file.
*****************************************************************************/

#ifndef __TUN_MxL203RF_H__
#define __TUN_MxL203RF_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include "MxL203RF_API.h"
#include "MxL203RF_Common.h"

#define MAX_TUNER_SUPPORT_NUM 		2

INT32 ali_nim_tuner_register(struct tuner_handle *model);
INT32 ali_nim_tuner_unregister(const UINT8 *model_name);

INT32 tun_mxl203rf_init(UINT32* tuner_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_mxl203rf_control(UINT32 tuner_id, UINT32 freq, UINT32 sym, UINT8 AGC_Time_Const, UINT8 _i2c_cmd);
INT32 tun_mxl203rf_status(UINT32 tuner_id, UINT8 *lock);
INT32 tun_mxl203rf_release(void);

#ifdef __cplusplus
}
#endif

#endif  /* __TUN_MxL5005_H__ */



