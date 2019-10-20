#ifndef _PORTING_MXL214C_TDS_H_
#define _PORTING_MXL214C_TDS_H_

#include <sys_config.h>
#include <types.h>
#include <retcode.h>
#include <hld/nim/nim_dev.h>
#include <hld/nim/nim_tuner.h>
#include <osal/osal.h>
#include <api/libc/alloc.h>
#include <api/libc/printf.h>
#include <api/libc/string.h>
#include <hal/hal_gpio.h>
#include <bus/i2c/i2c.h>
#include <hld/hld_dev.h>
#include <hld/nim/nim.h>
#include <bus/tsi/tsi.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


#define nim_print          libc_printf


#define comm_malloc 			MALLOC
#define comm_memset				MEMSET
#define comm_free				FREE
#define comm_delay				nim_comm_delay
#define comm_sleep				osal_task_sleep
#define comm_memcpy				memcpy

#define nim_i2c_read			i2c_read
#define nim_i2c_write			i2c_write
#define nim_i2c_write_read		i2c_write_read

#define div_u64(a,b)			a=(a/b)
//div_u64(ulHigh, pObj->sConfig.uSamplingClock);

void                             nim_comm_delay(UINT32 us);



#define NIM_MUTEX_ENTER(priv)      os_lock_mutex(priv->i2c_mutex, OSAL_WAIT_FOREVER_TIME)
#define NIM_MUTEX_LEAVE(priv)     os_unlock_mutex(priv->i2c_mutex)


typedef struct _mxl214c_lock_info
{	
	UINT32	freq;
	UINT32	symbol_rate;
	UINT8	modulation;
}MXL214C_LOCK_INFO;






struct nim_mxl214c_private
{
	struct QAM_TUNER_CONFIG_API qam_tuner_config;
	UINT32 tuner_id;
};






typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;




#ifdef __cplusplus
}
#endif

#endif

