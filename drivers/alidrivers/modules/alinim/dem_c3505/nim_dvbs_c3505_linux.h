/*****************************************************************************
*    Copyright (C)2015 Ali Corporation. All Rights Reserved.
*
*    File:	nim_dvbs_c3505_tds.h
*
*    Description:    
*    History:
*   Date			        Athor			Version		Reason
*   ============    =========   	=======   	=================
* 1.  09/29/2015	Paladin.Ye     	Ver 0.1   	Create file for C3505 DVBS2 project
*
* 2.  05/30/2016       Paladin.ye            Ver 0.2          1. Add ACM mode and multi stream function
                                                                                    2. Solve tone burst 0 issue in 'diseqc_operate' function  
                                                                                    3. Optimize the autoscan function struct, and open macro "CHANNEL_CHANGE_ASYNC"
                                                                                    4. Add timeout for softsearch to fix autoscan loss tp(mosaic) issue
                                                                                    5. Reduce dvb-s mode lock threshold in "nim_c3505_hw_init"
                                                                                    6. Fix some error for demod config in "nim_c3505_hw_init"
                                                                                    7. When in dvb-s2 mode we use a new way to get snr "nim_c3505_get_snr_std"
*																					
* 3.  11/28/2016	Paladin.Ye     	Ver 0.3   	1. Set reg0x106[2] = 1 for upl != 188byte case.
												2. Change nim_c3505_get_snr_std name to nim_c3505_get_snr_db_s2 and fix some bug, used in c3505 and c3503c	
												3. Optimize get_mer function
												4. Check ACM work mode for C3503c in nim_c3505_open
												5. Fix a bug which will cause multi stream search TP slow in DVBS mode
												6. Improve 43.2M symbol rate TP performance in some special case
												7. Fix TP loss lock(pdpd 3430/h/1250) issue.
												8. Optimize PL time band value for reduce lock time of low symbol TP
												9. Add IOCTL:NIM_DRIVER_GET_SPECTRUM/NIM_DRIVER_GET_VALID_FREQ
												10. Move some debug code form get_agc to task, and optimize it
												11. Optimize nim_c3505_autoscan return value for AUI test
												12. Add IOCTL NIM_DRIVER_CAP_DATA for capture demod data to men, and transform HW address in nim_c3505_adc2mem_entity
                                                
* 4.  01/16/2017	Paladin.Ye     	Ver 0.4     1. Add IOCTL NIM_DRIVER_SET_PLS for set specific PLSN, and add it in nim_c3505_channel_change
                                                2. Modifi nim_c3505_get_lock for autoscan fake tp
                                                3. Fix CRB4 to 0x1f in ACM work mode for echo
*******************************************************************************/

#ifndef __LLD_NIM_DVBS_C3505_LINUX_H__
#define __LLD_NIM_DVBS_C3505_LINUX_H__
#define __C3505_LINUX__

#include "../porting_linux_header.h"
//#include "../ali_dvbs_common/porting_m3501_linux.h"
#include "../ali_dvbs_common/unified_bsp_board_attr.h"
#include "nim_dvbs_c3505_private.h"
#include "ali_gpio.h" 

//----------------------------DEFINE for debug---------------------------------//
//#define HW_ADPT_CR_MONITOR 		// print OldAdpt parameters  
//#define HW_ADPT_NEW_CR_MONITOR	// print CR coefficients, no matter OldAdpt or NewAdpt
//#define NEW_CR_ADPT_SNR_EST_RPT 	// print c/n dB value
//#define SW_ADPT_CR					
//#define SW_SNR_RPT_ONLY

//#define C3505_DEBUG_GET_SPECTRUM  // Debug autoscan
//#define DEBUG_IN_TASK
//#define C3505_DEBUG_FLAG                // Debug signal issue and so on
//#define DEBUG_REGISTER                        // Monitor register value

//#define DEBUG_SOFT_SEARCH      		// Try someone TP for special case, for DEbug
//#define DEBUG_CHANNEL_CHANGE      	// Try Get current TP spectrum


#ifdef C3505_DEBUG_FLAG
	#define NIM_PRINTF_DEBUG
    #define MON_PRINTF_DEBUG
	#define ERR_DEBUG
	#define AUTOSCAN_DEBUG
	#define ACM_PRINTF
#endif

