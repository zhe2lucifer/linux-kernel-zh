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

/*****************************************************************************
*    File: tun_m3031.h
*   
*    Description: 
*    
*    History: 
*    Date           Athor        Version        Reason
*    ========       ========     ========       ========
*    2015/3/3       Dennis                      porting
*        
*****************************************************************************/
#ifndef __NIM_M3031_H
#define __NIM_M3031_H

#include "../basic_types.h"
#include "../porting_linux_header.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define MAX_TUNER_SUPPORT_NUM 	2
#define ERR_FAIL				-1
#define M3031_GAIN_ERROR		-111111
#define LT_GAIN_DISABLE			-111
//#define LT_GAIN_AUTO		100
#define M3031_XTAL_FREQ_MHZ 	27	 //must be 27
#define FAST_RFAGC_SPEED		0 	//use for fast RF AGC locking
#define FAST_RFAGC_WAIT_MS		100 //wait time (ms) for RFAGC to settle in fast mode
#define CHIP_ID_M3031B			0x02


#define M3031B_OLD 0
#define M3031B_NEW 1
INT32 tun_m3031_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptr_tuner_config);
INT32 tun_m3031_control(UINT32 tuner_id, UINT32 freq, UINT32 bb_sym);
INT32 tun_m3031_status(UINT32 tuner_id, UINT8 *lock);
INT32 tun_m3031_gain(UINT32 tuner_id, UINT32 demod_agc);
INT32 tun_m3031_LT_gain(UINT32 tuner_id, INT32 lt_gain);
INT32 tun_m3031_standby(UINT32 tuner_id);
INT32 tun_m3031_command(UINT32 tuner_id, INT32 cmd, INT32 *param);
INT32 tun_m3031_release(void);


#ifdef __cplusplus
}
#endif

#endif  /* __LLD_M3031_H__ */

