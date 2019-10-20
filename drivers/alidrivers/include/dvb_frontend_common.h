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

#ifndef _DVB_FRONTEND_COMMON_H_
#define _DVB_FRONTEND_COMMON_H_

#ifndef _KERNEL_
#include <linux/types.h>
#endif
#include <alidefinition/adf_sysdef.h>
#include <alidefinition/adf_nim.h>

/*! @addtogroup DeviceDriver
 *  @{
  */
 
/*! @addtogroup ALiNIM
 *  @{
    */
 
 
/*! def  DISEQC_CMD_MAX_LEN
@brief for DVB-S/S2
*/
#define DISEQC_CMD_MAX_LEN 16

/*!@enum  QAM_MODE
@brief for DVB-C
*/
typedef enum qam_mode_t
{
   QAM16 =4,
   QAM32,
   QAM64,
   QAM128,
   QAM256
}QAM_MODE; 

/*!@enum  AGC_TIMECST
@brief for DVB-C
*/
typedef enum timecst_agc_t
{
   SLOW_TIMECST_AGC=0,
   FAST_TIMECST_AGC
}AGC_TIMECST;


/*!@enum  NIM_DISEQC_MODE
@brief for DVB-S/S2, DiSEqC mode value.
*/
typedef enum nim_diseqc_mode_t
{
   NIM_DISEQC_MODE_22KOFF =0,            //!< 22kHz off 
   NIM_DISEQC_MODE_22KON,                //!< 22kHz on
   NIM_DISEQC_MODE_BURST0,               //!< Burst mode, on for 12.5mS = 0
   NIM_DISEQC_MODE_BURST1,               //!< Burst mode, modulated 1:2 for 12.5mS = 1
   NIM_DISEQC_MODE_BYTES,                //!< Modulated with bytes from DISEQC INSTR 
   NIM_DISEQC_MODE_ENVELOP_ON,           //!< Envelop enable
   NIM_DISEQC_MODE_ENVELOP_OFF,          //!< Envelop disable, output 22K wave form
   NIM_DISEQC_MODE_OTHERS,               //!< Undefined mode
   NIM_DISEQC_MODE_BYTES_EXT_STEP1,      //!<Split NIM_DISEQC_MODE_BYTES to 2 steps to improve the speed    
   NIM_DISEQC_MODE_BYTES_EXT_STEP2       //!<(30ms--->17ms) to fit some SPEC 
}NIM_DISEQC_MODE;

 

/*!@enum  NIM_PORLAR
@brief For DVB-S/S2
*/
typedef enum nim_porlar_t
{
   NIM_PORLAR_HORIZONTAL =0x00,
   NIM_PORLAR_VERTICAL,
   NIM_PORLAR_LEFT,
   NIM_PORLAR_RIGHT
}NIM_PORLAR;

/*!@enum  NIM_PORLAR_OP
@brief for DVB-S/S2 
*/
typedef enum nim_porlar_op_t
{
   NIM_PORLAR_REVERSE =0x01,
    NIM_PORLAR_SET_BY_22K
}NIM_PORLAR_OP;

/*!@enum  DISEQC2X_ERRCODE
@brief For DVB-S/S2, DISEQC2X error value.
*/
typedef enum diseqc2x_err_t
{
   DISEQC2X_ERR_NO_REPLY = 0x01,
   DISEQC2X_ERR_REPLY_PARITY,
   DISEQC2X_ERR_REPLY_UNKNOWN,
   DISEQC2X_ERR_REPLY_BUF_FUL,
}DISEQC2X_ERRCODE;

/*!@enum  nim_cofdm_work_mode
@brief For dvbt work mode
*/
enum nim_cofdm_work_mode
{
    NIM_COFDM_SOC_MODE = 0, 
    NIM_COFDM_ONLY_MODE = 1
};


/*!@enum  CHANNEL_CHANGE_USAGE_TYPE
@brief For channel change usage type
*/
typedef enum
{
    USAGE_TYPE_AUTOSCAN = 0,    //Try to auto detect the signal type (such as DVB-T or DVB-T2).
    USAGE_TYPE_CHANSCAN = 1,    //Try to auto detect the signal type (such as DVB-T or DVB-T2).
    USAGE_TYPE_CHANCHG  = 2,    //Tune quickly for play program, don't need to auto detect the signal type (such as DVB-T or DVB-T2).
    USAGE_TYPE_AERIALTUNE  = 3,    //Try to auto detect the signal type (such as DVB-T or DVB-T2), but don't spend time to wait DVB-T2 sync locked, just for improving user experience.
    USAGE_TYPE_NEXT_PIPE_SCAN = 4,  //Try to auto detect the next signal pipe within this channel, 
                                    //such as the next PLP of (MPLP of DVB-T2), or the next priority signal(Hierarchy of DVB-T).
                                    //Before USAGE_TYPE_NEXT_PIPE_SCAN can be used, you must call USAGE_TYPE_AUTOSCAN or USAGE_TYPE_CHANSCAN first.
}CHANNEL_CHANGE_USAGE_TYPE;


/*!@enum  NIM_TUNER_COMMAND_TYPE
@brief For nim tuner command
*/
typedef enum 
{
  // The following code is for tuner io command
    NIM_TUNER_COMMAND = 0x8000, 
    NIM_TUNER_POWER_CONTROL, 
    NIM_TUNER_GET_AGC, 
    NIM_TUNER_GET_RSSI_CAL_VAL,
    NIM_TUNER_RSSI_CAL_ON, //RSSI calibration on
    NIM_TUNER_RSSI_CAL_OFF,
    NIM_TUNER_RSSI_LNA_CTL,//RSSI set LNA
    NIM_TUNER_SET_GPIO,
    NIM_TUNER_CHECK, 

    NIM_TURNER_SET_STANDBY,/*set av2012 standby add by bill 2012.02.21*/

    NIM_TUNER_SET_THROUGH_MODE,
    NIM_TUNER_GET_RF_POWER_LEVEL,    //Get tuner power level for SQI/SSI.

  	NIM_TUNER_WAKE_UP,
  	NIM_TUNER_RW_REG,  //r/w tuner reg by leo.leo 2018,01.22
    NIM_TUNER_GET_CHIPID, //read chip id
}NIM_TUNER_COMMAND_TYPE;


/*!@enum  _nim_log_level_t
@brief For nim log level
*/
enum _nim_log_level_t
{
	NIM_LOG_NULL = 0,
    NIM_LOG_I2C,
	NIM_LOG_DBG,
	NIM_LOG_IC,
	NIM_LOG_AUTOSCAN,
	NIM_LOG_ERR,

	NIM_LOG_MAX
};

