#ifndef _PORTING_S3821_TDS_H_
#define _PORTING_S3821_TDS_H_

#include <sys_config.h>
#include <retcode.h>
#include <types.h>
#include <osal/osal.h>
#include <api/libc/alloc.h>
#include <api/libc/printf.h>
#include <api/libc/string.h>
#include <hal/hal_gpio.h>
#include <hld/hld_dev.h>
#include <hld/nim/nim_dev.h>
#include <hld/nim/nim.h>
#include <hld/nim/nim_tuner.h>
#include <bus/i2c/i2c.h>
#include <bus/tsi/tsi.h>
#include <math.h>
#include <hld/dmx/dmx_dev.h>
#include <hld/dmx/dmx.h>



#ifdef __cplusplus
extern "C" {
#endif


#define nim_print                  libc_printf


#define comm_malloc                MALLOC
#define comm_memset                MEMSET
#define comm_free                  FREE
#define comm_delay                 nim_comm_delay
#define comm_sleep                 osal_task_sleep
#define comm_memcpy                MEMCPY

#define nim_i2c_read               i2c_read
#define nim_i2c_write              i2c_write
#define nim_i2c_write_read         i2c_write_read

#define div_u64(a,b)            a=(a/b)
//div_u64(ulHigh, pObj->sConfig.uSamplingClock);

void                               nim_comm_delay(UINT32 us);



#define NIM_MUTEX_ENTER(priv)       os_lock_mutex(priv->i2c_mutex, OSAL_WAIT_FOREVER_TIME)
#define NIM_MUTEX_LEAVE(priv)       os_unlock_mutex(priv->i2c_mutex)

typedef void (*nim_s3821_string_out_fnp_t)(char *string);

typedef struct
{

    UINT8    fecrates;
    UINT8    modulation;
    UINT8    mode;            /* (ter) */
    UINT8    guard;           /* (ter) */
    UINT8    hierarchy;       /* (ter) */
    UINT8    spectrum;
    UINT8    channel_bw;       /* (ter) */
    UINT32    frequency;
    INT32    freq_offset;  /* (ter) */
} s3821_lock_info;

typedef struct _flag_lock
{
    UINT32 			flag_id; 	  //Asynchronous channel change control flag
}NIM_FLAG_LOCK;




typedef struct  nim_s3821_private
{

    struct COFDM_TUNER_CONFIG_API tuner_control;
    UINT32                    tuner_id;

    //joey, 20130625, to extend the info bit.
    UINT8                     cofdm_type; //0:ISDBT_TYPE, 1:DVBT_TYPE, 2:DVBT2, 3:DVBT2/T-combo...
    UINT8                     cur_type; //0:ISDBT_TYPE, 1:DVBT_TYPE, 2:DVBT2, no other case, means the real work mode.

//joey, 20140228. to refine tune the dvbt2 only search function.
    UINT8                     search_t2_only; //0: Not. 1: Search T2 only. bigger than 1: future extand.

//joey, 20140527, for DVBT2 FEF SSI display.
	UINT8                     last_ssi; // to record last time the ssi report value when FEF situation.
	UINT8                     con_ssi_small_cnt; // to record the continuous ssi small counter under FEF situation.

    UINT8                     m_reg_page;
    s3821_lock_info           s3821_cur_channel_info;


    UINT32                    autoscan_stop_flag;
    UINT32                    snr_ber;
    UINT32                    snr_per;
    UINT32                    per_tot_cnt;
    UINT32                    rec_ber_cnt;
    //joey, 20130724, for CNR estimation.
    UINT32                    cnr_info;
    //UINT8 cci_post_shut; // record current CCI post on/off status.
    UINT8                     flt_cci_info_cnt; // to do a easy filter for CCI iinformation update.

    //joey, 20130730, for ADC capture force demod unlock.
    UINT8                      rec_init_offset;

    //joey, 20130730, move some static variable to private structure.
    UINT8                      rec_snr;
    UINT16                     g_tuner_if_freq;
    BOOL                       reset_patch_trggered;

    nim_ad_gain_table_t        rf_ad_gain_table;
    nim_ad_gain_table_t        if_ad_gain_table;
    UINT32                     log_en : 1;
    UINT32                     mon_ifft_en: 1;
    UINT32                     ifft_play_cnt: 8;
    UINT32                     reserved : 22;

    nim_s3821_string_out_fnp_t fn_output_string;
    char                       *output_buffer;
    UINT16                     *ifft_result;
    UINT8                      cci_pre_status;
    UINT8                      tps_cnt;

    UINT32	                   base_addr;					// Demodulator address 

	UINT32                     close_flag;
    NIM_FLAG_LOCK              flag_lock;
    OSAL_ID                    i2c_mutex;    //for i2c write and read mutec protection.	
    OSAL_ID                    thread_id;

} NIM_S3821_PRIVATE, *PNIM_S3821_PRIVATE;



typedef struct NIM_CHANNEL_CHANGE NIM_CHANNEL_CHANGE_T;

UINT32 		nim_flag_read(NIM_FLAG_LOCK *flag_lock, UINT32 T1, UINT32 T2, UINT32 T3);
UINT32 		nim_flag_create(NIM_FLAG_LOCK *flag_lock);
UINT32 		nim_flag_set(NIM_FLAG_LOCK *flag_lock, UINT32 value);
UINT32 		nim_flag_clear(NIM_FLAG_LOCK *flag_lock, UINT32 value);
UINT32 		nim_flag_del(NIM_FLAG_LOCK *flag_lock);



#ifdef __cplusplus
}
#endif

#endif

