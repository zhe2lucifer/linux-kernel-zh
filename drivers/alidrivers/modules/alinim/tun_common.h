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

#ifndef __TUN_COMMON_H__
#define __TUN_COMMON_H__



#include "basic_types.h"

TUNER_IO_FUNC *tuner_setup(UINT32 type,UINT32 tuner_id);



//tda18250
extern INT32 tun_tda18250_init(UINT32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_tda18250_control_X(UINT32 Tun_id, UINT32 freq, UINT32 sym);
extern INT32 tun_tda18250_status(UINT32 Tun_id, UINT8 *lock);
extern INT32 tun_tda18250_command(UINT32 tuner_id, INT32 cmd, UINT32 param);
extern INT32 tun_tda18250_close(UINT32 tuner_id);

//tda18250ab
extern INT32 tun_tda18250ab_init(UINT32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_tda18250ab_control(UINT32 Tun_id, UINT32 freq, UINT32 sym);
extern INT32 tun_tda18250ab_status(UINT32 Tun_id, UINT8 *lock);

//mxl603
extern INT32 tun_mxl603_init_DVBC(UINT32 *tuner_id , struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_mxl603_control_DVBC_X(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 tun_mxl603_command(UINT32 tuner_id, INT32 cmd, UINT32 param);

extern INT32 tun_mxl603_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_mxl603_release(void);


extern INT32 tun_mxl603_init(UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_mxl603_init_ISDBT(UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)  ;
//extern INT32 tun_mxl603_init_ISDBT(UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)  ;
extern INT32 tun_mxl603_init_CDT_MN88472(UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config) ;
extern INT32 tun_mxl603_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth)	;
extern INT32 tun_mxl603_command(UINT32 tuner_id, INT32 cmd, UINT32 param);


//av2012
extern INT32 ali_nim_av2011_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 ali_nim_av2011_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 ali_nim_av2011_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 ali_nim_av2011_close(void);

//RDA5815M
extern INT32 nim_rda5815m_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 nim_rda5815m_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 nim_rda5815m_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 nim_rda5815m_command(UINT32 tuner_id, INT32 cmd, UINT32 param);
extern INT32 nim_rda5815m_close(void);

//sharp_vz7306
extern INT32 ali_nim_vz7306_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 ali_nim_vz7306_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 ali_nim_vz7306_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 ali_nim_vz7306_close(void);


//dct70701
extern INT32 tun_dct70701_init(UINT32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_dct70701_control(UINT32 Tun_id, UINT32 freq, UINT32 sym);//, UINT8 AGC_Time_Const, UINT8 _i2c_cmd
extern INT32 tun_dct70701_status(UINT32 Tun_id, UINT8 *lock);
extern INT32 tun_dct70701_release(void);


//sony cxd 2872 Tuner
extern INT32 tun_cxd_ascot3_Init(INT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_cxd_ascot3_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd);
extern INT32 tun_cxd_ascot3_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_cxd_ascot3_release(void);
extern INT32 tun_cxd_ascot3_command(UINT32 tuner_id, INT32 cmd, UINT32 param);

//ALi M3031 tuner
extern INT32 tun_m3031_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_m3031_control(UINT32 tuner_id, UINT32 freq, UINT32 bb_sym);
extern INT32 tun_m3031_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_m3031_command(UINT32 tuner_id, INT32 cmd, INT32 *param);
extern INT32 tun_m3031_release(void);
extern INT32 tun_m3031_gain(UINT32 tuner_id, UINT32 AGC_level);

extern INT32 tun_si2141_init_dvbt(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptr_Tuner_Config);
extern INT32 tun_si2141_init_isdbt(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_si2141_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd);
extern INT32 tun_si2141_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_si2141_command(UINT32 tuner_id, INT32 cmd, UINT32 param);
extern INT32 tun_si2141_release(void);

//r858
extern INT32 tun_r858_init(UINT32 *tuner_id , struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_r858_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 tun_r858_command(UINT32 tuner_id, INT32 cmd, UINT32 param);
extern INT32 tun_r858_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_r858_release(void);

extern INT32 tun_rt7x0_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_rt7x0_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
extern INT32 tun_rt7x0_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_rt7x0_command(UINT32 tuner_id, INT32 cmd, INT32 *param);
extern INT32 tun_rt7x0_release(void);

//r836
extern INT32 tun_r836_dvbc_init(UINT32* tuner_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_r836_dvbt2_t_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);
extern INT32 tun_r836_isdbt_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config);

extern INT32 tun_r836_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd);
extern INT32 tun_r836_status(UINT32 tuner_id, UINT8 *lock);
extern INT32 tun_r836_command(UINT32 tuner_id, INT32 cmd, UINT32 param);
extern INT32 tun_r836_release(UINT32 tuner_id);

#endif