//autoscan type
enum AUTOSCAN_TYPE
{
    NORMAL_MODE = 0,        // Do 20 times fft, do channel search, program search
    GET_TP_ONLY = 1,        // Do 20 times fft, do channel search
	GET_SPECTRUM_ONLY = 2,  // Do 10 times fft
};

/*!@struct _nim_dev_name_t
@brief The structure for nim device match table
*/
typedef struct _nim_dev_name_t
{
	__u8   dev_name[20];       //Nim device name 
	__u8   dev_path[32];       //Nim device path
	__u8   dev_opened;         //Nim device open flag
}NIM_DEV_NAME;


/*!@struct nim_ad_gain_table_t
@brief The structure for RSSI display
*/
typedef __u16 nim_ad_gain_t;
typedef struct
{
	nim_ad_gain_t *p_table;
	int count;
}nim_ad_gain_table_t;



/*!@struct EXT_DM_CONFIG
@brief The structure passes the argument of DMOD config extension data to driver from board config (for QPSK).
*/
struct EXT_DM_CONFIG
{
	__u32 i2c_base_addr;           //!<I2C Base address for Write Operation: (BaseAddress + 1) for Read 
	__u32 i2c_type_id;             //!<i2c type and dev ID select
	__u32 dm_crystal;              //!<Used Crystal: in KHz unit 
	__u32 dm_clock;                //!<Clock 
	__u32 polar_gpio_num;          
  __u32 lock_polar_reverse;      
};

/*!@struct EXT_DM_CONFIG
@brief The structure LNB config extension data to driver from board config (for QPSK).
*/
struct EXT_LNB_CTRL_CONFIG
{
	__u32 param_check_sum; //!<ext_lnb_control+i2c_base_addr+i2c_type_id = param_check_sum
	int  (*ext_lnb_control) (int, int, int);  //!<Externel callback
	__u32 i2c_base_addr;            //!<I2C base address
	__u32 i2c_type_id;              //!<I2C type id
	__u8 int_gpio_en;               //!<GPIO enable
	__u8 int_gpio_polar;            //!<GPIO polar
	__u8 int_gpio_num;              //!<GPIO number
};

/*!@struct QAM_TUNER_CONFIG_DATA
@brief QAM_TUNER_CONFIG_DATA config parameter
*/


struct DEMOD_CONFIG_ADVANCED
{
    __u32  qam_config_advanced; //bit0: demode_mode 0:j83b 1:j83ac;
                                 //bit1: ad sample clock 0: 27m, 1:54m;
    __u32  qam_buffer_len;      //buffer length
    __u32  qam_buffer_addr;     //buffer address
};

typedef enum//For  qam mode  select, add by robin for R858 tuner use
{
    QAM_MODE_J83AC = 0,
    QAM_MODE_J83B
} QAM_MODE_STR ;


/*!@struct QAM_TUNER_CONFIG_DATA
@brief The structure passes the argument of tuner config data to driver from board config (for QAM).
*/
#if 1  //ndef _NIM_TUNER_H_
struct QAM_TUNER_CONFIG_DATA
{
	__u8 RF_AGC_MAX;     //!<Maximum value of RF_AGC, x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8 RF_AGC_MIN;     //!<Minimum value of RF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8 IF_AGC_MAX;     //!<Maximum value of IF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8 IF_AGC_MIN;     //!<Minimum value of IF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8 AGC_REF;         //!<The reference value of AGC,the average amplitude to full scale of A/D. % percentage rate.
	__u8 c_tuner_tsi_setting;   //!<TSI selection. 
};

/*!@struct QAM_TUNER_CONFIG_EXT
@brief The structure passes the argument of tuner configuration extension data to driver from board config (for QAM).
*/
struct QAM_TUNER_CONFIG_EXT
{
	__u8  c_tuner_crystal;        //!<Tuner Used Crystal: in KHz unit
	__u8  c_tuner_base_addr;		//!< Tuner Base I2C address for Write Operation: (BaseAddress + 1) for Read 
	__u8  c_chip;                //!<Tuner chip type. 
	__u8  c_tuner_special_config;		//!<Tuner special configuration. If the value is set to 0x01, RF AGC is disabled
	__u8  c_tuner_ref_divratio;       //!<Tuner ref div ratio. 
	__u16 w_tuner_if_freq;            //!<Intermediate frequency
	__u8  c_tuner_agc_top;          //!<AGC top
	__u8  c_tuner_step_freq;        //!<The tuner step frequency
	__u32 i2c_type_id;	          //!<I2C type and dev id select. bit16~bit31: type, I2C_TYPE_SCB/I2C_TYPE_GPIO. bit0~bit15:device ID, 0/1.
	__u8 c_tuner_freq_param;     //!< RT810_Standard_Type 
	__u16 c_tuner_reopen;        //!<A flag of Tuner reopen
	__u16 w_tuner_if_freq_j83a;    //!<J83A Intermediate frequency
	__u16 w_tuner_if_freq_j83b;    //!<J83B Intermediate frequency
	__u16 w_tuner_if_freq_j83c;    //!<J83C Intermediate frequency
	__u8  w_tuner_if_j83ac_type;   //!<Select J83AC/J83B,0x00 j83a , 0x01 j83c
	QAM_MODE_STR  qam_mode;//add by robin for R858 tuner use	
};


/*!@struct QAM_TUNER_CONFIG_API
@brief QAM_TUNER_CONFIG_API config parameter
*/

struct QAM_TUNER_CONFIG_API
{
	/* struct for QAM Configuration */
	struct QAM_TUNER_CONFIG_DATA tuner_config_data;

	/* Tuner Initialization Function */
	int (*nim_tuner_init)(__u32 * ptrTun_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config);

	/* Tuner Parameter Configuration Function */
	int (*nim_tuner_control)(__u32 Tun_id, __u32 freq, __u32 sym, __u8 AGC_Time_Const, __u8 _i2c_cmd);//since there will no bandwidth demand, so pass "sym" for later use.

	/* Get Tuner Status Function */
	int (*nim_tuner_status)(__u32 Tun_id, __u8 *lock);

	/* Extension struct for Tuner Configuration */
	struct QAM_TUNER_CONFIG_EXT tuner_config_ext;
	
	struct EXT_DM_CONFIG ext_dem_config;
	struct DEMOD_CONFIG_ADVANCED dem_config_advanced;

	__u32 qam_mode;                                         //!<J83A/J83B/J83C and Sample CLK select
    __u32 demod_index;                                       // poe add

	
	__u32 tuner_id;// kent, for defferent tuner
};






