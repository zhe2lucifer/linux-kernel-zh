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

#ifndef _ALI_DMX_SEE_INTERFACE_H_
#define _ALI_DMX_SEE_INTERFACE_H_

#include <ali_basic_common.h>
#include "dmx_stack.h"

#if 1
#define DMX_SEE_DEBUG(...)  
#else
#define DMX_SEE_DEBUG printk
#endif

#define DMX_SEE_DEV_MAX 2
#define DMX_SEE_CH_CNT 8



/* Must keep compatible with TDS macro: #define TS_BLOCK_SIZE 0xbc000. */
#define DMX_SEE_BUF_SIZE  0xBC000       



/* Must keep compatible with TDS enum: enum DEMUX_STATE. */
enum DEMUX_STATE{
    DEMUX_FIND_START = 0,
    DEMUX_HEADER,
    DEMUX_DATA,
    DEMUX_SKIP 
};



/* Must keep compatible with TDS enum: enum STREAM_TYPE. */
enum STREAM_TYPE
{
    UNKNOW_STR = 0,
    PRG_STR_MAP,
    PRIV_STR_1,
    PAD_STR,
    PRIV_STR_2,
    AUDIO_STR,
    VIDEO_STR,
    ECM_STR,
    EMM_STR,
    DSM_CC_STR,
    ISO_13522_STR,
    H2221_A_STR,
    H2221_B_STR,
    H2221_C_STR,
    H2221_D_STR,
    H2221_E_STR,
    ANCILLARY_STR,
    REV_DATA_STR,
    PRG_STR_DIR,
    
    /* Added for DCII subtitle.
     * Date:2014.07.26
	*/
	DC2SUB_STR,    
};



/* Must keep compatible with TDS struct: struct pes_pkt_param. */
struct pes_pkt_param{
    __u8 stream_id;
    __u16 pes_pkt_len;
        /*
       DMX_UINT8 orig_or_copy:1;
       DMX_UINT8 copyright:1;
       DMX_UINT8 data_align_indi:1;
       DMX_UINT8 pes_priority:1;
       */
    __u8 filled_byte_3:4;
    __u8 pes_scramb_ctrl:2;
    __u8 marker_10:2;
        /*
       DMX_UINT8 pes_ext_flag:1;
       DMX_UINT8 pes_crc_flag:1;
       DMX_UINT8 additional_copy_info_flag:1;
       DMX_UINT8 dsm_trick_md_flag:1;
       DMX_UINT8 es_rate_flag:1;
       DMX_UINT8 escr_flag:1;
        */
    __u8 filled_byte_4:6;
    __u8 pts_dts_flags:2;

    __u8   pes_header_data_len;
    __u16 pes_data_len;
        /*
       DMX_UINT8 marker_bit_0:1;
       DMX_UINT8 pts_32_30:3;
       DMX_UINT8 prefix_4_bits:4;

       DMX_UINT16 marker_bit_1:1;
       DMX_UINT16 pts_29_15:15;

       DMX_UINT16 marker_bit_2:1;
       DMX_UINT16 pts_14_0:15;
        */
        // DMX_UINT32  total_byte_len;
    enum DEMUX_STATE dmx_state ;
    enum STREAM_TYPE stream_type;
    __u32 head_buf_pos;
    void * av_buf ;
    __u32  av_buf_pos ;
    __u32  av_buf_len ;
    struct control_block * ctrl_blk;
    void * device;
    __s32 (* request_write)(void *, __u32, void **, __u32 *, struct control_block *);
    void (* update_write)(void *, __u32 );


    __u8   get_pkt_len:1;
    __u8   get_header_data_len:1;
    __u8   get_pts:1;
    __u8   str_confirm:1;
    __u8   reserved:4;

    __u8   conti_conter;
    __u8    *head_buf;

    __u8 ch_num;
    struct dmx_channel *channel;

//cw 
  __u8      cw_parity_num; /*for 3601, (actual index+1)
                            1~8:  a ecw/ocw parity number,
                            0: no cw parity set for this channel*/
                            
//sgdma 
    __u8        xfer_es_by_dma;//transfer it by dma or not
    __u8        dma_ch_num;//dma channel of this stream
    __u32   last_dma_xfer_id;//latest dma transfer id
    
//statistic
    __u8        ts_err_code; //last err code of this channel
    __u32   ovlp_cnt;   //ovlp INT cnt
    __u32   discont_cnt;    
    __u32 LastTsAdaCtrl;
	__u32   unlock_cnt;

