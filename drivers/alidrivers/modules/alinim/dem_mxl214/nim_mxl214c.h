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
*    File:    nim_mxl214c.h
*
*    Description:    head file in LLD.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.20140717       alan      Ver 0.1       Create file.
*****************************************************************************/

#ifndef __NIM_MXL214C_H__
#define __NIM_MXL214C_H__

#if defined(__NIM_LINUX_PLATFORM__)
#include "porting_mxl214c_linux.h"
#elif defined(__NIM_TDS_PLATFORM__)
#include "porting_mxl214c_tds.h"
#endif

#include "MxL_HRCLS_Common.h"
#include "MxL_HRCLS_CommonApi.h"



#define NIM214C_DEBUG  0

#if(NIM214C_DEBUG)
	#define MXL214C_PRINTF		nim_print_x
#else
	#define MXL214C_PRINTF(...)		do{}while(0)
#endif





#define MXL_DEMOD_MAX_NUM           4
#define HRCLS_XTAL_CAP_VALUE        0 
typedef void (*NIM_MXL214C_AP_CALLBACK)(UINT8 devid);



/*****************************************************************************
* INT32 nim_mxl214c_attach(struct QAM_TUNER_CONFIG_API * ptrQAM_Tuner);
* Description: mxl214 series chip attach.
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
/*
INT32 nim_mxl214c_attach(struct QAM_TUNER_CONFIG_API * ptrQAM_Tuner);
static INT32 nim_mxl214c_open(struct nim_device *dev);
static INT32 nim_mxl214c_close(struct nim_device *dev);
static INT32 nim_mxl214c_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param);
static INT32 nim_mxl214c_ioctl_ext(struct nim_device *dev, INT32 cmd, void* param_list);
*/

void nim_mxl214_hwreset(void);
INT32 nim_mxl214c_hw_init(struct nim_device *dev);

void nim_mxl214c_set_retset_proc(NIM_MXL214C_AP_CALLBACK ptr_callback);
void nim_mxl214c_reset(UINT8 devId);


INT32 nim_mxl214c_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T* pst_ch_change);
INT32 nim_mxl214c_channel_search(struct nim_device *dev, UINT32 freq);

INT32 nim_mxl214c_get_ber(struct nim_device *dev, UINT32 *err_count);
INT32 nim_mxl214c_get_lock(struct nim_device *dev, UINT8 *lock);
INT32 nim_mxl214c_get_freq(struct nim_device *dev, UINT32 *freq);
INT32 nim_mxl214c_get_symbol_rate(struct nim_device *dev, UINT32 *sym_rate);
INT32 nim_mxl214c_get_qam_order(struct nim_device *dev, UINT8 *qam_order);
INT32 nim_mxl214c_get_agc(struct nim_device *dev, UINT8 *agc);
INT32 nim_mxl214c_get_snr(struct nim_device *dev, UINT8 *snr);
INT32 nim_mxl214c_get_per(struct nim_device *dev, UINT32 *RsUbc);

INT32        nim_mxl214c_get_rf_level(struct nim_device *dev, UINT16 *RfLevel);
INT32        nim_mxl214c_get_cn_value(struct nim_device *dev, UINT16 *CNValue);
void         nim_mxl214c_set_qam_type(struct nim_device *dev, NIM_CHANNEL_CHANGE_T* pstchl_change);
MXL_STATUS_E nim_mxl214c_checkversion(UINT8 devid, MXL_HRCLS_DEV_VER_T* verinfoptr);
MXL_STATUS_E nim_mxl214c_enablefbtuner(UINT8 devid);
MXL_STATUS_E nim_mxl214c_downloadfirmware(UINT8 devid);



#endif

