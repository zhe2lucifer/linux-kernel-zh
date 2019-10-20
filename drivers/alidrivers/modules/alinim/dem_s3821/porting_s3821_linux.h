#ifndef __PORTING_S3821_LINUX_H__
#define __PORTING_S3821_LINUX_H__

#include "../porting_linux_header.h"
#include "../basic_types.h"
#include "../tun_common.h"
#include "../nim_device.h"


#ifdef __cplusplus
extern "C" {
#endif


#define SYS_FUNC_ON				0x00000001	/* Function on */


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
    UINT32   frequency;
    INT32    freq_offset;  /* (ter) */
} s3821_lock_info;




typedef struct  nim_s3821_private
{
	dvbt_tuner_init_callback   nim_tuner_init;    // Tuner Initialization Function 
	tuner_control_callback     nim_tuner_control; // Tuner Parameter Configuration Function 
	tuner_status_callback      nim_tuner_status;  // Get Tuner Status Function 
    tuner_close_callback       nim_tuner_close;   // Close Function.

    struct COFDM_TUNER_CONFIG_API tuner_control;
    UINT32                     tuner_id;

    //joey, 20130625, to extend the info bit.
    UINT8                      cofdm_type; //0:ISDBT_TYPE, 1:DVBT_TYPE, 2:DVBT2, 3:DVBT2/T-combo...
    UINT8                      cur_type; //0:ISDBT_TYPE, 1:DVBT_TYPE, 2:DVBT2, no other case, means the real work mode.

//joey, 20140228. to refine tune the dvbt2 only search function.
    UINT8                      search_t2_only; //0: Not. 1: Search T2 only. bigger than 1: future extand.

//joey, 20140527, for DVBT2 FEF SSI display.
	UINT8                      last_ssi; // to record last time the ssi report value when FEF situation.
	UINT8                      con_ssi_small_cnt; // to record the continuous ssi small counter under FEF situation.

    UINT8                      m_reg_page;
    s3821_lock_info            s3821_cur_channel_info;
    
    UINT32                     autoscan_stop_flag;
    UINT32                     snr_ber;
    UINT32                     snr_per;
    UINT32                     per_tot_cnt;
    UINT32                     rec_ber_cnt;
    //joey, 20130724, for CNR estimation.
    UINT32                     cnr_info;
    //UINT8 cci_post_shut; // record current CCI post on/off status.
    UINT8                      flt_cci_info_cnt; // to do a easy filter for CCI iinformation update.

    //joey, 20130730, for ADC capture force demod unlock.
    UINT8                      rec_init_offset;

    //joey, 20130730, move some static variable to private structure.
    UINT8                      rec_snr;
    UINT16                     g_tuner_if_freq;
    BOOL                       reset_patch_trggered;

    nim_ad_gain_table_t        rf_ad_gain_table;
    nim_ad_gain_table_t        if_ad_gain_table;
    UINT32                     log_en: 1;
    UINT32                     mon_ifft_en: 1;
    UINT32                     ifft_play_cnt: 8;
    UINT32                     reserved: 22;

    nim_s3821_string_out_fnp_t fn_output_string;
    char                       *output_buffer;
    UINT16                     *ifft_result;
    UINT8                      cci_pre_status;
    UINT8                      tps_cnt;
	UINT32	                   base_addr;					// Demodulator address 
	
    UINT32                     close_flag;
    NIM_FLAG_LOCK			   flag_lock;
    struct mutex  	      	   i2c_mutex;
    struct workqueue_struct	  *workqueue;
    struct work_struct 		   work;
	
} NIM_S3821_PRIVATE, *PNIM_S3821_PRIVATE;




typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;
               




#ifdef __cplusplus
}
#endif

#endif

