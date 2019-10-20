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
 
#ifndef __PORTING_TDA10025_LINUX_H__
#define __PORTING_TDA10025_LINUX_H__


#include "../porting_linux_header.h"


#include "../basic_types.h"
#include "../tun_common.h"
#include "../nim_device.h"
#include "nim_tda10025.h"




#define MAX_TUNER_SUPPORT_NUM 	3

#if 0
#define TDA10025_PRINTF nim_print_x   //(x...) printk(KERN_INFO x)
#else
#define TDA10025_PRINTF(...) do{}while(0)
#endif





typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;	

struct nim_tda10025_private
{

	/* struct for QAM Configuration */
	struct   QAM_TUNER_CONFIG_DATA tuner_config_data;

	/* Tuner Initialization Function */
	INT32 (*nim_tuner_init)(UINT32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);

	/* Tuner Parameter Configuration Function */
	INT32 (*nim_tuner_control)(UINT32 Tun_id, UINT32 freq, UINT32 sym, UINT8 AGC_Time_Const, UINT8 _i2c_cmd);

	/* Get Tuner Status Function */
	INT32 (*nim_tuner_status)(UINT32 Tun_id, UINT8 *lock);

	/* Open  Function*/
	INT32 (*nim_Tuner_Open)(UINT32 Tun_id);

	INT32 (*nim_tuner_command)(UINT32 tuner_id, INT32 cmd, UINT32 param);

	/* Close Function. */
	INT32 (*nim_tuner_close)(UINT32 Tun_id);
	/* END */


	/* Extension struct for Tuner Configuration */
	struct QAM_TUNER_CONFIG_EXT tuner_config_ext;
	struct EXT_DM_CONFIG        ext_dem_config;
	//struct QAM_TUNER_CONFIG_API TUNER_PRIV;	

	UINT32 tuner_id;
	UINT32 qam_mode;
	
	UINT32 i2c_type_id;
	struct mutex i2c_mutex;
	/*new additions for mutil-process*/
	BOOL   nim_init;
	UINT8   nim_used;
	struct mutex process_mutex;
	//struct flag scan_flag;
	struct workqueue_struct *workqueue;
	struct work_struct work;
	UINT8  dev_idx;
};


INT32 	nim_tda10025_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *pstChl_Change );

#endif