#ifdef C3505_DEBUG_GET_SPECTRUM
	#define NIM_PRINTF_DEBUG
	#define ERR_DEBUG
	#define FFT_PRINTF_DEBUG
	#define AUTOSCAN_DEBUG
	#define ACM_PRINTF
#endif

//#define PLSN_DEBUG

#ifdef PLSN_DEBUG
	#define PLSN_PRINTF	printk
#else
	#define PLSN_PRINTF(...)
#endif

#ifdef NIM_PRINTF_DEBUG
	#define NIM_PRINTF(fmt,args...)  printk("NIM_PRINTF: " fmt, ##args)    
#else
    #define NIM_PRINTF(...)
#endif


#ifdef MON_PRINTF_DEBUG
	#define MON_PRINTF(fmt,args...)  printk("NIM_PRINTF: " fmt, ##args)    
#else
    #define MON_PRINTF(...)
#endif
#ifdef ERR_DEBUG
    #define ERR_PRINTF(fmt, args...)  printk("ERR_PRINTF: " fmt, ##args)
#else
	#define ERR_PRINTF(...)
#endif

#ifdef TIME_COST_DEBUG
    #define TIME_COST_PRINTF(fmt, args...)  printk("TIME_COST_PRINTF: " fmt, ##args)
#else
	#define TIME_COST_PRINTF(...)
#endif

#ifdef FFT_PRINTF_DEBUG
    #define FFT_PRINTF(fmt, args...)  printk("FFT_PRINTF: " fmt, ##args)
#else
	#define FFT_PRINTF(...)
#endif

#ifdef HW_ADPT_CR_MONITOR
    #define ADPT_CR_PRINTF(fmt, args...)  printk("ADPT_CR_PRINTF: " fmt, ##args)
#else
	#define ADPT_CR_PRINTF(...)
#endif

#ifdef HW_ADPT_NEW_CR_MONITOR
    #define ADPT_NEW_CR_PRINTF(fmt, args...)  printk("ADPT_NEW_CR_PRINTF: " fmt, ##args)
#else
	#define ADPT_NEW_CR_PRINTF(...)
#endif

#ifdef AUTOSCAN_DEBUG
    #define AUTOSCAN_PRINTF(fmt, args...)  printk("AUTOSCAN_PRINTF: " fmt, ##args)
#else
	#define AUTOSCAN_PRINTF(...)
#endif

#ifdef ACM_PRINTF
	#define ACM_DEBUG_PRINTF(fmt, args...)  printk("ACM_PRINTF: " fmt, ##args)
#else
	#define ACM_DEBUG_PRINTF(...)	
#endif
#ifdef __C3505_LINUX__
typedef struct NIM_AUTO_SCAN         NIM_AUTO_SCAN_T;
typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;
#define OSAL_ID UINT32
#define ID WORD
#define ER long 
#endif

enum DEV_SWITCH
{
    DEV_ENABLE = 0,
    DEV_DISABLE = 1,
};

#ifdef __C3505_TDS__
enum NIM_BLSCAN_MODE
{
    NIM_SCAN_FAST = 0,
	NIM_SCAN_SLOW = 1,
};
#endif


struct nim_c3505_tsk_status
{
	UINT32 m_lock_flag;
	ID m_task_id;
	UINT32 m_sym_rate;
	UINT8 m_work_mode;
	UINT8 m_map_type;
	UINT8 m_code_rate;
	UINT8 m_info_data;
};

struct nim_c3505_t_param
{
	int t_last_snr;		
	int t_last_iter;
	int t_aver_snr;
	int t_snr_state;
	int t_snr_thre1;
	int t_snr_thre2;
	int t_snr_thre3;
	INT32 t_phase_noise_detected;
	INT32 t_dynamic_power_en;
	UINT32 phase_noise_detect_finish;
	UINT32 t_reg_setting_switch;
	UINT8 t_i2c_err_flag;
	UINT8 auto_adaptive_cnt;
    UINT8 auto_adaptive_state;
    UINT8 auto_adaptive_doing;
	UINT32 freq_offset;
};

