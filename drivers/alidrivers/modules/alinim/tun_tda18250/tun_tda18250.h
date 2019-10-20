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

#ifndef __LLD_TUN_TDA18250_H__
#define __LLD_TUN_TDA18250_H__


#ifdef __cplusplus
extern "C"
{
#endif

INT32 		tun_tda18250_init(UINT32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 		tun_tda18250_control(UINT32 Tun_id, UINT32 freq, UINT32 sym, UINT8 AGC_Time_Const, UINT8 _i2c_cmd);
INT32 		tun_tda18250_control_X(UINT32 Tun_id, UINT32 freq, UINT32 sym);
INT32 		tun_tda18250_status(UINT32 Tun_id, UINT8 *lock);
INT32 		tun_tda18250_close(UINT32 tuner_id);


#ifdef __cplusplus
}
#endif

#endif  /* __LLD_TUN_TDA18250_H__ */


