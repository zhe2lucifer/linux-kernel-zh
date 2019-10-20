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

/****************************************************************************(I)(S)
 *  File: ali_rpc_hld_decv.c
 *  (I)
 *  Description: hld decv remote process call api
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.04.08			Sam			Create
 ****************************************************************************/
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld_decv.h>
#include "../ali_rpc.h"

static UINT32 desc_vdec_m36_attach[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_config_par)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

void vdec_m36_attach(struct vdec_config_par *pconfig_par)
{
    UINT32 *desc = desc_vdec_m36_attach;

    DESC_STATIC_STRU_SET_SIZE(desc, 0, (pconfig_par->dev_num*sizeof(struct vdec_config_par)));

    jump_to_func(NULL, ali_rpc_call, pconfig_par, (LLD_DECV_M36_MODULE<<24)|(1<<16)|FUNC_VDEC_M36_ATTACH, desc);
}

RET_CODE vdec_enable_advance_play(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (LLD_DECV_M36_MODULE<<24)|(1<<16)|FUNC_VDEC_ENABLE_ADVANCE_PLAY, NULL);

	return ret;
}

static UINT32 desc_vdec_avc_attach[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_avc_config_par)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

void vdec_avc_attach(struct vdec_avc_config_par *pconfig_par)
{
    UINT32 *desc = desc_vdec_avc_attach;

    DESC_STATIC_STRU_SET_SIZE(desc, 0, (pconfig_par->dev_num*sizeof(struct vdec_avc_config_par)));

    jump_to_func(NULL, ali_rpc_call, pconfig_par, (LLD_DECV_AVC_MODULE<<24)|(1<<16)|FUNC_VDEC_AVC_ATTACH, desc);
}

#ifdef CONFIG_RPC_H265
static UINT32 desc_vdec_hevc_attach[] =
{
    //desc of pointer para
    1, DESC_STATIC_STRU(0, sizeof(struct vdec_hevc_config_par)),
    1, DESC_P_PARA(0, 0, 0),
    //desc of pointer ret
    0,
    0,
};
#endif
#ifdef CONFIG_RPC_H265
void vdec_hevc_attach(struct vdec_hevc_config_par *pconfig_par)
{
    jump_to_func(NULL, ali_rpc_call, pconfig_par, \
        (LLD_HHE_MODULE<<24)|(1<<16)|FUNC_VDEC_HEVC_ATTACH, desc_vdec_hevc_attach);
}
#endif

static UINT32 desc_vdec_avs_attach[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_avs_config_par)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

void vdec_avs_attach(struct vdec_avs_config_par *pconfig_par)
{
    jump_to_func(NULL, ali_rpc_call, pconfig_par, (LLD_DECV_AVS_MODULE<<24)|(1<<16)|FUNC_VDEC_AVS_ATTACH, desc_vdec_avs_attach);
}

static UINT32 desc_vdec_p_uint32[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, 4),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_get_status[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct vdec_status_info)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_get_decore_status[] =
{ //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct vdec_decore_status)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_fill_frm[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct ycb_cr_color)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

UINT32 desc_vdec_reg_callback[] =
{   //desc of pointer para
    1, DESC_STATIC_STRU(0, sizeof(struct vdec_io_reg_callback_para)),
    1, DESC_P_PARA(0, 2, 0),
    //desc of pointer ret
    0,
    0,
};

static UINT32 desc_vdec_vbv_request[] =
{ //desc of pointer para
	2, DESC_OUTPUT_STRU(0, 4), DESC_OUTPUT_STRU(1, 4),
	2, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_set_output[] =
{   //desc of pointer para
	3, DESC_STATIC_STRU(0, sizeof(struct vdec_pipinfo)), DESC_OUTPUT_STRU(1, sizeof(struct mpsource_call_back)), DESC_OUTPUT_STRU(2, sizeof(struct pipsource_call_back)),
	3, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1), DESC_P_PARA(2, 4, 2),
	//desc of pointer ret
	0,
	0,
};
#if 0
static UINT32 desc_vdec_switch_pip[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct position)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_switch_pip_ext[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct rect)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};
#endif 

static UINT32 desc_vdec_set_output_rect[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_pipinfo)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cap_disp_frm[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct vdec_picture)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cap_frm_info[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct vdec_capture_frm_info)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_io_dbg_flag_info[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_io_dbg_flag_info)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_set_trick_mode[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_playback_param)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_set_pip_param[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_pip_param)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