struct nim_c3505_status
{
	OSAL_ID nim_c3505_sema;
	ER ret;
	UINT8 c3505_autoscan_stop_flag;
	UINT8 c3505_chanscan_stop_flag;
	UINT32 old_ber;
	UINT32 old_per;
	UINT32 old_ldpc_ite_num;
	UINT8 *adc_data;// = (unsigned char *)__MM_DMX_FFT_START_BUFFER;//[2048];
	UINT8 *adc_data_malloc_addr;
	UINT8 *adc_data_raw_addr;
	INT32 m_freq[256];
	UINT32 m_rs[256];
	INT32 fft_i_1024[1024];
	INT32 fft_q_1024[1024];
	UINT8 m_cr_num;
	UINT32 m_cur_freq;
	UINT8 c_rs ;
	UINT32 m_step_freq;
	pfn_nim_reset_callback m_pfn_reset_c3505;
	UINT8 m_enable_dvbs2_hbcd_mode;
	UINT8 m_dvbs2_hbcd_enable_value;
	UINT8 c3505_lock_status;
	UINT8 c3505_lock_adaptive_done;  //In order to fix lowfeq mosaic
	UINT32 phase_err_check_status;
	UINT32 m_c3505_type;
	UINT32 m_c3505_sub_type;
	UINT32 m_setting_freq;
	UINT32 m_err_cnts;
	UINT16 m_hw_timeout_thr;
};

#ifdef __C3505_TDS__
#define NIM_FLAG_LOCK UINT32
#endif
struct nim_c3505_private
{
	INT32	 (*nim_tuner_init) (UINT32 *, struct QPSK_TUNER_CONFIG_EXT *);	 // Tuner Initialization Function
	INT32	 (*nim_tuner_control) (UINT32, UINT32, UINT32);  // Tuner Parameter Configuration Function
	INT32	 (*nim_tuner_status) (UINT32, UINT8 *);  
	/*C3031B Tuner special api function,add by dennis on 20140905*/
	INT32	 (*nim_tuner_command)(UINT32 , INT32 , INT32 * );
	INT32	 (*nim_tuner_gain)(UINT32,UINT32);
	INT32    (*nim_tuner_close) (UINT32);
	struct   QPSK_TUNER_CONFIG_DATA tuner_config_data;
	UINT32 tuner_index;
	UINT32 tuner_id;	    //current Tuner type. 
	UINT32 tuner_type;  	// 3031 or not 3031  
	UINT32 i2c_type_id;
	UINT32 polar_gpio_num;
	UINT32 sys_crystal;
	UINT32 sys_clock;
	UINT16 pre_freq;
	UINT16 pre_sym;
	INT8 autoscan_stop_flag;
	UINT8 search_type;  // 1=AutoScan; 0=ChannelChange
	UINT8 autoscan_debug_flag;  // AUTOSCAN_DEBUG_FLAG
	UINT8 autoscan_control;	//0:normal autoscan, 1:get valid freq, 2:get 64K spectrum,just execute half autoscan
							//attention: you should set the variable base on the function before call nim_s3503_autoscan
	
	#ifdef __C3505_TDS__
	struct nim_device_stats stats;
	#endif
	
	UINT8 chip_id;
	struct EXT_DM_CONFIG ext_dm_config;
	struct nim_c3505_status ul_status;
	UINT32 ext_lnb_id;
	UINT32 ext_lnb_type;
	INT32 (*ext_lnb_command) (UINT32*, UINT32,UINT32);
	struct EXT_LNB_CTRL_CONFIG ext_lnb_config;
	struct nim_c3505_tsk_status tsk_status;
	struct nim_c3505_t_param t_param;
	UINT32 cur_freq;
	UINT32 cur_sym; 
    UINT8 change_type; // For ACM multi stream, 0 = Set TP and get ISID, 1 = Set TP and set ISID, 2 = Onyl set ISID
	NIM_FLAG_LOCK flag_id;
	enum NIM_BLSCAN_MODE blscan_mode;
	OSAL_ID c3505_mutex;
	struct nim_dvbs_channel_info *channel_info;
	struct nim_dvbs_bb_header_info *bb_header_info;
	struct nim_dvbs_isid *isid;

	struct mutex plsn_mutex;
	PLS plsn;
    OSAL_ID scan_mutex;
#ifdef __C3505_LINUX__
	struct kumsgq 				   *nim_kumsgq;
	struct mutex 					i2c_mutex;
	struct mutex 					multi_process_mutex;
	struct mutex 					tuner_open_mutex;
	NIM_AUTO_SCAN_T 				as_info;
	struct t_diseqc_info 			diseqc_info;
	struct QPSK_TUNER_CONFIG_EXT 	tuner_config;
	AUTO_SCAN_PARA 					blind_msg;
	NIM_FLAG_LOCK					flag_lock;
	UINT8 							dev_idx;
	UINT8 							nim_used;
	BOOL 							yet_return;
	BOOL							nim_init;
	unsigned char 					as_status;
	UINT8 							work_alive;
	UINT8							tuner_opened;
	wait_queue_head_t       		as_sync_wait_queue;
#endif
    struct nim_tso_cfg nim_tso_cfg;
	//1:channel change or autoscan is working now;0:channel change or autoscan finished
	UINT8 							chanscan_autoscan_flag;
	//1:function waiting_channel_lock finished;0:function waiting_channel_lock is working now
	UINT8							wait_chanlock_finish_flag;
}; 

