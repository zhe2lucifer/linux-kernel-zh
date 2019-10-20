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
*    File:    nim_sharp6158.h
*
*    Description:    Header file in LLD.
*    History:
*           Date            Athor        Version          Reason
*       ============    =============   =========   =================
*   1.  4.27.2006        Joey Gao        Ver 0.1       Create file.
*   
*****************************************************************************/

#ifndef __LLD_NIM_CXD2834_H__
#define __LLD_NIM_CXD2834_H__

#include "../porting_linux_header.h"
#include "porting_cxd2838_linux.h"
#include "sony_demod.h"
#include "sony_demod_isdbt.h"
#include "sony_demod_isdbt_monitor.h"


typedef struct AGC_Table
{
	UINT32 AGC;
	UINT32 SignalStrength;
}AGC_Table_t;

//define the current demo type as the same as S3821(DVBT)
#define    ISDBT_TYPE        0
#define    DVBT_TYPE         1
#define    DVBT2_TYPE        2
#define    DVBT2_COMBO       3

INT32 nim_cxd2838_channel_change_smart(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para);
INT32 nim_cxd2838_channel_change(struct nim_device *dev, UINT32 freq, UINT32 bandwidth, UINT8 guard_interval, UINT8 fft_mode, UINT8 modulation, UINT8 fec, UINT8 usage_type, UINT8 inverse,UINT8 priority);
INT32 nim_cxd2838_get_agc(struct nim_device *dev, UINT8 *agc);
INT32 nim_cxd2838_get_snr(struct nim_device *dev, UINT8 *snr);
INT32 nim_cxd2838_get_ber(struct nim_device *dev, UINT32 *ber);
INT32 nim_cxd2838_get_lock(struct nim_device *dev, UINT8 *lock);
INT32 nim_cxd2838_close(struct nim_device *dev);
INT32 nim_cxd2838_open(struct nim_device *dev);
INT32 nim_cxd2838_get_modulation(struct nim_device *dev, UINT8 *modulation);
INT32 nim_cxd2838_get_guard(struct nim_device *dev,UINT8 *guard);

void cxd2838_log_i2c(sony_demod_t* param, UINT8 err, UINT8 write, UINT8 slv_addr, UINT8 *data, int len);




#endif  /* __LLD_NIM_CXD2834_H__ */

