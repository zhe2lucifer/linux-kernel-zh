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
 
#include "tun_common.h"
#include "porting_linux_header.h"

#if 0
#define PRINTK_INFO   nim_print_x
#else
#define PRINTK_INFO(x...)
#endif






//Please add callback function to this arrray when new tuner is coming.
//kent.2014.1.13

static TUNER_IO_FUNC g_tuner_array[]=
{
#ifdef  CONFIG_AV2012   // CONFIG_AV2012		
	{
	   	NIM_DVBS, 
	   	AV_2012,	
	   	(tuner_init_callback)ali_nim_av2011_init,     
	   	(tuner_control_callback)ali_nim_av2011_control,   
	   	(tuner_status_callback)ali_nim_av2011_status,  
		(tuner_command_callback)NULL,
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)ali_nim_av2011_close
	 },
#endif

#ifdef CONFIG_RDA5815M
	{
		NIM_DVBS,
		RDA5815M,
		(tuner_init_callback)nim_rda5815m_init,
		(tuner_control_callback)nim_rda5815m_control,
		(tuner_status_callback)nim_rda5815m_status,
		(tuner_command_callback)nim_rda5815m_command,
		(tuner_gain_callback)NULL,
		(tuner_close_callback)nim_rda5815m_close
	},
#endif

#ifdef CONFIG_MXL603
	{
	   	NIM_DVBC,
	   	MXL603,
	   	(tuner_init_callback)tun_mxl603_init_DVBC,     
	   	(tuner_control_callback)tun_mxl603_control_DVBC_X,   
	   	(tuner_status_callback)tun_mxl603_status, 
		(tuner_command_callback)tun_mxl603_command,
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_mxl603_release
	 },
#endif
#ifdef CONFIG_SHARP_VZ7306
	{
	   	NIM_DVBS,
		SHARP_VZ7306,
	   	(tuner_init_callback)ali_nim_vz7306_init,     
	   	(tuner_control_callback)ali_nim_vz7306_control,   
	   	(tuner_status_callback)ali_nim_vz7306_status,  
		(tuner_command_callback)NULL,
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)ali_nim_vz7306_close
	 },
#endif		
#ifdef CONFIG_TDA18250
	{
	    NIM_DVBC,
	   	TDA18250,
	   	(tuner_init_callback)tun_tda18250_init,     
	   	(tuner_control_callback)tun_tda18250_control_X,   
	   	(tuner_status_callback)tun_tda18250_status,  
	    (tuner_command_callback)tun_tda18250_command, 
	    (tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_tda18250_close
	 },
#endif	

#ifdef CONFIG_TDA18250_AB
	{
	    NIM_DVBC,
	   	TDA18250AB,
	   	(tuner_init_callback)tun_tda18250ab_init,     
	   	(tuner_control_callback)tun_tda18250ab_control,   
	   	(tuner_status_callback)tun_tda18250ab_status, 
		(tuner_command_callback)NULL,
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)NULL
	 },
#endif	


#ifdef CONFIG_DCT70701
	{
	    NIM_DVBC,
	   	DCT70701,
	   	(tuner_init_callback)tun_dct70701_init,     
	   	(tuner_control_callback)tun_dct70701_control,   
	   	(tuner_status_callback)tun_dct70701_status,  
		(tuner_command_callback)NULL,
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_dct70701_release
	 },
#endif	
#ifdef CONFIG_CXD2872
	{
	    NIM_DVBT,
	   	CXD2872,
	    (tuner_init_callback)tun_cxd_ascot3_Init,
        (tuner_control_callback)tun_cxd_ascot3_control,
        (tuner_status_callback)tun_cxd_ascot3_status,
		(tuner_command_callback)tun_cxd_ascot3_command,
		(tuner_gain_callback)NULL,
        (tuner_close_callback)tun_cxd_ascot3_release
	 },
#endif	