/*!@struct QPSK_TUNER_CONFIG_EXT
@brief The structure passes the argument of tuner configuration extension data to driver from board configuration. (for QPSK).
*/
struct QPSK_TUNER_CONFIG_EXT
{
	__u16 							w_tuner_crystal;			//!< Tuner Used Crystal: in KHz unit 
	__u8  							c_tuner_base_addr;		//!< Tuner Base address for Write Operation: (BaseAddress + 1) for Read 
	__u8  							c_tuner_out_S_d_sel;		//!< Tuner Output mode Select: 1 --> Single end, 0 --> Differential 
	__u32 							i2c_type_id;	//!<I2C type and device ID select. bit16~bit31: type, I2C_TYPE_SCB/I2C_TYPE_GPIO. bit0~bit15:device ID, 0/1.	
};

/*!@struct QPSK_TUNER_CONFIG_DATA
@brief The structure passes the argument of tuner config data to driver from board config (for QPSK).
*/
struct QPSK_TUNER_CONFIG_DATA
{
	__u16 						recv_freq_low;      //!<Low value of Tuner Receiving Frequency range
	__u16 						recv_freq_high;     //!<High value of Tuner Receiving Frequency range
	__u16 						ana_filter_bw;      
	__u8 							connection_config;  //!<The meaning of each bit as follows:
                                        //!<bit2: I/Q swap
                                        //!<bit1: I_Diff swap
                                        //!<bit0: Q_Diff swap (0: no, 1: swap)

	__u8 							reserved_byte;     //!<Reserved byte
	__u8 							agc_threshold_1;   //!<AGC threshold 1
	__u8 							agc_threshold_2;   //!<AGC threshold 2
	__u16 						qpsk_config;     //!<QPSK configuration. The meaning of each bit as follows:
                                       //!<bit0:QPSK_FREQ_OFFSET,bit1:EXT_ADC,bit2:IQ_AD_SWAP,bit3:I2C_THROUGH,bit4:polar revert bit5:NEW_AGC1,bit6bit7:QPSK bitmode:
	                                     //!<00:1bit,01:2bit,10:4bit,11:8bit
	                                     
	int disqec_polar_position;/*control disqec_polar by gpio*/
};



/*!@struct QPSK_TUNER_CONFIG_API
@brief The structure passes the argument of tuner config data to driver from board config (for QPSK).
*/
struct QPSK_TUNER_CONFIG_API
{

	struct QPSK_TUNER_CONFIG_DATA config_data;   //!<See struct #QPSK_TUNER_CONFIG_DATA
	int (*nim_tuner_init) (__u32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);  //!<callback,Tuner initialization function
	int (*nim_tuner_control) (__u32 tuner_id, __u32 freq, __u32 sym);   //!<Callback,Tuner Parameter Configuration Function
	int (*nim_tuner_status) (__u32 tuner_id, __u8 *lock);            //!<Callback,Get Tuner Status Function 

	struct QPSK_TUNER_CONFIG_EXT tuner_config;   //!<Extension structure for Tuner Configuration 
	struct EXT_DM_CONFIG ext_dm_config;          //!<External demodulator configuration parameter
	struct EXT_LNB_CTRL_CONFIG ext_lnb_config;   //!<Extension structure for LNB Configuration
	__u32 device_type;	                         //!<Current chip type. only used for M3501A
    __u32 demod_index;                           //!<Multi demodulator control,the index of demodulator
	__u32 tuner_id;                              //!<The identifier of tuner
};

/*!@struct nim_m3501_sig_status
*brief The struct sig status data for OpenTV API request
*/
struct nim_m3501_sig_status
{
	__u8		    fec;         //!<Forward error correction
	__u8       roll_off;    //!<Rolloff(alpha)
	__u8       modulation;  //!<Modulation schema
	__u32      polar;       //!<LNB Polarisation
	__u32	    dvb_mode;	 //!<non-zero for S2, 0 for S
};
/*!@struct nim_m3501_quality_info
*brief The struct quality info for OpenTV API request
*/
struct nim_m3501_quality_info
{
	int      iA;  //!<The coefficient A in (A x 10^B)
	int	     iB;	 //!<The coefficient B in (A x 10^B)
};


/*!@struct COFDM_TUNER_CONFIG_EXT
@brief The structure passes the argument of tuner config extension data to driver from board config (for COFDM).
*/
struct COFDM_TUNER_CONFIG_EXT
{
	__u16 c_tuner_crystal;       //!<Tuner Used Crystal: in KHz unit
	__u8  c_tuner_base_addr;	 	 //!<Tuner BaseAddress for Write Operation: (BaseAddress + 1) for Read
	__u8  c_chip;                //!<Chip flag
	__u8  c_tuner_ref_divratio;  //!<Tuner refenrence div ratio
	__u16 w_tuner_if_freq;       //!<Intermediate frequency
	__u8  c_tuner_agc_top;       //!<Tuner agc top
	__u16 c_tuner_step_freq;     //!<tuner step frequency
	int  (*tuner_write)(__u32 id, __u8 slv_addr, __u8 *data, int len);		//!<Callback function which writes the register on the tuner.
	int  (*tuner_read)(__u32 id, __u8 slv_addr, __u8 *data, int len);		//!<Callback function which reads the register on the tuner.
	int  (*tuner_write_read)(__u32 id, __u8 slv_addr, __u8 *data, __u8 wlen,int len); //!<Callback function which write_read the register on the tuner.
	__u32 i2c_type_id;	//!<i2c type and dev id select. bit16~bit31: type, I2C_TYPE_SCB/I2C_TYPE_GPIO. bit0~bit15:dev id, 0/1.
                      
	__u16 cofdm_config; //!< copy from struct COFDM_TUNER_CONFIG_DATA  in order to  let tuner knows whether the RF/IF AGC is enable or not.
	                    //!< esp for max3580, which uses this info to turn on/off internal power detection circuit. See max3580 user manual for detail.
	                    //!<bit0: IF-AGC enable <0: disable, 1: enalbe>;bit1: IF-AGC slop <0: negtive, 1: positive>
	                    //!<bit2: RF-AGC enable <0: disable, 1: enalbe>;bit3: RF-AGC slop <0: negtive, 1: positive>
	                    //!<bit4: Low-if/Zero-if.<0: Low-if, 1: Zero-if>
	                    //!<bit5: RF-RSSI enable <0: disable, 1: enalbe>;bit6: RF-RSSI slop <0: negtive, 1: positive>

	int  if_signal_target_intensity;  //!<if signal taget intensity
};


