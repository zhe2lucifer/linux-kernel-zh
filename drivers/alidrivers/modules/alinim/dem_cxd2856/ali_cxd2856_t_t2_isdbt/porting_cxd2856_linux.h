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

#ifndef __PORTING_CXD2856_LINUX_H__
#define __PORTING_CXD2856_LINUX_H__

#include "../porting_linux_header.h"
#include "../tun_common.h"
#include "nim_cxd2856_proc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_FUNC_ON				0x00000001	/* Function on */    

typedef void (*nim_cxd2856_string_out_fnp_t)(char *string);
#if 0
typedef struct
{

    UINT8    fecrates;
    UINT8    modulation;
    UINT8    mode;            /* (ter) */
    UINT8    guard;           /* (ter) */
    UINT8    hierarchy;       /* (ter) */
    UINT8    spectrum;
    UINT8    channel_bw;       /* (ter) */
    UINT32   frequency;
    INT32    freq_offset;  /* (ter) */
} cxd2856_lock_info;
#endif
typedef INT32  (*INTERFACE_DEM_WRITE_READ_TUNER)(void * nim_dev_priv, UINT8 tuner_address, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
typedef struct
{
    void * nim_dev_priv; //for support dual demodulator.   
    //The tuner can not be directly accessed through I2C,
    //tuner driver summit data to dem, dem driver will Write_Read tuner.
    //INT32  (*Dem_Write_Read_Tuner)(void * nim_dev_priv, UINT8 slv_addr, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
    INTERFACE_DEM_WRITE_READ_TUNER  Dem_Write_Read_Tuner;
} DEM_WRITE_READ_TUNER;  //Dem control tuner by through mode (it's not by-pass mode).

typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;
//nim private data
typedef struct
{
	struct COFDM_TUNER_CONFIG_API tuner_control;
	UINT8	t2_signal;			//0:DVB-T signal, 1:DVB-T2 signal.
	__u32  nim_id; // HW id on real board
	UINT32  demod_id;
	__u32	tuner_id;
	__u32   tuner_name;
	__u32	nim_type;	//dvbs,dvbc,dvbt_t2,isdbt
	UINT8   tuner_addr;
	UINT32 	tuner_i2c_type;         //Ali I2C type. add by leo.liu when porting
	__u32  	tuner_i2c_communication_mode;// 1:getway mode,2:repteater mode
	UINT8   demod_addr;
	UINT32 	demod_i2c_type;
	int 	reset_pin;//store reset gpio get from dts, add by robin on 20161216
	
	UINT32	Frequency;
	UINT32  bandwidth;
	struct 	mutex ioctl_mutex;
	struct 	mutex i2c_mutex;
	struct 	mutex nim_mutex;
	UINT8	priority;			//for DVB-T
	UINT8	plp_index;			//Current selected data PLP index.
	UINT8	plp_num;
	UINT8	plp_id; 			//plp_id of plp_index.	
	UINT16	t2_system_id;		//t2_system_id of this channel. 
	uint8_t	all_plp_id[255];	//all plp_id. 
	UINT8	t2_profile;
	UINT8 	search_t2_only;
	BOOL	nim_init; //the flag of initalize
	INT8 	nim_used; //the num of user using nim
	pfn_nim_reset_callback 	     m_pfn_reset_cxd2856;  
	UINT32 autoscan_stop_flag			:1;
	UINT32 do_not_wait_t2_signal_locked :1;
	UINT32 reserved 					:30;
	/***for procfs debug***/
	struct nim_debug  debug_data; 
}ALI_CXD2856_DATA_T;

#ifdef __cplusplus
}
#endif

#endif