#ifdef CONFIG_MXL603
	{
	   	NIM_DVBT,
	   	MXL603,
	   	(tuner_init_callback)tun_mxl603_init,     
	   	(tuner_control_callback)tun_mxl603_control,   
	   	(tuner_status_callback)tun_mxl603_status,  
		(tuner_command_callback)tun_mxl603_command, 
		(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_mxl603_release
	 },
#endif
#ifdef CONFIG_M3031
	{
	   	NIM_DVBS,
	   	M3031,
	   	(tuner_init_callback)tun_m3031_init,     
	   	(tuner_control_callback)tun_m3031_control,   
	   	(tuner_status_callback)tun_m3031_status,
	   	(tuner_command_callback)tun_m3031_command,
	   	(tuner_gain_callback)tun_m3031_gain,
	   	(tuner_close_callback)tun_m3031_release
	 },
#endif
#ifdef CONFIG_SI2141_DVBT
	{
	   	NIM_DVBT,
	   	SI2141,
	   	(tuner_init_callback)tun_si2141_init_dvbt,     
	   	(tuner_control_callback)tun_si2141_control,   
	   	(tuner_status_callback)tun_si2141_status,
	   	(tuner_command_callback)tun_si2141_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_si2141_release
	 },
#endif
#ifdef CONFIG_SI2141_ISDBT
	{
	   	NIM_DVBT,
	   	SI2141,
	   	(tuner_init_callback)tun_si2141_init_isdbt,     
	   	(tuner_control_callback)tun_si2141_control,   
	   	(tuner_status_callback)tun_si2141_status,
	   	(tuner_command_callback)tun_si2141_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_si2141_release
	 },
#endif

#ifdef CONFIG_R858
	{
	   	NIM_DVBC,
	   	R858,
	   	(tuner_init_callback)tun_r858_init,     
	   	(tuner_control_callback)tun_r858_control,   
	   	(tuner_status_callback)tun_r858_status,
	   	(tuner_command_callback)tun_r858_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_r858_release
	 },
#endif

#ifdef CONFIG_RT7X0
	{
	   	NIM_DVBS,
	   	RT7X0,
	   	(tuner_init_callback)tun_rt7x0_init,     
	   	(tuner_control_callback)tun_rt7x0_control,   
	   	(tuner_status_callback)tun_rt7x0_status,
	   	(tuner_command_callback)tun_rt7x0_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_rt7x0_release
	 },
#endif

#ifdef CONFIG_TUN_R836
	{
	   	NIM_DVBC,
	   	TUN_R836,
	   	(tuner_init_callback)tun_r836_dvbc_init,     
	   	(tuner_control_callback)tun_r836_control,   
	   	(tuner_status_callback)tun_r836_status,
	   	(tuner_command_callback)tun_r836_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_r836_release
	 },
#endif

#ifdef CONFIG_TUN_R836
	{
	   	NIM_DVBT,
	   	TUN_R836,
	   	(tuner_init_callback)tun_r836_dvbt2_t_init,     
	   	(tuner_control_callback)tun_r836_control,   
	   	(tuner_status_callback)tun_r836_status,
	   	(tuner_command_callback)tun_r836_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_r836_release
	 },
#endif

#ifdef CONFIG_TUN_R836
	{
	   	NIM_ISDBT,
	   	TUN_R836,
	   	(tuner_init_callback)tun_r836_isdbt_init,     
	   	(tuner_control_callback)tun_r836_control,   
	   	(tuner_status_callback)tun_r836_status,
	   	(tuner_command_callback)tun_r836_command,
	   	(tuner_gain_callback)NULL,
	   	(tuner_close_callback)tun_r836_release
	 },
#endif



};

TUNER_IO_FUNC *tuner_setup(UINT32 type,UINT32 tuner_id)
{
	UINT32 i = 0;
    UINT32 tuner_count =0 ;

	tuner_count = sizeof(g_tuner_array)/sizeof(TUNER_IO_FUNC);
	for(i = 0;i<tuner_count;i++)
	{
		if((g_tuner_array[i].tuner_id == tuner_id) && (g_tuner_array[i].nim_type==type))
		{
			PRINTK_INFO(NIM_LOG_DBG,"[%s]line=%d,found tuner,tunerid=0x%x,i=%d!\n",__FUNCTION__,__LINE__,(unsigned int)tuner_id,(int)i);
			return &g_tuner_array[i];
		}
	}

    PRINTK_INFO(NIM_LOG_DBG,"[%s]line=%d,no found tuner,tuner_id=0x%x,type=%d,return null!\n",__FUNCTION__,__LINE__,(unsigned int)tuner_id,type);
	return NULL;
	
}