/*!@struct COFDM_TUNER_CONFIG_DATA
@brief The structure passes the argument of tuner config data to driver from board config (for COFDM).
*/
struct COFDM_TUNER_CONFIG_DATA
{
    #if 0
	__u8 *ptmt352;
	__u8 *ptmt353;
	__u8 *ptst0360;	
	__u8 *ptst0361;
	__u8 *ptst0362;
	__u8 *ptaf9003;
	__u8  *ptnxP10048;
	__u8  *ptsh1432;
	__u16 *ptsh1409;
 	#endif  
    //for ddk and normal design.
	//for I/Q conncetion config. bit2: I/Q swap. bit1: I_Diff swap. bit0: Q_Diff swap.< 0: no, 1: swap>; 
	__u8 connection_config;
	//!<bit0: IF-AGC enable <0: disable, 1: enalbe>;bit1: IF-AGC slop <0: negtive, 1: positive>
	//!<bit2: RF-AGC enable <0: disable, 1: enalbe>;bit3: RF-AGC slop <0: negtive, 1: positive>
	//!<bit4: Low-if/Zero-if.<0: Low-if, 1: Zero-if>
	//!<bit5: RF-RSSI enable <0: disable, 1: enalbe>;bit6: RF-RSSI slop <0: negtive, 1: positive>
	//!<bit8: fft_gain function <0: disable, 1: enable>
	//!<bit9: "blank channel" searching function <0: accuate mode, 1: fast mode>
	//!<bit10~11: frequency offset searching range <0: +-166, 1: +-(166*2), 2: +-(166*3), 3: +-(166*4)>
	//!<bit12: RSSI monitor <0: disable, 1: enable>

	__u16 cofdm_config; //!<For COFDM config setting

	__u8  AGC_REF;    //!<The reference value of AGC,the average amplitude to full scale of A/D. % percentage rate.
	__u8  RF_AGC_MAX; //!<Maximum value of RF_AGC, x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8  RF_AGC_MIN; //!<Minimum value of RF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8  IF_AGC_MAX; //!<Maximum value of IF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	__u8  IF_AGC_MIN; //!<Minimum value of IF_AGC,x.y V to xy value, 5.0v to 50v(3.3v to 33v)Qam then use it to configue register.
	
	__u32 i2c_type_sel; //!<For I2C select
	__u32 i2c_type_sel_1;//!<For I2C_SUPPORT_MUTI_DEMOD	
	
	__u8  demod_chip_addr;  //!<For I2C addr
	__u8  demod_chip_addr1; //!<For I2C addr
	__u8  demod_chip_ver;   //!<For I2C chip versions
	__u8  tuner_id;         //!<For Tuner ID
	__u8  c_tuner_tsi_setting_0;//!<For Tuner setting
	__u8  c_tuner_tsi_setting_1;//!<For Tuner setting

	__u32 flag;
    __u8 *memory;
    __u32 memory_size;
	
};

/*!@struct COFDM_TUNER_CONFIG_API
@brief The structure passes the argument of tuner config data to driver from board config (for COFDM).
*/
typedef struct COFDM_TUNER_CONFIG_API
{	
	
	int (*nim_tuner_init) (__u32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrtuner_config);
	int (*nim_tuner_control) (__u32 tuner_id,__u32 freq, __u8 bandwidth,__u8 agc_time_const,__u8 *data,__u8 cmd_type);
	int (*nim_tuner_status) (__u32 tuner_id,__u8 *lock);
	int (*nim_tuner_close) (void);
    union
    {
	    int (*nim_tuner_cal_agc) (__u32 tuner_id, __u8 flag, __u16 rf_val, __u16 if_val, __u8 *data);
        int (*nim_tuner_command)(__u32 tuner_id, int cmd, __u32 param);
    };
    void (*nim_lock_cb) (__u8 lock);
	struct COFDM_TUNER_CONFIG_DATA config_data;	
	struct COFDM_TUNER_CONFIG_EXT tuner_config;
	struct EXT_DM_CONFIG ext_dm_config;

    __u32 tuner_type;
    __u32 rev_id           : 8;
    __u32 config_mode      : 1;
    __u32 work_mode        : 1;    // NIM_COFDM_SOC_MODE or NIM_COFDM_ONLY_MODE
    __u32 ts_mode          : 2;    // enum nim_cofdm_ts_mode, only for NIM_COFDM_ONLY_MODE
    __u32 reserved         : 20;

	__u32 demod_index;                           //!<Multi demodulator control,the index of demodulator
	__u32 tuner_id;                              //!<The identifier of tuner
	
}*PCOFDM_TUNER_CONFIG_API;






#endif


/*!@struct ali_nim_m3501_cfg
@brief The structure passes the argument of m3501 to driver from board configuration. 
*/
struct ali_nim_m3501_cfg
{
	__u16 recv_freq_low;      //!<Low value of Tuner Receiving Frequency range
	__u16 recv_freq_high;     //!<High value of Tuner Receiving Frequency range 
	
	__u16 qpsk_config;   //!< QPSK configuration. The meaning of each bit as follows:
                       //!<bit0:QPSK_FREQ_OFFSET,bit1:EXT_ADC,bit2:IQ_AD_SWAP,bit3:I2C_THROUGH,bit4:polar revert bit5:NEW_AGC1,bit6bit7:QPSK bitmode:
	                     //!<00:1bit,01:2bit,10:4bit,11:8bit
	__u16 reserved;     

	__u32 tuner_i2c_id;     //!<The identifier of I2C used by tuner
	__u32 tuner_i2c_addr;   //!<The address of I2C used by tuner
	__u32 demod_i2c_id;      //!<The identifier of I2C used by demodulator
	__u32 demod_i2c_addr;    //!<The address of I2C used by demodulator 
	__u32 tuner_id;         //!<The identifier of tuner

	int disqec_polar_position;//!<The gpio number for control disqec polar

	__u32 lnb_i2c_id;     //!<The identifier of I2C used by lnb
	__u32 lnb_i2c_addr;   //!<The address of I2C used by lnb
	__u32 lnb_name;       //!<The name of lnb ;0:no lnb,1:LNB_A8304,2:LNB_TPS65233
};



/*!@struct ali_nim_mn88436_cfg
@brief The structure passes the argument of mn88436 to driver from board configuration.
*/
struct ali_nim_mn88436_cfg
{
	struct COFDM_TUNER_CONFIG_DATA cofdm_data;    //!<The configuration of tuner
	struct COFDM_TUNER_CONFIG_EXT tuner_config;   //!<The extension configuration of tuner
	struct EXT_DM_CONFIG ext_dm_config;           //!<External demodulator configuration parameter
	__u32 tuner_id;                               //!<The identifier of tuner 
};


