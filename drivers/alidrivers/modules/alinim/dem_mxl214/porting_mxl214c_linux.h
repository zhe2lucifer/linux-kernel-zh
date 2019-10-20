#ifndef __PORTING_MXL214C_LINUX_H__
#define __PORTING_MXL214C_LINUX_H__

#include "../porting_linux_header.h"

#include "../basic_types.h"
#include "../tun_common.h"

#include "../nim_device.h"



#ifdef __cplusplus
extern "C" {
#endif




typedef struct _mxl214c_lock_info
{	
	UINT32	freq;
	UINT32	symbol_rate;
	UINT8	modulation;
}MXL214C_LOCK_INFO;




struct  nim_mxl214c_private
{
    struct QAM_TUNER_CONFIG_DATA  tuner_config_data;
    struct QAM_TUNER_CONFIG_EXT   tuner_config_ext;
	struct EXT_DM_CONFIG          ext_dem_config;
	
    MXL214C_LOCK_INFO             lock_info;
	
    UINT32                        tuner_id;
    UINT32                        qam_mode;
	UINT8 				          dev_idx;
    struct mutex  		          i2c_mutex;
    struct workqueue_struct	      *workqueue;
    struct work_struct 		      work;

};


typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;
               

#ifdef __cplusplus
}
#endif

#endif

