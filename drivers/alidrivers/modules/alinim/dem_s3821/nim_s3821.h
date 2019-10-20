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
 
/*
*    File:  nim_s3821.h
*
*    Description:  s3821 header
*    History:
*           Date            Athor        Version          Reason
*        ============    =============    =========    =================
*    1.    6.14.2013        Joey.Gao         Ver 1.0       Create file.
*
*/

#ifndef __LLD_NIM_S3821_H__
#define __LLD_NIM_S3821_H__

#if defined(__NIM_LINUX_PLATFORM__)
#include "porting_s3821_linux.h"
#elif defined(__NIM_TDS_PLATFORM__)
#include "porting_s3821_tds.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif



#if 0
#define S3821_PRINTF     nim_print_x
#else
#define S3821_PRINTF(...)
#endif





//#define ISDBT_SFU_TEST

//for TPS auto-trace function

#define S3821_AUTO_TRACE_ENABLE    1
#define RET_CONTINUE    0xFF


//for SOC OR COFDM_ONLY

//#define S3821_COFDM_ONLY   0

//#define S3821_COFDM_SOC    1



// #define S3821_COFDM_SYS_CLOCK_KHZ    28636     // move the define to nim_s3821_clk.h.

//#define S3821_COFDM_WORK_MODE           S3811_COFDM_SOC    //S3821_COFDM_ONLY

// define the current demo type
#define    ISDBT_TYPE        0
#define    DVBT_TYPE        1
#define    DVBT2_TYPE        2
#define    DVBT2_COMBO        3


// enable to print the BER & PER in main thread
#define CHIP_VERIFY_FUNC        SYS_FUNC_OFF     //SYS_FUNC_ON SYS_FUNC_OFF
#define    NIM_3821_FLAG_ENABLE    0x00000100
#define NIM_3821_SCAN_END       0x00000001



#define    NIM_S3821_ADC2DMA_START_ADDR    (__MM_EPG_BUFFER_START)
//#define    NIM_S3821_ADC2DMA_MEM_LEN    (0xA6000000 - __MM_EPG_BUFFER_START)
#define	NIM_S3821_ADC2DMA_MEM_LEN	(__MM_FB_TOP_ADDR - __MM_EPG_BUFFER_START)
//#define	NIM_S3821_ADC2DMA_START_ADDR	(0xA3530000)
//#define	NIM_S3821_ADC2DMA_MEM_LEN	(0xA6000000 - 0xA3530000)



#define S3821_COFDM_SOC_BASE_ADDR      0xB804C000
#define S3821_COFDM_ONLY_I2C_BASE_ADDR      0x40//SYS_COFDM_S3821_CHIP_ADRRESS


#define NIM_S3821_GET_DWORD(i)           (*(volatile UINT32 *)(i))
#define NIM_S3821_SET_DWORD(i,d)        (*(volatile UINT32 *)(i)) = (d)
#define NIM_S3821_GET_WORD(i)             (*(volatile UINT16 *)(i))
#define NIM_S3821_SET_WORD(i,d)          (*(volatile UINT16 *)(i)) = (d)
#define NIM_S3821_GET_BYTE(i)             (*(volatile UINT8 *)(i))
#define NIM_S3821_SET_BYTE(i,d)          (*(volatile UINT8 *)(i)) = (d)


#define TPS_CONST_DQPSK        0x02
#define TPS_CONST_QPSK        0x04
#define TPS_CONST_16QAM        0x10
#define TPS_CONST_64QAM        0x40


#define FEC_1_2   0
#define FEC_2_3   1
#define FEC_3_4   2
#define FEC_5_6   3
#define FEC_7_8   4

#define GUARD_1_32 0x20
#define GUARD_1_16 0x10
#define GUARD_1_8  0x08
#define GUARD_1_4  0x04
#define MODE_2K    0x02
#define MODE_8K 0x08
#define MODE_4K    0x04
#define BW_6M    6
#define BW_7M    7
#define BW_8M    8
#define BW_10M   10