RET_CODE vdec_open(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_OPEN, NULL);

	return ret;
}

RET_CODE vdec_close(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_CLOSE, NULL);

	return ret;
}

RET_CODE vdec_start(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_START, NULL);

	return ret;
}

RET_CODE vdec_stop(struct vdec_device *dev,BOOL bclosevp, BOOL bfillblack)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_STOP, NULL);

	return ret;
}

RET_CODE vdec_vbv_request(void *dev, UINT32 size_requested, void **v_data,
                             UINT32 *size_got, struct control_block *ctrl_blk)
{
	register RET_CODE ret asm("$2");

	if(ctrl_blk != NULL)
    {
		//Just assume ctrl_blk should be NULL for remote call, for current remote call have bug to support NULL pointer.
		//After remote call support NULL pointer and we update desc_vdec_vbv_request for "ctrl_blk",
		//then we can remove this ASSERT.
		//--Michael Xie 2010/2/10
		return RET_FAILURE;
	}
	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(5<<16)|FUNC_VDEC_VBV_REQUEST, desc_vdec_vbv_request);

	return ret;
}

void vdec_vbv_update(void *dev, UINT32 uDataSize)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_VDEC_VBV_UPDATE, NULL);
}

RET_CODE vdec_set_output(struct vdec_device *dev, enum VDEC_OUTPUT_MODE mode,struct vdec_pipinfo *init_info,
                           struct mpsource_call_back *mp_call_back, struct pipsource_call_back *pip_call_back)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(5<<16)|FUNC_VDEC_SET_OUTPUT, desc_vdec_set_output);

	return ret;
}

RET_CODE vdec_sync_mode(struct vdec_device *dev, UINT8 sync_mode,UINT8 sync_level)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_SYNC_MODE, NULL);

	return ret;
}

RET_CODE vdec_profile_level(struct vdec_device *dev, UINT8 profile_level, VDEC_BEYOND_LEVEL cb_beyond_level)
{
	register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_PROFILE_LEVEL, NULL);

	return ret;
}

RET_CODE vdec_io_control(struct vdec_device *dev, UINT32 io_code, UINT32 param)
{
	register RET_CODE ret asm("$2");
	UINT32 *desc = NULL;

	switch(io_code)
	{
		case VDEC_IO_GET_STATUS:
			desc = desc_vdec_get_status;
			break;
        case VDEC_IO_GET_DECORE_STATUS:
            desc = desc_vdec_get_decore_status;
            break;
		case VDEC_IO_GET_MODE:
			desc = desc_vdec_p_uint32;
			break;
		case VDEC_IO_GET_OUTPUT_FORMAT:
			desc = desc_vdec_p_uint32;
			break;
		case VDEC_IO_FILL_FRM:
		case VDEC_IO_FILL_FRM2:
			desc = desc_vdec_fill_frm;
			break;
		case VDEC_IO_REG_CALLBACK:
			desc = desc_vdec_reg_callback;
			break;
        case VDEC_IO_SET_OUTPUT_RECT:
            desc = desc_vdec_set_output_rect;
            break;
        case VDEC_IO_CAPTURE_DISPLAYING_FRAME:
            desc = desc_vdec_cap_disp_frm;
            break;
		case VDEC_IO_SET_DBG_FLAG:
			desc = desc_vdec_io_dbg_flag_info;
			break;
		case VDEC_IO_SET_TRICK_MODE:
			desc = desc_vdec_set_trick_mode;
			break;
		case VDEC_IO_GET_CAPTURE_FRAME_INFO:
			desc = desc_vdec_cap_frm_info;
			break;
        case VDEC_IO_SET_PIP_PARAM:
            desc = desc_vdec_set_pip_param;
            break;
		case VDEC_IO_GET_FREEBUF:
		case VDEC_IO_COLORBAR:
		case VDEC_IO_SWITCH:
		case VDEC_IO_DONT_RESET_SEQ_HEADER:
		case VDEC_IO_SET_DROP_FRM:
		case VDEC_SET_DMA_CHANNEL:
		case VDEC_DTVCC_PARSING_ENABLE:
        case VDEC_VBV_BUFFER_OVERFLOW_RESET:
        case VDEC_IO_SET_SYNC_DELAY:
        case VDEC_IO_SAR_ENABLE:
        case VDEC_IO_FIRST_I_FREERUN:
        case VDEC_IO_CONTINUE_ON_ERROR:
        case VDEC_IO_SET_FREERUN_THRESHOLD:
        case VDEC_IO_DBLOCK_ENABLE:
        case VDEC_IO_SET_DEC_FRM_TYPE:
        case VDEC_IO_BG_FILL_BLACK:
        case VDEC_IO_SET_MULTIVIEW_WIN:
        case VDEC_IO_PARSE_AFD:
        case VDEC_IO_SEAMLESS_SWITCH_ENABLE:
        case VDEC_IO_RESTART_DECODE:
        case VDEC_IO_FLUSH:
        case VDEC_IO_GET_ALL_USER_DATA:
        case VDEC_IO_GET_USER_DATA_INFO:
		case VDEC_IO_SET_SYNC_REPEAT_INTERVAL:
			break;
		default:
			return RET_FAILURE;
	};

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_IO_CONTROL, desc);

	return ret;
}