/*!@struct ali_nim_mn88436_cfg
@brief The structure passes the argument of M3200 to driver from board configuration.
*/
struct ali_nim_m3200_cfg
{
	struct QAM_TUNER_CONFIG_DATA tuner_config_data;  //!<The configuration of tuner
 	struct QAM_TUNER_CONFIG_EXT  tuner_config_ext;   //!<The extension configuration of tuner
	struct EXT_DM_CONFIG         ext_dem_config;     //!<External demodulator configuration parameter
    __u8 qam_mode;                                 //!<The QAM mode  
    __u32 tuner_id;                                 //!<The identifier of tuner.                                                 
};

/*!@struct ali_nim_mn88436_cfg
@brief The structure passes the argument of M3200 to driver from board configuration.
*/
struct ali_nim_mxl241_cfg
{
	struct QAM_TUNER_CONFIG_DATA tuner_config_data;  //!<The configuration of tuner
	struct QAM_TUNER_CONFIG_EXT tuner_config_ext;    //!<The extension configuration of tuner
};

struct nim_dvbs_data_cap
{
    __u32 dram_base_addr;	// Mem address for demod hw used
    __u32 cap_len;         // Men len
    __u8 cap_src;          // Data source which need captured
};

/*!@struct ali_nim_agc
@brief The structure passes the argument to tuner driver and get tuner's rf_level
*/
struct ali_nim_agc
{
	__u16 demod_agc;	//the agc of demod, transfer to tuner driver, some tuner mayn't use this param
	char rf_level;    //the rf_level get from tuner driver, unit is dbm
};

// For verification ACM mode
struct nim_dvbs_channel_info
{
    __u32 symbol_rate;
    __s32 freq_offset;
    __u8 work_mode;    // 1=dvbs2, 0=dvbs
    __u8 sub_work_mode; 
    __u8 code_rate;
    __u8 roll_off;
    __u8 pilot_mode;
    __u8 modcod;
    __u8 iqswap_flag;
    __u8 map_type;
    __u8 frame_type; 
    __s8 signal_level;
    __s16 snr;
    __u32 per;
	__u32 ber;
    __s32 phase_err;
	__u32 period_per;  // For per monitor
    __u8 phase_err_stat_cnt;
    __u8 fake_lock_dete_thr;
    __u8 fake_lock_rst_cnt;
};

struct nim_dvbs_bb_header_info
{
    // BBHEADER 10Byte
    // [MATYPE(2)]  [UPL(2)]  [DFL(2)]  [SYNC(1)]  [SYNCD(2)]  [CRC-8(1)] 
    __u8 source_data[10];    // Source bb header data from reg105

    // Decompose of BBHEADER
	__u8 stream_mode;  // 11=ts/00~01=gs
	__u8 stream_type;  // 1=signle/0=multi
	__u8 acm_ccm;      // 0=acm/1=ccm
	__u8 issyi;        // 1=active/0=no active
	__u8 npd;          // 1=active/0=no active
	__u8 roll_off;     //
	__u8 isi;          // Valid when stream_type == NIM_STREAM_MULTIPLE
	__u16 upl;         // 0~65535, 0 = continu stream, 188x8d = MPEG ts 
	__u16 dfl;         // 0 ~ 58112
	__u8 sync;         //
	__u16 syncd;       //
	__u8 crc_8;        //
};

struct nim_tso_clk_cfg
{
    __u8 is_manual_mode;   //: 0 = auto mode(HW calculate it automatic), 1 = manual mode
    __u8 tso_clk_src;      //: tso clk source select(Only calid when is_manual_mode = 1)
    __u8 clk_div_bypass;   //: tso clk div bypass, 0 = don't bypass, 1 = bypass (Only calid when is_manual_mode = 1)
    __u8 clk_div;          //: tso clk div select(Only calid when is_manual_mode = 1 and clk_div_bypass = 0)
};


struct nim_tso_cfg
{
    struct nim_tso_clk_cfg tso_clk_cfg;
    __u8 tso_switch;
    __u8 tso_dummy_switch;
    __u8 tso_clk_phase;
    __u8 tso_clk_pad_driving;
    __u8 tso_data_pad_driving;
};

struct nim_dvbs_isid
{
	__u8 get_finish_flag;	//0:not finished, 1:finished	
    __u8 isid_overflow;    // Unused
    __u16 isid_num;         // Isid mumber, 0 means it not a multistream TP, max 256
    __u8 isid_read[256];   // Read  isid from hardware
    __u16 isid_read_timeout;   // Read  isid timeout, unit is ms

    __u8 isid_bypass;       // 1 = Bypass isid filter  0= default
    __u8 isid_mode;         // 1 = use software config, 0 = auto select first id
    __u8 isid_write;       // Write isid from sw to hw , valid while isi_mode is 1 and isi_bypass is 0
    __u8 debug_set_isid;	// Unused, for test 
};

typedef enum TSN_OPERATE_TYPE {
	TSN_SET_TP_ONLY,		//set tp only
	TSN_SET_TP_AND_TSN,		//set tp and tsn
	TSN_SET_TP_AND_GET_TSN,	//set tp and get tsn
} tsn_operate_type_t;

struct nim_dvbs_tsn
{
	__u8 is_wideband;				//0:not wideband, 1:wideband
	__u8 get_finish_flag;	        // 0:not finished, 1:finished	
    __u16 tsn_num;                 // TSN mumber, max 256
    __u8 tsn_read[256];            // TSN read from hardware
    __u16 tsn_read_timeout;        // TSN read timeout, unit is ms

    __u8 tsn_enable;               // 1 = Enable tsn filter  0 = disable
    __u8 tsn_write;                // Write tsn from sw to hw , valid while tsn_en is 1
    tsn_operate_type_t tsn_operate_tpye;	
};

typedef enum PLSN_STATE//added by robin in 20170228
{
	PLSN_RUNNING,	//the task is running
	PLSN_FIND,		//the value has found
	PLSN_OVER,		//all value is tried
	PLSN_STOPED,		//the task is stoped
	PLSN_UNSUPPORT,	//not support super scan
}plsn_state;

