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
*    File:    Tun_tda18250.h
*
*    Description:    Header file for alpstdae.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.  20090219	Magic Yang	Ver 0.1		Create file.
*****************************************************************************/

#ifndef __TUN_TDA18250AB_H__
#define __TUN_TDA18250AB_H__

//#include "../ali_qam.h"

#ifdef __cplusplus
extern "C"
{
#endif

INT32 tun_tda18250ab_init(UINT32 * tuner_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_tda18250ab_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
INT32 tun_tda18250ab_status(UINT32 tuner_id, UINT8 *lock);
INT32 tun_tda18250ab_release(UINT32 tuner_id);
unsigned int  tun_tda18250abwrite	(UINT32  uAddress, UINT32  uSubAddress, UINT32  uNbData, UINT32* pDataBuff);
//Bool tun_tda18250writebit(UInt32  uAddress, UInt32  uSubAddress, UInt32  uMaskValue, UInt32  uValue);
unsigned int	tun_tda18250abread(UINT32  uAddress, UINT32  uSubAddress,UINT32  uNbData,UINT32* pDataBuff);

#ifdef __cplusplus
}
#endif

#endif  /* __TUN_TDA18250_H__ */