RET_CODE vdec_playmode(struct vdec_device *dev, enum vdec_direction direction, enum vdec_speed speed)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_PLAYMODE, NULL);

	return ret;
}

RET_CODE vdec_dvr_set_param(struct vdec_device *dev, struct vdec_dvr_config_param param)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_VDEC_DVR_SET_PARAM, NULL);

	return ret;
}

RET_CODE vdec_dvr_pause(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_DVR_PAUSE, NULL);

	return ret;
}

RET_CODE vdec_dvr_resume(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

    	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_DVR_RESUME, NULL);

	return ret;
}

RET_CODE vdec_step(struct vdec_device *dev)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_DECV_MODULE<<24)|(1<<16)|FUNC_VDEC_STEP, NULL);

	return ret;
}

void h264_decoder_select(int select, BOOL in_preivew)
{
	jump_to_func(NULL, ali_rpc_call, select, (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_H264_DECODER_SELECT, NULL);
}

struct vdec_device * get_selected_decoder(void)
{
	register struct vdec_device *ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, null, (HLD_DECV_MODULE<<24)|(0<<16)|FUNC_GET_SELECTED_DECODER, NULL);

	return ret;
}

enum video_decoder_type get_current_decoder(void )
{
	register enum video_decoder_type ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, null, (HLD_DECV_MODULE<<24)|(0<<16)|FUNC_GET_CURRENT_DECODER, NULL);

	return ret;
}

BOOL is_cur_decoder_avc(void)
{
    register BOOL ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, null, (HLD_DECV_MODULE<<24)|(0<<16)|FUNC_IS_CUR_DECODER_AVC, NULL);

    return ret;
}

static UINT32 desc_vdec_get_cur_decoder[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, 4),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_get_cur_dev_name[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, 4),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