typedef struct
{
	__u8  super_scan; 		//set by upper level,  control whether do super scan, 1:super scan, 0:nomal scan
	__u8  start_search;	//set by driver,  if the TP is config, set start_search to 1, super_scan and start_search control whether start plsn search
	__u8  auto_scan_start;
	// For 3501h
	__u8  plsn_algo_preset;	// from upper
	__u8  search_algo;		// current search used
	__u8  search_lock_state;	// 0:haven't lock, 1: pl lock,  2: cr lock
	__u8  plsn_hw_find;	// New for 3501h
	__u8  plsn_hw_end;		// New for 3501h
	__u8  plsn_hw_en;
	__u8  plsn_same_cnt;
	__u32 plsn_same_value;
	__u8  plsn_patch_flag;  // For spec plsn value:262141 in BBH algo 20180106 paladin
	// End
	__u8  search_plsn_force_stop;
	__u8  search_plsn_stop;
	__u8  plsn_find;	
	__u32 plsn_num;		//how many plsn has found
	__u32 plsn_val[64];
	//__u8  plsn_update; // to the upper
	__u32 plsn_report; // to the upper
	__u32 plsn_now;  // from the upper
	__u8  plsn_try;
	__u32 plsn_try_val;
	
	__s32  plsn_search_algo;//0:pilot on algorithm, 4:bbh algorithm	
	__u8  plsn_finish_flag;
	__u8  plsn_pls_lock; //added for reducing cost time of autoscan when searching plsn is enable
	__u32 plsn_pls_unlock_cnt; //added for reducing cost time of autoscan when searching plsn is enable
	__u8  plsn_pls_timeout; //added for reducing cost time of autoscan when searching plsn is enable
	__u8  generate_table_happened_flag; //in order to  control the relese table must happen after generate table happen

}PLS;

struct ali_plsn_address
{
	__u32 table_address;
	__u32 capture_address;
};

/*!@struct NIM_CHANNEL_CHANGE
@brief Structure for Channel Change parameters.
*/
struct NIM_CHANNEL_CHANGE
{
	__u32 freq;				    //!< Channel Center Frequency: in MHz unit 
	__u32 sym;				    //!< Channel Symbol Rate: in KHz unit
	__u8 fec;				      //!< Channel FEC rate 
	__u32 bandwidth;		  //!< Channel Symbol Rate: same as Channel Symbol Rate ? -- for DVB-T
	__u8 guard_interval;	//!< Guard Interval -- for DVB-T 
	__u8 fft_mode;			  //!< For DVB-T 
	__u8 modulation;		  //!< For DVB-T 
	__u8 usage_type;		  //!< For DVB-T 
	__u8 inverse;			    //!< For DVB-T

    __u8 priority;         //!< For DVB-T 

    __u8 t2_signal;        //!<0:DVB-T signal, 1:DVB-T2 signal.
    __u8 plp_num;          //!<Total number of PLP within this channel.
    __u8 plp_index;        //!<Current selected data PLP index.

    __u8  plp_id;          //!<plp_id of plp_index.
    __u16 t2_system_id;    //!<t2_system_id of this channel.

    //!<T2_delivery_system_descriptor: transport_stream_id that identified by t2_system_id/plp_id paire.
    __u16 t_s_id;
    //!<T2_delivery_system_descriptor: original_network_id that identified by t2_system_id/plp_id paire.
    __u16 network_id;	
	//!<0:SONY_DVBT2_PROFILE_BASE, 1:SONY_DVBT2_PROFILE_LITE, 2:SONY_DVBT2_PROFILE_ANY
	__u8 t2_profile;		
	__u8  t2_signal_mixed_type; 	//!< 01:T2-base, 10:T2-lite, 11:T2-base + T2-lite

    // Only For DVB-S2 multisream
    __u8 change_type;  			//!< 0 = set TP only, 1 = set TP and ISID, 2 = set ISID only, 3 = set TP and get ISID information
    struct nim_dvbs_isid *isid;

	struct nim_dvbs_tsn *tsn;
  	
	__u8 	super_scan;	 //whether support super scan which will search plsn, 0:not support, 1:support
	__u8	search_algo_preset;
    __u32 	plsn_db;	//the plsn value get from database

	__u8 freq_band;    // For dvb-s/s2, 0 = default, 1 = C band, 2 = Ku band
};



/*!@struct NIM_AUTO_SCAN
@brief The structure passes the argument of AutoScan data to driver from application.
*/
struct NIM_AUTO_SCAN
{
	__u8 unicable;      //!<Whether to use single cable to transmit data
	__u16 fub;	    		//!<Unicable: UB slots centre freq (MHz)
	__u32 sfreq;			  //!<Start Frequency of the Scan procedure: in MHz unit.
	__u32 efreq;			  //!<End Frequency of the Scan procedure: in MHz unit.
    __u8 isid_num;     // 0 = Not a multiply stream, other = multiply stream
    __u8 isid;         // Current isid for auto scan
    __u8 scan_type;    // 0: normal scan(single stream scan); 1: multistream scan
	__u32 (*callback)(void *pfun,__u8 status, __u8 polar,
	       __u16 freq, __u32 sym, __u8 fec,__u8 stop);	
	                   //!<Callback pointer. The callback function calls back the frequency of data which AutoScan procedure search.
	struct CHANNEL_SPECTRUM
	{		
		int *p_channel_spectrum;
		int spectrum_length;
	}spectrum;
	
	struct VALID_FREQ_SYM
	{
		int *p_frequency;
		int *p_symbol_rate;
		int real_freq_sym;
	}valid_freq_sym;

	__u8 	super_scan;	 //whether support super scan which will search plsn, 0:not support, 1:support
    __u32 	plsn_db;	//autoscan will set this value when the tp lock success, the uplevel can get to save

	__u8 freq_band;    // For dvb-s/s2, 0 = default, 1 = C band, 2 = Ku band
};


/*!@struct ali_nim_diseqc_cmd
@brief The structure passes the argument of AutoScan data to driver from application.
*/
struct ali_nim_diseqc_cmd
{
	__u32 mode; 						            //!<Input: Diseqc command mode
	__u8 cmd[DISEQC_CMD_MAX_LEN];		    //!<Input: Diseqc command bytes
	__u8 cmd_size;						          //!<Input: Diseqc command length
	__u8 ret_bytes[DISEQC_CMD_MAX_LEN]; //!<Output: Diseqc command return bytes
	__u8 ret_len; 						          //!<Output: Diseqc command return bytes length
	__u16 diseqc_type;					        //!<1: Diseqc 1x. 2: Diseqc 2X
};


/*!@struct ali_lnb_freqs
@brief The structure passes the argument of LNB frequecy.
*/
struct ali_lnb_freqs
{
	__u32 freq_hi_khz;    //!<High band frequency in kHz 
	__u32 freq_lo_khz;     //!<Low band frequency in kHz
	__u32 freq_switch_khz;  //!<Switch frequency in kHz
};
/*@brief
Structure for get spectrum parameters 
*/
struct nim_get_symbol
{  
	__u32 point_length;
	__u8 *p_cr_out_i;			
	__u8 *p_cr_out_q;	
};