	__u32 new_vbv_method_enable;
	__s32 (*new_vbv_request)(__u32 uSizeRequested, void ** ppVData, __u32 * puSizeGot, struct control_block * ctrl_blk);
	void (*new_vbv_update)(__u32 uDataSize);	
};


/* For C3921.
*/
#if 0
/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 * struct SEE_PARSE_INFO
 * {
 *     DMX_UINT32 rd ;
 *     DMX_UINT32 wt ;
 *     DMX_UINT32 dmx_ts_blk ;
 *     ID mutex;
 * };
 */
struct dmx_see_raw_buf_info
{
    __u32 rd;
    __u32 wr;
    __u32 buf;

    /* Not used. 
     */
    __u16 mutex;
};

/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 *  struct dmx_see_init_param
 *  {
 *      volatile struct SEE_PARSE_INFO *p_see_parse_info;
 *      UINT32 see_blk_buf;
 *      UINT32 see_blk_buf_size;
 *  };
 */
struct dmx_see_init_param
{
    /* Main to SEE buffer.
    */
    volatile struct dmx_see_raw_buf_info *main2see_buf;

    /* SEE decrypt buffer.
    */
    __u32 decrypt_buf;
    __u32 decrypt_buf_size;

    /* SEE statistics.
    */  
    volatile struct Ali_DmxSeeStatistics *statistics;
	volatile struct Ali_DmxSeeGlobalStatInfo* GlobalStatInfo;
	volatile struct Ali_DmxSeePlyChStatInfo* PlyChStatInfo;
};
 
#else
/* For M3527.
*/
/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 * struct share_wt_rd_pointer
 * {
 *    UINT32 rd;
 *    UINT32 wt;
 * };
 */
struct share_wt_rd_pointer
{
    __u32 rd;
    __u32 wt;
};

/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 * struct SEE_PARSE_INFO   
 */
struct SEE_PARSE_INFO       //Data Direction:MAIN->SEE
{
    struct share_wt_rd_pointer *ptr_wt_rd;
    __u32 dmx_ts_blk ;
    __u16 mutex;
};

/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 *  struct dmx_see_init_param
 *  {
 *      volatile struct SEE_PARSE_INFO *p_see_parse_info;
 *      UINT32 see_blk_buf;
 *      UINT32 see_blk_buf_size;
 *  };
 */
struct dmx_see_init_param
{
    volatile struct SEE_PARSE_INFO *p_see_parse_info;
    UINT32 see_blk_buf;
    UINT32 see_blk_buf_size;
};


/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 *  struct MAIN_PARSE_INFO
 */
struct MAIN_PARSE_INFO      //Data Direction:SEE->MAIN
{
    struct share_wt_rd_pointer *ptr_wt_rd;
    UINT32 dmx_ts_blk;
    __u16 mutex;
};

/* Used to communicate with SEE.
 * Must keep compartible with TDS:
 *  struct dmx_main_init_param
 */
struct dmx_main_init_param
{
    volatile struct MAIN_PARSE_INFO *p_main_parse_info;
    UINT32 main_blk_buf;
    UINT32 main_blk_buf_size;
};
typedef struct
{
    UINT16 aud_pid;
    UINT16 vde_pid;
    UINT32 aud_pkt_num;
    UINT32 vde_pkt_num;
    UINT32 aud_pkt_discontinue;
    UINT32 vde_pkt_discontinue;
    UINT32 total_pkt_discontinue;
    UINT32 vde_pes_lost_num;
    UINT32 aud_pes_lost_num;
    UINT32 parse_aud_pts_instream;
    UINT32 parse_vde_pts_instream;
    UINT32 parse_stream_busy_cnt;
    UINT32 sed_dataflow_control_option;
    UINT32 sed_vde_req_busy_cnt;
    UINT32 sed_vde_req_fail_cnt;
    UINT32 sed_vde_req_ok_cnt;
    UINT32 sed_aud_req_busy_cnt;
    UINT32 sed_aud_req_fail_cnt;
    UINT32 sed_aud_req_ok_cnt;
    UINT32 sed_other_req_busy_cnt;
    UINT32 sed_other_req_fail_cnt;
    UINT32 sed_other_req_ok_cnt;
}sed_dmx_dbg_info_t;
#endif












