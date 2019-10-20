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
 *
 *  File: basic_types.h
 *
 *  Contents: 	This file define the basic data types which may be used
 *			throughout the project.
 *  History:
 *		Date		Author      		Version 	Comment
 *		==========	==================	========== 	=======
 *  1.  03/09/2004  Tom Gao     		0.1.000 	Initial
 *
 *****************************************************************************/
#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__

#include <alidefinition/adf_basic.h>
#include "dvb_frontend_common.h"

#define HW_TYPE_CHIP_REV		0x00020000	/* Chip Revised */

/* CHIP Revised */
#define IC_REV_0				(HW_TYPE_CHIP_REV + 1)
#define IC_REV_1				(HW_TYPE_CHIP_REV + 2)
#define IC_REV_2				(HW_TYPE_CHIP_REV + 3)
#define IC_REV_3				(HW_TYPE_CHIP_REV + 4)
#define IC_REV_4				(HW_TYPE_CHIP_REV + 5)
#define IC_REV_5				(HW_TYPE_CHIP_REV + 6)
#define IC_REV_6				(HW_TYPE_CHIP_REV + 7)
#define IC_REV_7				(HW_TYPE_CHIP_REV + 8)
#define IC_REV_8				(HW_TYPE_CHIP_REV + 9)


#ifndef	NULL
#define NULL 			      ((void *)0)
#endif

#ifndef TRUE
#define TRUE             1
#endif

#ifndef FALSE
#define FALSE            0
#endif




#define SUCCESS          0       /* Success return */

#define ERR_NO_MEM      -1      /* Not enough memory error */
#define ERR_LONG_PACK   -2      /* Package too long */
#define ERR_RUNT_PACK   -3      /* Package too short */
#define ERR_TX_BUSY     -4      /* TX descriptor full */
#define ERR_DEV_ERROR   -5      /* Device work status error */
#define ERR_DEV_CLASH   -6      /* Device clash for same device in queue */
#define ERR_QUEUE_FULL  -7      /* Queue node count reached the max. val*/
#define ERR_NO_DEV      -8      /* Device not exist on PCI */
#define ERR_FAILURE	  	-9      /* Common error, operation not success */
/* Compatible with previous written error*/
#define ERR_FAILUE	  	-9

#define ERR_PARA        -20     /* Parameter is invalid */
#define ERR_ID_FULL     -21     /* No more ID available */
#define ERR_ID_FREE     -22     /* Specified ID isn't allocated yet */

#define ERR_OFF_SCRN    -30     /* Block is out off the screen */
#define ERR_V_OVRLAP    -31     /* Block is overlaped in vertical */
#define ERR_BAD_CLR     -32     /* Invalid Color Mode code */
#define ERR_OFF_BLOCK   -33     /* Bitmap is out off the block */
#define ERR_TIME_OUT    -34     /* Waiting time out */

/* add by Sen */
#define ERR_FAILED		-40
#define ERR_BUSY		-41
#define ERR_ADDRESS		-42
/* end of Sen */

typedef struct
{
	unsigned long parg1;
	unsigned long parg2;
}AUTOSCAN_MSG;

typedef struct
{
	INT32 port_id;
	UINT32 flag;
	AUTOSCAN_MSG msg;
}AUTO_SCAN_PARA;



struct t_diseqc_info
{
	UINT8 							sat_or_tp;			/* 0:sat, 1:tp*/
	UINT8 							diseqc_type;
	UINT8 							diseqc_port;
	UINT8 							diseqc11_type;
	UINT8 							diseqc11_port;
	UINT8 							diseqc_k22;
	UINT8 							diseqc_polar;		/* 0: auto,1: H,2: V */
	UINT8 							diseqc_toneburst;	/* 0: off, 1: A, 2: B */	

	UINT8 							positioner_type;	/*0-no positioner 1-1.2 positioner support, 2-1.3 USALS*/
	UINT8 							position;			/*use for DiSEqC1.2 only*/	
	UINT16 							wxyz;			/*use for USALS only*/
};



enum NIM_BLSCAN_MODE
{
	NIM_SCAN_FAST = 0,
	NIM_SCAN_SLOW = 1,
};


enum nim_perf_level
{
	NIM_PERF_DEFAULT		= 0,
	NIM_PERF_SAFER		= 1,
	NIM_PERF_RISK			= 2,
};



typedef INT32 (*pfn_nim_reset_callback)(UINT32 param);



enum NIM_TYPE
{
	NIM_DVBS     =1,
	NIM_DVBC,
	NIM_DVBT,//NIM_DVBT :DVBT2 or DVBT 
	NIM_ISDBT
};

typedef struct _tuner_config_t
{
	UINT8							type;
	UINT16          				tuner_crystal;		// Tuner Used Crystal: in KHz unit
	UINT8  							tuner_base_addr;	// Tuner BaseAddress for Write Operation: (BaseAddress + 1) for Read
	UINT32 							i2c_type;	        //i2c type and dev id select. bit16~bit31: type, I2C_TYPE_SCB/I2C_TYPE_GPIO. bit0~bit15:dev id, 0/1.
	UINT16                          tuner_if_freq;      //
}TUNER_CONFIG_DATA;



//common struct,unified use of the interface in the future
typedef INT32 (*tuner_init_callback) (UINT32* tuner_id, TUNER_CONFIG_DATA* tuner_config);
typedef INT32 (*tuner_control_callback) (UINT32 tuner_id, UINT32 freq, UINT32 sym);
typedef INT32 (*tuner_status_callback) (UINT32 tuner_id, UINT8 *lock);
typedef INT32 (*tuner_close_callback)(UINT32);

typedef INT32 (*tuner_command_callback)(UINT32 tuner_id, INT32 cmd, INT32 *param);
typedef	INT32 (*tuner_gain_callback)(UINT32 tuner_id, UINT32 agc_level); 

typedef INT32 (*dvbs_tuner_init_callback) (UINT32 *tuner_id, struct QPSK_TUNER_CONFIG_EXT  * ptrTuner_Config);
typedef INT32 (*dvbt_tuner_init_callback) (UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
typedef INT32 (*dvbc_tuner_init_callback) (UINT32 *tuner_id, struct QAM_TUNER_CONFIG_EXT   * ptrTuner_Config);

typedef int (*dvbt_tuner_init) (__u32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrtuner_config);
typedef int (*dvbt_tuner_control) (__u32 tuner_id,__u32 freq, __u8 bandwidth,__u8 agc_time_const,__u8 *data,__u8 cmd_type);
typedef int (*dvbt_tuner_status) (__u32 tuner_id,__u8 *lock);
typedef int (*dvbt_tuner_close) (void);
typedef int (*dvbt_tuner_command) (__u32 tuner_id, int cmd, __u32 param);


typedef struct _tuner_io_func_t
{
	enum NIM_TYPE 			nim_type;              // 1,dvbs,2,dvbc,3,dvbt
	UINT32                  tuner_id;
	tuner_init_callback     pf_init;
	tuner_control_callback  pf_control;
	tuner_status_callback   pf_status;
	tuner_command_callback  pf_command;
	tuner_gain_callback     pf_gain;
	tuner_close_callback    pf_close;

}TUNER_IO_FUNC;





#endif	//__BASIC_TYPES_H__