enum NIM_CHANNEL_CHANGE_TYPE
{
	NIM_CHANGE_SET_TP_ONLY = 0,
	NIM_CHANGE_SET_TP_AND_ISID = 1,
	NIM_CHANGE_SET_ISID_ONLY = 2,
	NIM_CHANGE_SET_TP_AND_GET_ISID = 3,
};

enum NIM_TUNER_TYPE
{
	IS_M3031 = 0,
	NOT_M3031 = 1,
};

enum NIM_CODE_MODE
{
    NIM_ACM = 0,    // VCM is signalled as ACM
	NIM_CCM = 1,
};

enum NIM_STREAM_TYPE
{
    NIM_STREAM_MULTIPLE = 0,
	NIM_STREAM_SINGLE = 1,
};

enum NIM_TS_GS_MODE
{
    NIM_GS_PACKET = 0,
	NIM_GS_CONTINUE = 1,
    NIM_RESERVED = 2,
	NIM_TS = 3,	
};

enum NIM_ISSYI
{
    NIM_ISSYI_NOT_ACTIVE = 0,    
	NIM_ISSYI_ACTIVE = 1,
};

enum NIM_NPD
{
    NIM_NPD_NOT_ACTIVE = 0,    
	NIM_NPD_ACTIVE = 1,
};

enum NIM_ROLL_OFF
{
    NIM_RO_035 = 0,    
	NIM_RO_025 = 1,
    NIM_RO_020 = 2,    
	NIM_RO_RESERVED = 3,	
};

enum NIM_RET
{
    NIM_SUCCESS = 0,
    NIM_FAILED = 1,
};

struct DEMOD_GPIO_STRUCT
{
	__s16 position;
	__u8 value;
};

#define ALI_NIM_CHANNEL_CHANGE             _IOW('o', 84, struct NIM_CHANNEL_CHANGE)  //!<Change channel 
#define ALI_NIM_SET_POLAR	                 _IOW('o', 85, __u32)                      //!<Set LNB polarization of DVB-S NIM device
#define ALI_NIM_GET_LOCK_STATUS            _IO('o', 86)                              //!<Get current NIM device lock status
#define ALI_NIM_READ_QPSK_BER 	           _IO('o', 87)                              //!<Read bit error rate
#define ALI_NIM_READ_RSUB	                 _IO('o', 88)                              //!<Read reed solomon uncorrected block counter
#define ALI_NIM_READ_AGC	                 _IO('o', 89)                              //!<Get current NIM device channel AGC value
#define ALI_NIM_READ_SNR	                 _IO('o', 90)                              //!<Return an approximate estimation of the SNR from the NIM
#define ALI_NIM_READ_SYMBOL_RATE	         _IO('o', 91)                              //!<Read the symbol rate
#define ALI_NIM_READ_FREQ                  _IOWR('o', 92, __u32)                              //!<Get current frequency
#define ALI_NIM_READ_CODE_RATE	           _IO('o', 93)                              //!<Get bit error rate
#define ALI_NIM_HARDWARE_INIT_S            _IOW('o', 94, struct ali_nim_m3501_cfg)   //!<Initialize the hardware equipment(DVBS/S2)
#define ALI_NIM_HARDWARE_INIT_C            _IOW('o', 95, struct ali_nim_m3200_cfg)   //!<Initialize the hardware equipment(DVBC)
#define ALI_NIM_HARDWARE_INIT_T            _IOW('o', 96, struct ali_nim_mn88436_cfg) //!<Initialize the hardware equipment(DVBT/T2/ISDBT/ATSC)
#define ALI_NIM_AUTO_SCAN 	               _IO('o', 97)                              //!<Do AutoScan procedure(DVBS/S2)
#define ALI_NIM_DISEQC_OPERATE 	           _IOWR('o', 98, struct ali_nim_diseqc_cmd) //!<NIM DiSEqC device operation(DVBS/S2)
#define ALI_NIM_GET_RF_LEVEL               _IO('o', 99)                              //!<Get RF level
#define ALI_NIM_GET_CN_VALUE               _IO('o', 100)                             //!<Get CN value
#define ALI_NIM_SET_PERF_LEVEL             _IOW('o', 101, __u32)                     //!<Set performance level
#define ALI_NIM_REG_RW 	                   _IOWR('o', 102, __u32)                    //!<Demodulator Register Read and Write. If you want to directly manipulate the register, please use it?¡ê
#define ALI_NIM_DRIVER_READ_SUMPER         _IOWR('o', 103, __u32)                    //!<Get the total of packet error rate
#define ALI_NIM_DRIVER_SET_MODE            _IOWR('o', 104, __u32)                    //!<Set the QAM mode (j83AC/j83b)
#define ALI_NIM_REG_RW_EXT                 _IOWR('o', 105, __u32)                    //!<Extension function for registering Read and Write
#define ALI_SYS_REG_RW                     _IOWR('o', 106, __u32)                    //!<System registers Read and Write. If you want to directly manipulate the register, please use it?¡ê
#define ALI_NIM_ADC2MEM_START              _IOW('o', 107, __u32)                     //!<Start the capture mode
#define ALI_NIM_ADC2MEM_STOP               _IO('o', 108)                             //!<Stop the capture mode
#define ALI_NIM_ADC2MEM_SEEK_SET           _IOW('o', 109, __u32)                     //!<Set read position where data captured
#define ALI_NIM_ADC2MEM_READ_8K            _IOR('o', 110, __u32)                     //!<Read captured data from buffer
#define ALI_NIM_TUNER_SELT_ADAPTION_S      _IOR('o', 111, struct ali_nim_m3501_cfg)  //!<Tuner self adation(DVBS/S2)
#define ALI_NIM_TUNER_SELT_ADAPTION_C      _IOR('o', 112, struct ali_nim_m3200_cfg)  //!<Tuner self adation(DVBC)
#define ALI_NIM_TUNER_SELT_ADAPTION_T      _IOR('o', 113, struct ali_nim_mn88436_cfg)//!<Tuner self adation(DVBT/T2/ISDBT/ATSC)
#define ALI_NIM_READ_TUNTYPE    	         _IOR('o', 114, __u32)                     //!<Read the type of tuner
#define ALI_NIM_RESET_FSM                  _IO('o', 115)                             //!<Reset FSM
#define ALI_NIM_TUNER_ACTIVE               _IO('o', 116)                             //!<Active/wakeup the tuner 
#define ALI_NIM_TUNER_STANDBY              _IO('o', 117)                             //!<Set the tuner into standby status 
#define ALI_NIM_STOP_AUTOSCAN	             _IOW('o', 118, __u32)                     //!<Stop autoscan
#define ALI_NIM_DRIVER_GET_CUR_FREQ	       _IOW('o', 119, __u32)                     //!<Get the current frequency of NIM
#define ALI_NIM_DRIVER_SET_RESET_CALLBACK  _IO('o', 120)                             //!<Wake up the tuner
#define ALI_NIM_TURNER_SET_STANDBY	       _IO('o', 121)                             //!<Set the tuner into standby status
#define ALI_NIM_DRIVER_GET_ID              _IO('o', 122)                             //!<Get the chip ID   
#define ALI_NIM_AS_SYNC_WITH_LIB           _IOWR('o', 123, __u32)                    //!<Set the synchronous signal of driver with application layer, which ensure that the upper application is ready.
//#define ALI_NIM_SET_NETLINKE_ID            _IOWR('o', 124, __u32)                    //!<Set NIM register netlink port 
#define ALI_NIM_SET_LNB_POWER              _IOWR('o', 125, __u32)                    //!<Set the LNB power     
#define ALI_NIM_GET_LNB_POWER              _IO('o', 126)                             //!<Get the LNB power state
#define ALI_NIM_SET_LNB_FREQS              _IOWR('o', 127, struct ali_lnb_freqs)     //!<set the lnb freq   
#define ALI_NIM_GET_LNB_FREQS              _IOWR('o', 128,struct ali_lnb_freqs)      //!<queries the lnb frequency settings 
#define ALI_NIM_SET_LOOPTHRU               _IOWR('o', 129, __u32)                    //!<set the loop through
#define ALI_NIM_GET_LOOPTHRU               _IO('o', 130)                             //!<query the loop through settings 