void video_decoder_select(enum video_decoder_type select, BOOL in_preview)
{
	jump_to_func(NULL, ali_rpc_call, select, (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_VIDEO_DECODER_SELECT, NULL);
}

RET_CODE vdec_select_decoder(INT32 vdec_id, enum video_decoder_type vdec_type)
{
    register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, vdec_id, \
        (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_VDEC_SELECT_DECODER, NULL);

    return ret;
}

RET_CODE vdec_get_cur_decoder(INT32 vdec_id, enum video_decoder_type *vdec_type)
{
    register RET_CODE ret asm("$2");

    jump_to_func(NULL, ali_rpc_call, vdec_id, \
        (HLD_DECV_MODULE<<24)|(2<<16)|FUNC_VDEC_GET_CUR_DECODER, desc_vdec_get_cur_decoder);

    return ret;
}

RET_CODE vdec_get_cur_dev_name(INT32 vdec_id, UINT32 *vdec_name, UINT8 len)
{
    register RET_CODE ret asm("$2");
    UINT32 *desc = desc_vdec_get_cur_dev_name;

    DESC_OUTPUT_STRU_SET_SIZE(desc, 0, (UINT32)len);

    jump_to_func(NULL, ali_rpc_call, vdec_id, \
        (HLD_DECV_MODULE<<24)|(3<<16)|FUNC_VDEC_GET_CUR_DEV_NAME, desc);

    return ret;
}

RET_CODE vdec_get_cur_device(INT32 vdec_id, UINT32 **vdec_dev)
{
    struct vdec_device *pdev = NULL;
    RET_CODE ret = RET_SUCCESS;
    UINT8 len = 16;
    char dev_name[16] = {0};

    if ((vdec_id >= VDEC_DEV_MAX) || (vdec_dev == NULL))
    {
        return RET_FAILURE;
    }

    memset(dev_name, 0, sizeof(dev_name));
    ret = vdec_get_cur_dev_name(vdec_id, (UINT32 *)dev_name, len);
    if (ret == RET_SUCCESS)
    {
        pdev = (struct vdec_device *)hld_dev_get_by_name(dev_name);
    }

    *vdec_dev = (UINT32 *)pdev;

    return ret;
}

static UINT32 desc_vdec_cmd_init[] =
{   //desc of pointer para
	2, DESC_OUTPUT_STRU(0, 4), DESC_STATIC_STRU(1, sizeof(vdec_init)),
	2, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_display_param[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_display_param)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cmd_reg_cb[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vdec_io_reg_callback_para)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cmd_sw_reset[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct output_frm_manager)),
	1, DESC_P_PARA(0, 3, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_display_rect[] =
{   //desc of pointer para
	2, DESC_STATIC_STRU(0, sizeof(struct av_rect)), DESC_STATIC_STRU(1, sizeof(struct av_rect)),
	2, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cmd_get_status[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct vdec_decore_status)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_decore_uint32[] =
{   //desc of pointer para
	2, DESC_OUTPUT_STRU(0, 4), DESC_OUTPUT_STRU(0, 4),
	2, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_cmd_extra_data[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, MAX_VIDEO_RPC_ARG_SIZE),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

static UINT32 desc_vdec_capture_frame[] =
{   //desc of pointer para
	2, DESC_OUTPUT_STRU(0, sizeof(struct vdec_picture)), DESC_STATIC_STRU(1, 4),
	2, DESC_P_PARA(0, 2, 0), DESC_P_PARA(1, 3, 1),
	//desc of pointer ret
	0,
	0,
};

int vdec_decore_ioctl(void *phandle, int cmd, void *param1, void *param2)
{
    register RET_CODE ret asm("$2");
    UINT32 func_desc;
	UINT32 *desc = NULL;

    func_desc = ((HLD_DECV_MODULE<<24)|(4<<16)|FUNC_VDEC_DECORE_IOCTL);

    switch(cmd)
    {
        case VDEC_CMD_INIT:
            desc = desc_vdec_cmd_init;
            break;
        case VDEC_CMD_EXTRA_DADA:
            desc = desc_vdec_cmd_extra_data;
            DESC_OUTPUT_STRU_SET_SIZE(desc, 0, (UINT32)param2);
            break;
		case VDEC_SET_DISPLAY_MODE:
				desc = desc_vdec_display_param;
				break;
		case VDEC_CMD_REG_CB:
				desc = desc_vdec_cmd_reg_cb;
			break;
        case VDEC_CMD_RELEASE:
        case VDEC_CMD_HW_RESET:
        case VDEC_CMD_PAUSE_DECODE:
        case VDEC_CFG_VIDEO_SBM_BUF:
        case VDEC_CFG_DISPLAY_SBM_BUF:
		case VDEC_STEP_DISPLAY:
            break;
        case VDEC_CMD_SW_RESET:
            desc = desc_vdec_cmd_sw_reset;
            break;
        case VDEC_CFG_SYNC_MODE:
        case VDEC_CFG_SYNC_THRESHOLD:
        case VDEC_CFG_DECODE_MODE:
        case VDEC_CFG_FREERUN_TRD:
        case VDEC_CFG_SYNC_PARAM:
        case VDEC_CFG_QUICK_MODE:
        case VDEC_DYNAMIC_FRAME_ALLOC:
            desc = desc_vdec_decore_uint32;
            break;
        case VDEC_CFG_DISPLAY_RECT:
            desc = desc_vdec_display_rect;
            break;
        case VDEC_CMD_GET_STATUS:
            desc = desc_vdec_cmd_get_status;
            break;
        case VDEC_CAPTURE_FRAME:
            desc = desc_vdec_capture_frame;
            break;
        case VDEC_SET_PIP_PARAM:
            desc = desc_vdec_set_pip_param;
            break;
        default:
            return RET_FAILURE;
    }

    jump_to_func(NULL, ali_rpc_call, phandle, func_desc, desc);

    return ret;
}