//usage_type

#define MODE_AUTOSCAN        0x00
#define MODE_CHANSCAN        0x01
#define MODE_CHANCHG        0x02
#define MODE_AERIALTUNE    0x03


// 2007-09-20 for aonvision special scan mode. exist channel scan and non-exist channel.

#define MODE_ACCURATESCAN        0x04    //try several time to ensure the accuracy. such as "3".
#define MODE_FASTSCAN        0x05    // only try 1 time


//Joey-20070524: for Mico channel scan/change optimization. every time try freq-offset for the best play point.

#define MODE_CHANPLAY        0x10
#define PARAM_UNKNOWN        0x00
#define PARAM_KNOWN        0x01

//IQ swap
#define IQ_NORMAL    0x00
#define IQ_SWAP        0x01



#define S3821_I2C_BYPASS_EN    0x01
#define s3821_cofdm_reset    0x80
#define s3821_cap_start        0x40
#define S3821_ZERO_IF        0x40


#define S3821_GUARD_1_32    0 <<1
#define S3821_GUARD_1_16    1 <<1
#define S3821_GUARD_1_8        2 <<1
#define S3821_GUARD_1_4        3 <<1
#define S3821_GUARD_KNOWN    1


#define S3821_MODE_2K        0 <<5
#define S3821_MODE_8K        1 <<5
#define S3821_MODE_KNOWN    1 <<4


#define S3821_IQ_NONSWAP    0 <<7
#define S3821_IQ_SWAP        1 <<7
#define S3821_IQSWAP_KNOWN    1 <<6


#define S3821_BW_6M    1
#define S3821_BW_7M    2
#define S3821_BW_8M    3

#define S3821_COFDM_STATUS_LOCK    1 <<7
#define S3821_INT_UNLOCK    1 <<6
#define S3821_INT_TIMEOUT    1 <<7


#define LOCK_OK           11
#define SCAN_UNLOCK         12
#define SCAN_TIMEOUT        13
#define TPS_UNLOCK            19


//hierachy mode setting.
#define LPSEL 0x02

#define HIER_NONE 0
#define HIER_1  1

#define HIER_2  2
#define HIER_4  4

//joey, 20130627. for DVBT2-COMBO work mode check.
#define MODE_DVBT2  0x04
#define MODE_ISDBT  0x02
#define MODE_DVBT    0x01



// type definition for string output function pointer.


INT32 nim_s3821_dvbt_isdbt_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para);
INT32 nim_s3821_dvbt2_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para);
INT32 nim_s3821_combo_channel_change(struct nim_device *dev, NIM_CHANNEL_CHANGE_T *change_para);

INT32 nim_s3821_hw_init(struct nim_device *dev);
INT32 nim_s3821_tuner_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param);

INT32 nim_s3821_init_config(struct nim_device *dev);
INT32 nim_s3821_read(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len);
INT32 nim_s3821_write(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len);
void nim_s3821_dvbt_isdbt_proc(struct nim_device *dev,PNIM_S3821_PRIVATE priv,UINT8 *unlock_cnt);
INT32 nim_s3821_disable(struct nim_device *dev);
INT32 nim_s3821_get_code_rate(struct nim_device *dev, UINT8 *code_rate);
INT32 nim_s3821_get_freq(struct nim_device *dev, UINT32 *freq);
INT32 nim_s3821_get_gi(struct nim_device *dev, UINT8 *guard_interval);
INT32 nim_s3821_get_fftmode(struct nim_device *dev, UINT8 *fft_mode);
INT32 nim_s3821_get_modulation(struct nim_device *dev, UINT8 *modulation);
INT32 nim_s3821_get_specinv(struct nim_device *dev, UINT8 *inv);
INT32 nim_s3821_get_freq_offset(struct nim_device *dev, INT32 *freq_offset);
INT32 nim_s3821_get_hier_mode(struct nim_device *dev, UINT8 *hier);
INT32 nim_s3821_get_agc(struct nim_device *dev, UINT8 *agc);
INT32 nim_s3821_read_external_cofdm_mode(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len);
INT32 nim_s3821_write_external_cofdm_mode(struct nim_device *dev, UINT16 reg_add, UINT8 *data, UINT8 len);
INT32 nim_s3821_get_lock(struct nim_device *dev, UINT8 *lock);
INT32 nim_s3821_get_dvbt2_lock(struct nim_device *dev, UINT8 *lock);
INT32 nim_s3821_dvbt2_getinfo(struct nim_device *dev, UINT8 *guard_interval,
              UINT8 *fft_mode, UINT8 *modulation, UINT8 *code_rate);