#define ALI_NIM_GET_SIG_STATUS             _IOWR('o', 131, __u32)                    //!<Get struct nim_m3501_sig_status 
#define ALI_NIM_GET_QUALITY_INFO           _IOWR('o', 132, __u32)                    //!<Get struct nim_m3501_quality_info
#define ALI_NIM_DRIVER_GET_DEMOD_LOCK_MODE _IOWR('o', 133, __u32)                    //!<
#define ALI_NIM_DRIVER_GET_FFT_MODE        _IOWR('o', 134, __u32)                    //!<
#define ALI_NIM_DRIVER_GET_MODULATION      _IOWR('o', 135, __u32)                    //!<
#define ALI_NIM_DRIVER_STOP_ATUOSCAN       _IOWR('o', 136, __u32)                    //!<

#define ALI_NIM_LOG_LEVEL                  _IOW('o', 137, __u32)                     //!<Set print log level
#define ALI_NIM_DRIVER_T2_SIGNAL_ONLY      _IOW('o', 138, __u32)                     //!<Set T2 signal flag
#define ALI_NIM_GET_GUARD_INTERVAL         _IOWR('o', 139, __u32)                    //!<
#define ALI_NIM_GET_SPECTRUM_INV           _IOWR('o', 140, __u32)                    //!<
#define ALI_NIM_TURNER_SET_LT              _IOWR('o', 141, __u32)                    //!<only control tuner Loopthrough function
#define ALI_NIM_DRIVER_SET_DEMOD_RESET     _IOWR('o', 142, __u32)                    //!<set demod restart

#define ALI_NIM_GET_KUMSGQ				   _IOR('o', 143, __u32)
#define ALI_NIM_GET_MER			           _IOR('o', 144, __u32)

#define ALI_NIM_ACM_MODE_GET_TP_INFO		_IO('o', 145)
#define ALI_NIM_ACM_MODE_SET_ISID			_IOWR('o', 146, struct nim_dvbs_isid)
#define ALI_NIM_ACM_MODE_GET_ISID			_IOWR('o', 147, struct nim_dvbs_isid)
#define ALI_NIM_DRIVER_CAP_DATA				_IOWR('o', 148, struct nim_dvbs_data_cap)
#define ALI_NIM_DRIVER_SET_PLS				_IOWR('o', 149, PLS)
#define ALI_NIM_DRIVER_GET_SYMBOL			_IOWR('o', 150, struct nim_get_symbol)
#define ALI_NIM_GET_LNB_OCP					_IOR('o', 151, __u32)
#define ALI_NIM_SET_LNB_POWER_ONOFF			_IOW('o', 152, __u32)
#define ALI_NIM_DRIVER_SYM_LIMIT_RANGE		_IOW('o', 153, __u32)
#define	ALI_NIM_DRIVER_GET_TUNER_STA		_IOR('o', 154, __u32)
#define ALI_NIM_LNB_CURRENT_LIMIT_CONTROL	_IOW('o', 155, __u32)

#define ALI_NIM_DRIVER_SET_PLSN				_IOW('o', 156, __u32)
#define ALI_NIM_DRIVER_GET_PLSN				_IOW('o', 157, __u32)
#define ALI_NIM_DRIVER_PLSN_GOLD_TO_ROOT	_IOW('o', 158, __u32)
#define ALI_NIM_DRIVER_PLSN_ROOT_TO_GOLD	_IOW('o', 159, __u32)
#define ALI_NIM_DRIVER_GENERATE_TABLE		_IOW('o', 160, struct ali_plsn_address)
#define ALI_NIM_DRIVER_RELEASE_TABLE		_IOW('o', 161, __u32)
#define ALI_NIM_DRIVER_GET_PLSN_FINISH_FLAG	_IOW('o', 162, __u8)
#define ALI_NIM_GET_WORK_MODE		   	   	_IOR('o', 163, __u8)
#define ALI_NIM_DRIVER_SET_TSN				_IOW('o', 164, struct nim_dvbs_tsn)
#define ALI_NIM_DRIVER_GET_TSN				_IOR('o', 165, struct nim_dvbs_tsn)
#define ALI_NIM_DRIVER_GET_WIDEBAND_MODE	_IOR('o', 166, __u8)
#define ALI_NIM_DRIVER_GET_TSN_FINISH_FLAG	_IOR('o', 167, __u8)
#define ALI_NIM_DRIVER_GET_FAST_LOCK_DETECT _IOR('o', 168, __u8)
#define ALI_NIM_DRIVER_GPIO_OUTPUT			_IOR('o', 169, struct DEMOD_GPIO_STRUCT)
#define ALI_NIM_DRIVER_ENABLE_GPIO_INPUT	_IOR('o', 170, struct DEMOD_GPIO_STRUCT)
#define ALI_NIM_DRIVER_GET_GPIO_INPUT		_IOR('o', 171, struct DEMOD_GPIO_STRUCT)
#define ALI_NIM_READ_PRE_BER 	        _IO('o', 172)                              //!<Read pre bit error rate

/*!
 * @}
 */

/*!
 * @}
 */

#endif