struct dmx_see_device
{
    __u8 name[16];

    enum DMX_SEE_STATUS status;

    enum dmx_see_av_sync_mode av_sync_mode;

    dev_t dev_id;

    /* Linux char device for dmx see.
     */
    struct cdev cdev;

    struct mutex mutex; 

    struct dmx_see_av_pid_info usr_param;

    __u32 video_pid;

    __u32 audio_pid;

    __u16 pcr_pid;

    __u16 ttx_pid;

    __u16 subt_pid;

	__u16 dcii_subt_pid;

    __u8 last_ts_hdr[4];

    /* For SEE.
     */
    struct dmx_see_init_param see_buf_init;

    /* For see pes parsing.
     */
    struct pes_pkt_param pes_para[7];

    struct control_block ctrl_blk[7];

    /* Old version of av scramble status in TDS.
     */
    __u8 av_scram_status;

    /* New version of av scramble status in TDS.
     */
    struct dmx_see_av_scram_info scram_param_linux;

    struct io_param_ex scram_param_tds;

    __u32 is_last_ts_resend;

	/* SEE to main buffer.
	*/
	struct dmx_main_init_param see2main_buf_init;

	/* For legacy API.
	*/
	__u32 last_chk_see_discon_cnt;

	__u8 used;

	__u32 dmx_main2see_buf_valid_size;
	enum Ali_DmxMain2seeSrc dmx_main2see_src;
    __u32 index;

	volatile __u32 *p_cur_pcr;
};

extern struct dmx_see_device ali_dmx_see_dev[DMX_SEE_DEV_MAX];
extern struct dmx_data_engine_module ali_dmx_data_engine_module;


void sed_dmx_get_dbg_info(UINT32 dmx_see_id, sed_dmx_dbg_info_t *dbg_info);
void sed_dmx_dataflow_control_set(UINT32 dmx_see_id, UINT32 control_potion);
__s32 dmx_see_buf_wr_ts(struct dmx_ts_pkt_inf *pkt_inf, __u32 param);
__s32 dmx_see_set_pcr(struct dmx_see_device *dmx_see, __u32 pcr);
__s32 dmx_see_get_statistics(struct Ali_DmxSeeStatistics *statistics);


__s32 dmx_see_video_stream_start(__u32 pid);
__s32 dmx_see_video_stream_stop(__u32 dummy);

__s32 dmx_see_audio_stream_start(__u32 pid);
__s32 dmx_see_audio_stream_stop(__u32 dummy);

__s32 Sed_DmxSee2mainPidAdd(__u32 Pid);
__s32 Sed_DmxSee2mainPidDel(__u32 Pid);
RET_CODE sed_add_scramble_pid(UINT32 dmx_see_id, UINT32 dev_addr, UINT16 pid);
RET_CODE sed_delete_scramble_pid(UINT32 dmx_see_id, UINT32 dev_addr, UINT16 pid);


__u32 dmx_see_see2main_buf_rd_get(__u32 dev_idx);

__u32 dmx_see_see2main_buf_rd_set(__u32 dev_idx, __u32 rd);


__u32 dmx_see_see2main_buf_wr_get(__u32 dev_idx);

__s32 dmx_see_see2main_buf_wr_set(__u32 dev_idx, __u32 wr);


__u32 dmx_see_see2main_buf_end_idx_get(__u32 dev_idx);

__u32 dmx_see_see2main_buf_start_addr_get(__u32 dev_idx);


__s32 dmx_see_ttx_stream_start(__u32 pid);
__s32 dmx_see_ttx_stream_stop(void);

__s32 dmx_see_subt_stream_start(__u32 pid);
__s32 dmx_see_subt_stream_stop(void);

__s32 dmx_see_ch_info_get(struct Ali_DmxSeePlyChStatInfo *ch_statistics, __s32 ch_cnt);
__s32 dmx_see_glb_info_get(struct Ali_DmxSeeGlobalStatInfo *glb_inf);

__s32 dmx_see_init(void);


#endif