INT32 nim_s3821_init_adc2dma(struct nim_device *dev);
INT32 nim_s3821_init_dvbt_isdbt(struct nim_device *dev);
INT32 nim_s3821_init_dvbt2(struct nim_device *dev);
void nim_s3821_main_thread(UINT32 param1, UINT32 param2);
INT32 nim_s3821_tf_int_proc(struct nim_device *dev);
INT32 nim_s3821_idle_reset_proc(struct nim_device *dev);
INT32 nim_s3821_ldpc_proc(struct nim_device *dev);
INT32 nim_s3821_adc2dma_func_start(struct nim_device *dev, UINT32 type);
INT32 nim_s3821_adc2dma_func_stop(struct nim_device *dev, UINT32 type);
INT32 nim_s3821_get_cur_mode(struct nim_device *dev, UINT8 *mode);
INT32 nim_s3821_set_cur_mode(struct nim_device *dev, UINT8 *mode);
INT32 nim_s3821_dvbt_isdbt_monitor_ber(struct nim_device *dev, UINT8 *ber_vld, UINT32 *m_vbber, UINT32 *m_per);
INT32 nim_s3821_dvbt2_monitor_cnr(struct nim_device *dev, UINT32 *m_cnr);
INT32 nim_s3821_monitor_ber(struct nim_device *dev, UINT8 *ber_vld, UINT32 *m_vbber, UINT32 *m_per);
INT32 nim_s3821_get_ber(struct nim_device *dev, UINT32 *vbber);
void nim_s3821_proc_test_mux(UINT8 tmp_dat);
INT32 nim_s3821_get_snr(struct nim_device *dev, UINT8 *snr);
INT32 nim_s3821_get_osd_int_freqoffset(struct nim_device *dev, INT32 *intf_offset);

//joey, 20130927, for T2 FFT_GAIN jump cause burst mosaic issue.
INT32 nim_s3821_cci_rm_proc(struct nim_device *dev);
//joey, 20140123, for T2 without common PLP TS, DJB overflow issue.
INT32 nim_s3821_djb_busy_proc(struct nim_device *dev);

//joey, 20140220, for T2 C3821 version.
INT32 nim_s3821_check_rev_id(struct nim_device *dev, UINT8 *rev_id);

INT32 nim_s3821_dvbt_isdbt_monitor_cnr(struct nim_device *dev, UINT32 *m_cnr);

//joey, 20140804, for T2 moscow dynamic small cci proc.
INT32 nim_s3821_dvbt2_dyn_cci_proc(struct nim_device *dev, UINT8 cci_per_vld, UINT32 cci_m_per);

//joey, 20150521, for chan_type selection processing. 
INT32 nim_s3821_chan_type_proc(struct nim_device *dev);

extern UINT32     osal_get_tick(void);

extern struct dmx_device *gpdmx;
extern BOOL mon_ifft_en;
extern UINT8 mon_status_print;
extern UINT8 cci_info_print;
extern UINT8 dem_print;

extern UINT32 g_var_adc2dma_addr;
extern UINT32 g_var_adc2dma_len; // to avoid the buffer excess the bound limit.




#ifdef __cplusplus
}
#endif
#endif    /* __LLD_NIM_S3821_H__ */