struct nim_c3505_tp_scan_para
{
    UINT32 est_rs;      // Softsearch mode = Estimate rs from search arithmetic;  Channel change mode = The rs from upper software
	UINT32 correct_rs;
	INT32 est_freq;    // Softsearch mode = Estimate freq from search arithmetic;  Channel change mode = The freq from upper software
	UINT32 tuner_freq;
	UINT32 est_fec;     // Unused 
    UINT32 low_sym;     // 1 = low sym(less than LOW_SYM_THR); 0 = normal
    INT32  freq_err;    // 1/256M only valid for M3031
    INT32  delfreq;     // Delta freq set to demod to correct deomdsearch freq point
    UINT32 pl_lock_thr; // Used for softsearch
    UINT32 tr_lock_thr; // Used for softsearch
    UINT32 cr_lock_thr; // Used for softsearch
    UINT32 fs_lock_thr; // Used for softsearch    
    UINT32 search_status; // 0 = initial, 1 = has cr lock
    UINT8 change_work_mode_flag; // 0: Try s2, 1: Try s
};

#ifdef __C3505_LINUX__
/*#define LNB_CMD_BASE			0xf0
#define LNB_CMD_ALLOC_ID		(LNB_CMD_BASE+1)
#define LNB_CMD_INIT_CHIP		(LNB_CMD_BASE+2)
#define LNB_CMD_SET_POLAR		(LNB_CMD_BASE+3)
#define LNB_CMD_POWER_EN		(LNB_CMD_BASE+4)
*/
#define NIM_TUNER_C3031B_ID			NIM_TUNER_M3031_ID
#define	NIM_TUNER_SET_C3031B_FREQ_ERR   NIM_TUNER_SET_M3031_FREQ_ERR
#define	NIM_TUNER_GET_C3031B_FREQ_ERR   NIM_TUNER_GET_M3031_FREQ_ERR
#define	NIM_TUNER_GET_C3031B_GAIN_FLAG  NIM_TUNER_GET_M3031_GAIN_FLAG

#define HAL_GPIO_BIT_SET ali_gpio_set_value

extern RET_CODE 	nim_send_as_msg(struct kumsgq * nim_kumsgq, unsigned char lck, unsigned char polar, unsigned short freq, 
                                unsigned int sym, unsigned char fec, unsigned char as_stat);
extern __u32 nim_c3505_dvbs_as_cb2_ui(void *p_priv, __u8 lck, __u8 polar, __u16 freq, __u32 sym, __u8 fec, __u8 as_stat);
                               
extern INT32 		nim_callback(NIM_AUTO_SCAN_T *pst_auto_scan, void *pfun, UINT8 status, UINT8 polar, 
                             UINT16 freq, UINT32 sym, UINT8 fec, UINT8 stop); 
#endif

extern UINT8  *dram_base_t;
                              
//---------------System Essensial Function --------------//
DWORD nim_c3505_multu64div(UINT32 v1, UINT32 v2, UINT32 v3);
INT32 nim_c3505_crc8_check(void *input, INT32 len,INT32 polynomial);
UINT32 nim_c3505_Log10Times100_L( UINT32 x);

extern INT32 nim_c3505_generate_table(struct nim_device *dev);
extern INT32 nim_c3505_start_generate(struct nim_device *dev, struct ali_plsn_address *p_addr);
extern INT32 nim_c3505_release_table(struct nim_device *dev);
extern INT32 nim_c3505_search_plsn_top(struct nim_device *dev);
extern INT32 nim_c3505_plsn_gold_to_root(INT32 plsn_gold);
extern INT32 nim_c3505_plsn_root_to_gold(INT32 plsn_root);
extern UINT8 g_table_finish_flag;


#endif	// __LLD_NIM_DVBS_C3505_TDS_H__ */

