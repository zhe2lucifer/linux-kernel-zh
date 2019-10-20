/*****************************************************************************
*    Copyright (C)2008 Ali Corporation. All Rights Reserved.
*
*    File:    tun_r836.h
*
*    Description:    Header file for alpstdae.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.  20131201		Joey.Gao	Ver 0.1		Create file.
*****************************************************************************/

#ifndef __LLD_TUN_R836_H__
#define __LLD_TUN_R836_H__

#include "../basic_types.h"
#include "../porting_linux_header.h"

//orange,2017.9.8
typedef struct _r836_config
{
	UINT16  				c_tuner_crystal;
	UINT8  					c_tuner_base_addr;		/* Tuner BaseAddress for Write Operation: (BaseAddress + 1) for Read */	
	UINT16 					w_tuner_if_freq;
	UINT32 					i2c_type_id;
}R836_CONFIG;

enum Tuner_Mode
{
	TUN_R836_DVBT_T2,
	TUN_R836_ISDBT,
	TUN_R836_DVBC
};


#ifdef __cplusplus
extern "C"
{
#endif

INT32 tun_r836_dvbc_init(UINT32 *tuner_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_r836_dvbt2_t_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_r836_isdbt_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);


INT32 tun_r836_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd);

INT32 tun_r836_status(UINT32 tuner_id, UINT8 *lock);

INT32 tun_r836_command(UINT32 tuner_id, INT32 cmd, UINT32 param);

INT32 tun_r836_release(UINT32 tuner_id);


#ifdef __cplusplus
}
#endif

#endif  


