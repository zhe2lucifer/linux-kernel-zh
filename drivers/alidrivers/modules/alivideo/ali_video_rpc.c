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
 *  File: ali_video_rpc.c
 *  (I)
 *  Description: rpc hld decv opeartion
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.04.09				Sam		Create
 ****************************************************************************/
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <asm/mach-ali/typedef.h>
#include "ali_video.h"

void hld_decv_set_cur_dev(struct ali_video_info *info, enum video_decoder_type vdec_type)
{
    RET_CODE ret = RET_SUCCESS;
    UINT8 len = 16;
    char dev_name[16] = {0};

    info->vdec_type = vdec_type;

    memset(dev_name, 0, sizeof(dev_name));
    ret = vdec_get_cur_dev_name(info->index, (UINT32*)dev_name, len);
    if (ret == RET_SUCCESS)
    {
        info->cur_dev = (struct vdec_device *)hld_dev_get_by_name(dev_name);
    }

	VDEC_PRINTF(info->index, "video set device 0x%x id %d\n", (int)info->cur_dev, vdec_type);
}

int hld_decv_request_buf(struct ali_video_info *info, void *instance,
                            void **buf_start, int *buf_size, struct ctrl_blk *blk)
{
	RET_CODE ret = 0;
	UINT32 size_request = (UINT32)(*buf_size);

	ret = vdec_vbv_request(info->cur_dev, size_request, buf_start, (UINT32 *)buf_size, (struct control_block *)blk);

	if(ret == RET_SUCCESS)
		ret = ALI_DECV_REQ_RET_OK;
	else if(ret == RET_FAILURE)
		ret = ALI_DECV_REQ_RET_FAIL;
	else
		ret = ALI_DECV_REQ_RET_ERROR;

	return ret;
}

void hld_decv_update_buf(struct ali_video_info *info, void *instance, int buf_size)
{
	vdec_vbv_update(info->cur_dev, buf_size);
}

static int decv_send_msg(struct kumsgq *video_kumsgq, void *msg_buf, int msg_size)
{
#ifdef CONFIG_ALI_TRANSPORT
	return ali_kumsgq_sendmsg(video_kumsgq, msg_buf, msg_size);
#else
	return 0;
#endif
}

static void decv_pcb_first_showed(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_FIRST_SHOWED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);
    VDEC_PRINTF(info->index, "video first picture showed\n");
}

static void decv_pcb_mode_switch_ok(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_MODE_SWITCH_OK;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);
}

static void decv_pcb_backward_reset_gop(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_BACKWARD_RESTART_GOP;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);
}

static void decv_pcb_first_header_parsed(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_FIRST_HEADRE_PARSED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video first header parsed\n");
}

static void decv_pcb_first_i_decoded(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_FIRST_I_DECODED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video first i decoded\n");
}

static void decv_pcb_vdec_error(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_ERROR;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video error %lx type %lu\n", uParam1, uParam2);
}

static void decv_pcb_vdec_state_changed(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_STATE_CHANGED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video state change to %lu\n", uParam1);
}

static void decv_pcb_vdec_new_frame(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_NEW_FRAME;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video new frame\n");
}

static void decv_pcb_vdec_start(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_STARTED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video started\n");
}

static void decv_pcb_vdec_stop(UINT32 uParam1, UINT32 uParam2)
{
    struct ali_video_info *info = NULL;
    UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

    info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_STOPPED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video stopped\n");
}

static void decv_pcb_frame_displayed(UINT32 uParam1, UINT32 uParam2)
{
	struct ali_video_info *info = NULL;
	UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];

	info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;

	msg[0] = MSG_FRAME_DISPLAYED;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);

    VDEC_PRINTF(info->index, "video new frame\n");
}

static void decv_pcb_vdec_monitor_gop(UINT32 uParam1, UINT32 uParam2)
{
	struct ali_video_info *info = NULL;
	UINT8 vdec_id = (uParam1>>24)&0xff;
	UINT8 msg[4];
	info = ali_video_priv[vdec_id];
    uParam1 &= 0x00FFFFFF;
	msg[0] = MSG_MONITOR_GOP;
	msg[1] = 2;
	msg[2] = uParam1;
	msg[3] = uParam2;
	decv_send_msg(info->video_kumsgq, (void *)msg, 4);
    VDEC_PRINTF(info->index, "vdec_monitor_gop\n");
}
static void decv_pcb_user_data_parsed(UINT32 param1, UINT32 param2)
{
    struct ali_video_info *info = NULL;
	UINT8 msg[4+DTVCC_USER_DATA_LENGTH_MAX];
    UINT32 size = param1 & 0x00FFFFFF;
    UINT8 *buf = (UINT8 *)param2;
    UINT8 vdec_id = (param1>>24) & 0xFF;

    if(size > DTVCC_USER_DATA_LENGTH_MAX)
    {
        return;
    }

    info = ali_video_priv[vdec_id];

	msg[0] = MSG_USER_DATA_PARSED;
	msg[1] = 2;
	msg[2] = (UINT8)size;
	msg[3] = (UINT8)(size>>8);
    memcpy(&msg[4], buf, (UINT16)size);
	decv_send_msg(info->video_kumsgq, (void *)msg, 4 + size);
}

static void decv_pcb_vdec_info_changed(UINT32 vdec_id, UINT32 param2)
{
    struct ali_video_info *info = ali_video_priv[vdec_id];
    struct vdec_info_cb_param *pvedc_info = (struct vdec_info_cb_param *)param2;
    UINT16 size = sizeof(struct vdec_info_cb_param);
    UINT8 msg[4+sizeof(struct vdec_info_cb_param)];

    if(pvedc_info == NULL)
    {
        return;
    }

    msg[0] = MSG_INFO_CHANGED;
    msg[1] = 2;
    msg[2] = (UINT8)size;
    msg[3] = (UINT8)(size>>8);
    memcpy(&msg[4], (void *)pvedc_info, (UINT16)size);
    decv_send_msg(info->video_kumsgq, (void *)msg, 4+size);

    VDEC_PRINTF(info->index, "video info changed flag 0x%lx width %lu height %lu frame rate %lu\n",
        pvedc_info->info_change_flags, pvedc_info->pic_width, pvedc_info->pic_height, pvedc_info->frame_rate);
}

static void vdec_cb_info_routine(UINT32 param1, UINT32 param2)
{
    struct vdec_callback *vdec_cb = NULL;
    struct vdec_info_cb_param *pvedc_info = (struct vdec_info_cb_param *)param2;
    UINT8 vdec_id = (pvedc_info->info_change_flags>>24) & 0xFF;

    pvedc_info->info_change_flags &= ~(0xFF<<24);

    if (vdec_id < DEVICE_NUM)
        vdec_cb = &(ali_video_priv[vdec_id]->call_back);
    else
        return;

    if(vdec_cb->pcb_vdec_info_changed)
    {
        vdec_cb->pcb_vdec_info_changed(vdec_id, param2);
    }
}

static void vdec_cb_spec_routine(UINT32 param1, UINT32 param2)
{
    struct vdec_callback *vdec_cb = NULL;
    struct user_data_pram *puser_data = (struct user_data_pram *)param2;
    UINT8 vdec_id = (puser_data->user_data_size>>24) & 0xFF;

    if (vdec_id < DEVICE_NUM)
        vdec_cb = &(ali_video_priv[vdec_id]->call_back);
    else
        return;

    if(vdec_cb->pcb_vdec_user_data_parsed)
    {
        vdec_cb->pcb_vdec_user_data_parsed(puser_data->user_data_size, (UINT32)puser_data->user_data);
    }
}

static void vdec_cb_routine(UINT32 res, UINT32 uParam)
{
    struct vdec_callback *vdec_cb = NULL;
	UINT32 uParam1 = (uParam>>8)&0xff;
	UINT32 uParam2 = (uParam>>16)&0xff;
	UINT8 type = uParam&0xff;
    UINT8 vdec_id = (uParam>>24)&0xff;

	//VDEC_PRINTF(vdec_id, "video callback type %u\n", type);

    if (vdec_id < DEVICE_NUM)
        vdec_cb = &(ali_video_priv[vdec_id]->call_back);
    else
        return;

    uParam1 |= (vdec_id<<24);

	switch(type)
	{
		case VDEC_CB_FIRST_SHOWED:
			if(vdec_cb->pcb_first_showed)
				vdec_cb->pcb_first_showed(uParam1, uParam2);
			break;
		case VDEC_CB_MODE_SWITCH_OK:
			if(vdec_cb->pcb_mode_switch_ok)
				vdec_cb->pcb_mode_switch_ok(uParam1, uParam2);
			break;
		case VDEC_CB_BACKWARD_RESTART_GOP: // used for advance play
			if(vdec_cb->pcb_backward_restart_gop)
				vdec_cb->pcb_backward_restart_gop(uParam1, uParam2);
			break;
		case VDEC_CB_FIRST_HEAD_PARSED:
			if(vdec_cb->pcb_first_head_parsed)
				vdec_cb->pcb_first_head_parsed(uParam1, uParam2);
			break;
        case VDEC_CB_FIRST_I_DECODED:
			if(vdec_cb->pcb_first_i_decoded)
				vdec_cb->pcb_first_i_decoded(uParam1, uParam2);
			break;
        case VDEC_CB_MONITOR_VDEC_START:
            if(vdec_cb->pcb_vdec_start)
				vdec_cb->pcb_vdec_start(uParam1, uParam2);
            break;
        case VDEC_CB_MONITOR_VDEC_STOP:
            if(vdec_cb->pcb_vdec_stop)
				vdec_cb->pcb_vdec_stop(uParam1, uParam2);
            break;
        case VDEC_CB_MONITOR_FRAME_VBV:
            if(vdec_cb->pcb_vdec_new_frame)
				vdec_cb->pcb_vdec_new_frame(uParam1, uParam2);
            break;
        case VDEC_CB_ERROR:
            if(vdec_cb->pcb_vdec_error)
				vdec_cb->pcb_vdec_error(uParam1, uParam2);
            break;
        case VDEC_CB_STATE_CHANGED:
            if(vdec_cb->pcb_vdec_state_changed)
				vdec_cb->pcb_vdec_state_changed(uParam1, uParam2);
            break;
		case VDEC_CB_FRAME_DISPLAYED:
			if (vdec_cb->pcb_frame_displayed)
				vdec_cb->pcb_frame_displayed(uParam1, uParam2);
			break;
		case VDEC_CB_MONITOR_GOP:
			if (vdec_cb->pcb_vdec_monitor_gop)
				vdec_cb->pcb_vdec_monitor_gop(uParam1, uParam2);
			break;
		default:
			break;
	}
}

static int vdec_capture_frame(struct ali_video_info *info, struct vdec_picture *ppic)
{
    struct vdec_capture_frm_info frm_info;
    unsigned char *yuv_y = NULL;
    unsigned char *yuv_u = NULL;
    unsigned char *yuv_v = NULL;
    int ret = RET_FAILURE;

    if(!info->capture_mem_addr || !info->capture_mem_size)
    {
        return ret;
    }

    memset(&frm_info, 0, sizeof(frm_info));

    if(info->work_mode)
    {
        ret = vdec_decore_ioctl(info->codec_handle, VDEC_GET_CAPTURE_FRAME_INFO, (void*)&frm_info, NULL);
    }
    else
    {
        ret = vdec_io_control(info->cur_dev, VDEC_IO_GET_CAPTURE_FRAME_INFO, (UINT32)&frm_info);
    }

    if(ret == RET_SUCCESS)
    {
        yuv_y = (unsigned char *)__VSTMALI(info->capture_mem_addr);
        yuv_u = yuv_y + frm_info.y_buf_size;
        yuv_v = yuv_u + (frm_info.y_buf_size>>2);

        if(frm_info.de_map_mode)
        {
            convert_h264_de2yuv((unsigned char *)__VSTMALI(frm_info.y_buf_addr), \
                                (unsigned char *)__VSTMALI(frm_info.c_buf_addr), \
                                frm_info.pic_width, frm_info.pic_height, \
                                yuv_y, yuv_u, yuv_v, \
                                frm_info.pic_stride, frm_info.pic_stride>>1);
        }
        else
        {
            convert_mpeg2_de2yuv((unsigned char *)__VSTMALI(frm_info.y_buf_addr), \
                                 (unsigned char *)__VSTMALI(frm_info.c_buf_addr), \
                                 frm_info.pic_width, frm_info.pic_height, \
                                 yuv_y, yuv_u, yuv_v, \
                                 frm_info.pic_stride, frm_info.pic_stride>>1);
        }

        ppic->type = VDEC_PIC_YV12;
        ppic->pic_width  = frm_info.pic_width;
        ppic->pic_height = frm_info.pic_height;
        ppic->out_data_valid_size = frm_info.y_buf_size + (frm_info.y_buf_size>>1);
        if(ppic->out_data_buf && ppic->out_data_valid_size \
           && (ppic->out_data_buf_size >= ppic->out_data_valid_size))
        {
            if(copy_to_user((char *)ppic->out_data_buf, (char *)yuv_y, ppic->out_data_valid_size))
            {
                ret = RET_FAILURE;
            }
        }
        else
        {
            ret = RET_FAILURE;
        }

        VDEC_PRINTF(info->index, "video capture frame width %lu height %lu map %u\n",
            ppic->pic_width, ppic->pic_height, frm_info.de_map_mode);
    }

    return ret;
}

int vdec_display_mode(struct ali_video_info *info, struct vdec_display_param *pparam)
{
	struct vdec_pipinfo pip_info;
	struct mpsource_call_back mp_callback;
	struct pipsource_call_back pip_callback;
	enum VDEC_OUTPUT_MODE mode = MP_MODE;
	struct vpo_io_get_info vpo_info;
	int ret = RET_FAILURE;

    if (info->work_mode) {
        ret = vdec_decore_ioctl(info->codec_handle, VDEC_SET_DISPLAY_MODE, (void *)pparam, NULL);
    } else {
		memset(&pip_info, 0, sizeof(pip_info));
		memset(&vpo_info, 0, sizeof(vpo_info));
#ifdef CONFIG_RPC_HLD_DIS
		ret = vpo_ioctl((struct vpo_device *)hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 0), VPO_IO_GET_INFO, (UINT32)&vpo_info);
#endif

		if (pparam->mode == VDEC_PREVIEW)
			mode = PREVIEW_MODE;
		else if (pparam->mode == VDEC_SW_PASS)
			mode = SW_PASS_MODE;

		pip_info.adv_setting.out_sys = vpo_info.tvsys;
		pip_info.adv_setting.bprogressive = vpo_info.bprogressive;
		pip_info.adv_setting.switch_mode = 1;
		pip_info.src_rect.u_start_x = pparam->rect.src_x;
		pip_info.src_rect.u_start_y = pparam->rect.src_y;
		pip_info.src_rect.u_width   = pparam->rect.src_w;
		pip_info.src_rect.u_height  = pparam->rect.src_h;
	    pip_info.dst_rect.u_start_x = pparam->rect.dst_x;
	    pip_info.dst_rect.u_start_y = pparam->rect.dst_y;
	    pip_info.dst_rect.u_width   = pparam->rect.dst_w;
	    pip_info.dst_rect.u_height  = pparam->rect.dst_h;
		pip_info.adv_setting.init_mode = (pparam->mode == VDEC_PREVIEW) ? 1 : 0;
        ret = vdec_io_control(info->cur_dev, VDEC_IO_SET_OUTPUT_RECT, (UINT32)&pip_info);
		ret = vdec_set_output(info->cur_dev, mode, &pip_info, &mp_callback, &pip_callback);
    }

    VDEC_PRINTF(info->index, "video set display mode %u <%ld %ld %ld %ld> => <%ld %ld %ld %ld>\n", pparam->mode,
		pparam->rect.src_x, pparam->rect.src_y, pparam->rect.src_w, pparam->rect.src_h,
		pparam->rect.dst_x, pparam->rect.dst_y, pparam->rect.dst_w, pparam->rect.dst_h);

	return ret;
}

static int vdec_get_multiview_buf(struct ali_video_info *info)
{
#if 0
    struct multiview_info muti_info;
    int ret = RET_FAILURE;
    multiview_buf_attr_t *multiview_buf_attr = (multiview_buf_attr_t *)request_attr(MULTIVIEW_BUF);

    muti_info.full_screen_height = 1080;
	muti_info.full_screen_width = 1920;
	muti_info.multiview_buf = multiview_buf_attr->multiview_buf_start_addr;

	ret = vdec_io_control(g_decv_dev0, VDEC_IO_GET_MULTIVIEW_BUF,(unsigned long)&muti_info);

	ret = vdec_io_control(g_decv_avc_dev0, VDEC_IO_GET_MULTIVIEW_BUF,(unsigned long)&muti_info);

    return ret;
#endif
    return 0;
}

static int io_control(struct ali_video_info *info)
{
    int ret = RET_FAILURE;

    switch(*(unsigned long *)(info->rpc_arg[0]))
    {
        case VDEC_IO_GET_STATUS:
        case VDEC_IO_GET_DECORE_STATUS:
        case VDEC_IO_FILL_FRM:
        case VDEC_IO_SET_OUTPUT_RECT:
        case VDEC_IO_FILL_BG_VIDEO:
        case VDEC_IO_SET_PIP_PARAM:
            ret = vdec_io_control(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]), (unsigned long)(info->rpc_arg[1]));
            break;
        case VDEC_IO_COLORBAR:  //only valid when vp had got one framebuffer from ve and vp.mp layer is opened.
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
		case VDEC_IO_SET_SYNC_REPEAT_INTERVAL:
            ret = vdec_io_control(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]), *(unsigned long *)(info->rpc_arg[1]));
            break;
        case VDEC_IO_CAPTURE_DISPLAYING_FRAME:
            ret = vdec_capture_frame(info, (struct vdec_picture *)info->rpc_arg[1]);
            break;
        case VDEC_IO_GET_MULTIVIEW_BUF:
            ret = vdec_get_multiview_buf(info);
            break;
        default:
            break;
    }

    return ret;
}

static void switch_working_mode(struct ali_video_info *info)
{
    enum video_decoder_type decoder = MPEG2_DECODER;
    RET_CODE ret = RET_SUCCESS;

    if(info->work_mode) {
        vdec_decore_ioctl(info->codec_handle, VDEC_CMD_RELEASE, 0, 0);
        info->codec_handle = NULL;

        vdec_get_cur_decoder(info->index, &decoder);

        if(info->vdec_type == MPEG2_DECODER && decoder != MPEG2_DECODER)
        {
            decoder = MPEG2_DECODER;
            ret = vdec_select_decoder(info->index, decoder);
            if (ret == RET_SUCCESS)
                hld_decv_set_cur_dev(info, decoder);
            else
                VDEC_PRINTF(info->index, "select decoder %d fail\n", decoder);
        }
        else if(info->vdec_type == H264_DECODER && decoder != H264_DECODER)
        {
            decoder = H264_DECODER;
            ret = vdec_select_decoder(info->index, decoder);
            if (ret == RET_SUCCESS)
                hld_decv_set_cur_dev(info, decoder);
            else
                VDEC_PRINTF(info->index, "select decoder %d fail\n", decoder);
        }
        else if(info->vdec_type == AVS_DECODER && decoder != AVS_DECODER)
        {
            decoder = AVS_DECODER;
            ret = vdec_select_decoder(info->index, decoder);
            if (ret == RET_SUCCESS)
                hld_decv_set_cur_dev(info, decoder);
            else
                VDEC_PRINTF(info->index, "select decoder %d fail\n", decoder);
        }
        else if(info->vdec_type == H265_DECODER && decoder != H265_DECODER)
        {
            decoder = H265_DECODER;
            ret = vdec_select_decoder(info->index, decoder);
            if (ret == RET_SUCCESS)
                hld_decv_set_cur_dev(info, decoder);
            else
                VDEC_PRINTF(info->index, "select decoder %d fail\n", decoder);
        }

        info->work_mode = 0;
        info->codec_tag = 0;
        info->pause_decode  = 0;
        info->pause_display = 0;

        VDEC_PRINTF(info->index, "video %u switch working mode to live play\n", decoder);
    }
}

static void rpc_set_dbg_flag(unsigned int *par)
{
	struct vdec_io_dbg_flag_info flag_info;

	memset((void *)&flag_info, 0, sizeof(flag_info));

	flag_info.dbg_flag = par[0];

	if(par[1])
		flag_info.active_flag = TRUE;
	else
		flag_info.active_flag = FALSE;

	if(par[2])
		flag_info.unique_flag = TRUE;
	else
		flag_info.unique_flag = FALSE;

	vdec_io_control(ali_video_priv[0]->cur_dev, VDEC_IO_SET_DBG_FLAG, (UINT32)&flag_info);
}

int hld_decv_rpc_operation(struct ali_video_info *info, int API_idx)
{
        vdec_init *pvdec_init = NULL;
        vdec_format_info *pfmt = NULL;
		struct vdec_io_reg_callback_para cb_param;
        int ret = RET_SUCCESS;

        //VDEC_PRINTF(info->index, "hld decv rpc operation api %d\n\n", API_idx);

        switch(API_idx)
        {
        	case RPC_VIDEO_START:
                switch_working_mode(info);
        		ret = vdec_start(info->cur_dev);
                VDEC_PRINTF(info->index, "video %u start, ret %d\n", info->vdec_type, ret);
        		break;
        	case RPC_VIDEO_STOP:
                if(info->logo_mode) {
                    hld_decv_logo_rls(info);
                }
        		ret = vdec_stop(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]), *(unsigned long *)(info->rpc_arg[1]));
                VDEC_PRINTF(info->index, "video %u stop <%ld %ld> ret %d\n",
                    info->vdec_type, *(unsigned long *)(info->rpc_arg[0]), *(unsigned long *)(info->rpc_arg[1]), ret);
        		break;
        	case RPC_VIDEO_SET_OUT:
        		ret = vdec_set_output(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]),
        			                  (struct vdec_pipinfo *)(info->rpc_arg[1]),
        			                  (struct mpsource_call_back *)(info->rpc_arg[2]),
        			                  (struct pipsource_call_back *)(info->rpc_arg[3]));
                VDEC_PRINTF(info->index, "video set output %ld ret %d\n", *(unsigned long *)(info->rpc_arg[0]), ret);
        		break;
        	case RPC_VIDEO_SYNC_MODE:
        		ret = vdec_sync_mode(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]),
                                     *(unsigned long *)(info->rpc_arg[1]));
        		break;
        	case RPC_VIDEO_IO_CONTROL:
        		ret = io_control(info);
        		break;
        	case RPC_VIDEO_PLAY_MODE:
        		ret = vdec_playmode(info->cur_dev, *(unsigned long *)(info->rpc_arg[0]),
                                    *(unsigned long *)(info->rpc_arg[1]));
        		break;
        	case RPC_VIDEO_DVR_SET_PAR:
        		ret = vdec_dvr_set_param(info->cur_dev, *(struct vdec_dvr_config_param *)(info->rpc_arg[0]));
        		break;
        	case RPC_VIDEO_DVR_PAUSE:
        		ret = vdec_dvr_pause(info->cur_dev);
        		break;
        	case RPC_VIDEO_DVR_RESUME:
        		ret = vdec_dvr_resume(info->cur_dev);
        		break;
            case RPC_VIDEO_DVR_STEP:
                ret = vdec_step(info->cur_dev);
                break;
        	case RPC_VIDEO_SELECT_DEC:
            case RPC_VIDEO_DECODER_SELECT_NEW:
                vdec_select_decoder(info->index, info->vdec_type);
                if (ret == RET_SUCCESS)
                    hld_decv_set_cur_dev(info, *(enum video_decoder_type *)(info->rpc_arg[0]));
                VDEC_PRINTF(info->index, "video select decoder %u preview %d ret %d\n",
                    *(enum video_decoder_type *)(info->rpc_arg[0]), *(BOOL *)(info->rpc_arg[1]), ret);
    			break;
        	case RPC_VIDEO_IS_AVC:
                vdec_get_cur_decoder(info->index, &info->vdec_type);
                return (info->vdec_type == H264_DECODER);
            case RPC_VIDEO_GET_DECODER:
                vdec_get_cur_decoder(info->index, &info->vdec_type);
    			return info->vdec_type;
            case RPC_VIDEO_DECORE_IOCTL:
                if(*(int *)info->rpc_arg[1] == VDEC_CFG_VIDEO_SBM_BUF
                   || *(int *)info->rpc_arg[1] == VDEC_CFG_DISPLAY_SBM_BUF
                   || *(int *)info->rpc_arg[1] == VDEC_CMD_PAUSE_DECODE) {
                    ret = vdec_decore_ioctl(info->codec_handle, *(int *)info->rpc_arg[1],
                                            (void*)*(int *)info->rpc_arg[2], (void*)*(int *)info->rpc_arg[3]);

                    if(*(int *)info->rpc_arg[1] == VDEC_CMD_PAUSE_DECODE) {
                        if(*(int *)info->rpc_arg[2] > 0)
                            info->pause_decode = 1;
                        else if(*(int *)info->rpc_arg[2] == 0)
                            info->pause_decode = 0;

                        if(*(int *)info->rpc_arg[3] > 0)
                            info->pause_display = 1;
                        else if(*(int *)info->rpc_arg[3] == 0)
                            info->pause_display = 0;
                    }
                } else if(*(int *)info->rpc_arg[1] == VDEC_CMD_EXTRA_DADA) {
                    ret = vdec_decore_ioctl(info->codec_handle, *(int *)info->rpc_arg[1],
                                            (void*)info->rpc_arg[2], (void*)*(int *)info->rpc_arg[3]);
                    return ret;
                } else if(*(int *)info->rpc_arg[1] == VDEC_CMD_SW_RESET) {
                    ret = vdec_decore_ioctl(info->codec_handle, *(int *)info->rpc_arg[1],
                                            (void*)*(int *)info->rpc_arg[2], (void*)info->rpc_arg[3]);
                } else if(*(int *)info->rpc_arg[1] == VDEC_CAPTURE_FRAME) {
                    ret = vdec_capture_frame(info, (struct vdec_picture *)info->rpc_arg[2]);
                } else {
                    if(*(int *)info->rpc_arg[1] == VDEC_CMD_INIT) {
                        info->work_mode = 1;
                        info->codec_tag = *(unsigned int *)info->rpc_arg[0];
                        info->pause_decode  = 0;
                        info->pause_display = 0;

                        switch(info->codec_tag)
                        {
                            case mpg2:
                                info->vdec_type = MPEG2_DECODER;
                                break;
                            case h264:
                                info->vdec_type = H264_DECODER;
                                break;
                            case hevc:
                                info->vdec_type = H265_DECODER;
                                break;
                            case vc1:
                                info->vdec_type = VC1_DECODER;
                                break;
                            case xvid:
                                info->vdec_type = MPEG4_DECODER;
                                break;
                            case vp8:
                                info->vdec_type = VP8_DECODER;
                                break;
                            case rmvb:
                                info->vdec_type = RV_DECODER;
                                break;
                            case mjpg:
                                info->vdec_type = MJPG_DECODER;
                                break;
                            default:
                                break;
                        }

                        pvdec_init = (vdec_init *)(info->rpc_arg[3]);
                        pfmt = &(pvdec_init->fmt_in);
                        /* just tell see to use decore callback */
                        pvdec_init->pfn_decore_de_request = (void *)0x1;
                        pvdec_init->pfn_decore_de_release = (void *)0x1;
                        pvdec_init->dec_buf_addr  = info->mem_addr;
                        pvdec_init->dec_buf_size  = info->mem_size;
                        pvdec_init->priv_buf_addr = info->priv_mem_addr;
                        pvdec_init->priv_buf_size = info->priv_mem_size;
                        pvdec_init->vbv_buf_addr  = info->vbv_mem_addr;
                        pvdec_init->vbv_buf_size  = info->vbv_mem_size;
                        pvdec_init->vdec_id       = info->index;
                        ret = vdec_decore_ioctl((void*)info->rpc_arg[0], *(int *)info->rpc_arg[1],
                                                &info->codec_handle, (void*)info->rpc_arg[3]);

                        VDEC_PRINTF(info->index, "video %c%c%c%c init resolution <%ldx%ld> frame rate %lu aspect <%ldx%ld>\n",
                            (UINT8)pfmt->fourcc, (UINT8)(pfmt->fourcc>>8),
                            (UINT8)(pfmt->fourcc>>16), (UINT8)(pfmt->fourcc>>24),
                            pfmt->pic_width, pfmt->pic_height, pfmt->frame_rate,
                            pfmt->pixel_aspect_x, pfmt->pixel_aspect_y);

                        VDEC_PRINTF(info->index, "video buffer: fb 0x%lx %lu priv 0x%lx %lu vbv 0x%lx %lu\n",
                            pvdec_init->dec_buf_addr, pvdec_init->dec_buf_size, pvdec_init->priv_buf_addr,
                            pvdec_init->priv_buf_size, pvdec_init->vbv_buf_addr, pvdec_init->vbv_buf_size);

                    }else {
                        ret = vdec_decore_ioctl(info->codec_handle, *(int *)info->rpc_arg[1],
                                                (void*)info->rpc_arg[2], (void*)info->rpc_arg[3]);
                    }

                    if(*(int *)info->rpc_arg[1] == VDEC_CMD_RELEASE) {
                        info->work_mode = 0;
                        info->codec_tag = 0;
                        info->pause_decode  = 0;
                        info->pause_display = 0;
                        info->codec_handle = NULL;
                        vdec_get_cur_decoder(info->index, &info->vdec_type);
                        hld_decv_set_cur_dev(info, info->vdec_type);
                        VDEC_PRINTF(info->index, "video release\n");
                    } else if (*(int *)info->rpc_arg[1] == VDEC_CMD_INIT) {
				        if (info->debug_on) {
				            cb_param.monitor_rate = 0;
				            cb_param.p_cb = (vdec_cbfunc)1;
							cb_param.e_cbtype = VDEC_CB_FIRST_I_DECODED;
							vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
							cb_param.e_cbtype = VDEC_CB_FIRST_SHOWED;
							vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
							cb_param.e_cbtype = VDEC_CB_ERROR;
							vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
							cb_param.e_cbtype = VDEC_CB_INFO_CHANGE;
							vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
							cb_param.e_cbtype = VDEC_CB_STATE_CHANGED;
							vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
				        }
                    }
                }
                break;
            case RPC_VIDEO_SET_DBG_FLAG:
    		{
    			rpc_set_dbg_flag((unsigned int *)info->rpc_arg[0]);
    			break;
    		}
        	default:
        		ret = -1;
        		break;
        }

        if(ret != RET_SUCCESS) {
        	ret = -1;
        }
        return ret;
}

void hld_decv_rpc_suspend(struct ali_video_info *info)
{
    if(info->work_mode == 0) {
        vdec_stop(info->cur_dev, 0, 0);
    } else {
        hld_decv_rpc_release(info, 0);
    }
}

void hld_decv_rpc_resume(struct ali_video_info *info)
{
    if(info->work_mode == 0) {
        vdec_start(info->cur_dev);
    } else {

    }
}

void hld_decv_rpc_release(struct ali_video_info *info, int force)
{
	if((info->work_mode == 1) || (force == 1))
	{
		info->work_mode = 0;
		info->pause_decode = 0;

		if((info->vdec_type == MPEG2_DECODER) || (info->vdec_type == H264_DECODER)
           || (info->vdec_type == AVS_DECODER) || (info->vdec_type == H265_DECODER))
		{
			vdec_stop(info->cur_dev, FALSE, FALSE);
		}

		vdec_decore_ioctl(info->codec_handle, VDEC_CMD_RELEASE, 0, 0);

		if((info->vdec_type == MPEG2_DECODER) || (info->vdec_type == H264_DECODER)
           || (info->vdec_type == AVS_DECODER) || (info->vdec_type == H265_DECODER))
		{
            vdec_select_decoder(info->index, MPEG2_DECODER);
			hld_decv_set_cur_dev(info, MPEG2_DECODER);
		}
	}
}

void hld_decv_rpc_init(struct ali_video_info *info)
{
	ali_rpc_register_callback(ALI_RPC_CB_VDEC, vdec_cb_routine);
    ali_rpc_register_callback(ALI_RPC_CB_VDEC_SPEC, vdec_cb_spec_routine);
    ali_rpc_register_callback(ALI_RPC_CB_VDEC_INFO, vdec_cb_info_routine);

	info->call_back.pcb_first_showed = decv_pcb_first_showed;
	info->call_back.pcb_mode_switch_ok = decv_pcb_mode_switch_ok;
	info->call_back.pcb_backward_restart_gop = decv_pcb_backward_reset_gop;
	info->call_back.pcb_first_head_parsed = decv_pcb_first_header_parsed;
    info->call_back.pcb_first_i_decoded = decv_pcb_first_i_decoded;
    info->call_back.pcb_vdec_user_data_parsed = decv_pcb_user_data_parsed;
    info->call_back.pcb_vdec_info_changed = decv_pcb_vdec_info_changed;
    info->call_back.pcb_vdec_error = decv_pcb_vdec_error;
    info->call_back.pcb_vdec_state_changed = decv_pcb_vdec_state_changed;
    info->call_back.pcb_vdec_new_frame = decv_pcb_vdec_new_frame;
    info->call_back.pcb_vdec_start = decv_pcb_vdec_start;
    info->call_back.pcb_vdec_stop = decv_pcb_vdec_stop;
	info->call_back.pcb_frame_displayed = decv_pcb_frame_displayed;
	info->call_back.pcb_vdec_monitor_gop= decv_pcb_vdec_monitor_gop;

	return;
}

INT32 hld_decv_start(struct ali_video_info *info)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    switch_working_mode(info);
    ret_code = vdec_start(info->cur_dev);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video %u start, ret %ld\n", info->vdec_type, res);

    return res;
}

INT32 hld_decv_stop(struct ali_video_info *info, struct vdec_stop_param *stop_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (stop_param == NULL))
    {
        return res;
    }

    if(info->logo_mode) {
        res = hld_decv_logo_rls(info);
    }

    ret_code = vdec_stop(info->cur_dev, stop_param->close_display, stop_param->fill_black);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video %u stop <%ld %ld> ret %ld\n",
        info->vdec_type, stop_param->close_display, stop_param->fill_black, res);

    return res;
}

INT32 hld_decv_pause(struct ali_video_info *info)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_dvr_pause(info->cur_dev);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video pause %ld\n", res);

    return res;
}

INT32 hld_decv_resume(struct ali_video_info *info)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_dvr_resume(info->cur_dev);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video resume %ld\n", res);

    return res;
}

INT32 hld_decv_step(struct ali_video_info *info)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_step(info->cur_dev);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video step %ld\n", res);

    return res;
}

enum vdec_type hld_decv_get_cur_decoder(struct ali_video_info *info)
{
    RET_CODE ret = RET_SUCCESS;

    ret = vdec_get_cur_decoder(info->index, &info->vdec_type);
    if (ret != RET_SUCCESS)
    {
        VDEC_PRINTF(info->index, "get cur decoder fail\n");
    }

    return info->vdec_type;
}

INT32 hld_decv_set_sync_mode(struct ali_video_info *info, struct vdec_sync_param *sync_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (sync_param == NULL))
    {
        return res;
    }

    ret_code = vdec_sync_mode(info->cur_dev, sync_param->sync_mode, VDEC_SYNC_I|VDEC_SYNC_P|VDEC_SYNC_B);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set sync mode %u ret %ld\n", sync_param->sync_mode, res);

    return res;
}

INT32 hld_decv_set_play_mode(struct ali_video_info *info, struct vdec_playback_param *playback_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (playback_param == NULL))
    {
        return res;
    }

    ret_code = vdec_playmode(info->cur_dev, playback_param->direction, playback_param->rate);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set play mode %u %u ret %ld\n", playback_param->direction, playback_param->rate, res);

    return res;
}

INT32 hld_decv_set_pvr_param(struct ali_video_info *info, struct vdec_pvr_param *pvr_param)
{
    struct vdec_dvr_config_param param;
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (pvr_param == NULL))
    {
        return res;
    }

    param.is_scrambled = pvr_param->is_scrambled;
    ret_code = vdec_dvr_set_param(info->cur_dev, param);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set pvr param %d ret %ld\n", param.is_scrambled, res);

    return res;
}

INT32 hld_decv_select_decoder(struct ali_video_info *info, struct vdec_codec_param *codec_param)
{
    struct vdec_io_reg_callback_para cb_param;
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (codec_param == NULL))
    {
        return res;
    }

    ret_code = vdec_select_decoder(info->index, codec_param->type);
    if (ret_code == RET_SUCCESS)
    {
        hld_decv_set_cur_dev(info, (int)(codec_param->type));
    }

    if (info->debug_on) {
        cb_param.monitor_rate = 0;
        cb_param.p_cb = (vdec_cbfunc)1;
		cb_param.e_cbtype = VDEC_CB_FIRST_I_DECODED;
		vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
		cb_param.e_cbtype = VDEC_CB_FIRST_SHOWED;
		vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
        cb_param.e_cbtype = VDEC_CB_INFO_CHANGE;
        vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
        cb_param.e_cbtype = VDEC_CB_ERROR;
        vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
        cb_param.e_cbtype = VDEC_CB_STATE_CHANGED;
        vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
    }

    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video select decoder %u preview %ld ret %ld\n",
        codec_param->type, codec_param->preview, res);

    return res;
}

INT32 hld_decv_set_output(struct ali_video_info *info, struct vdec_output_param *output_param)
{
    struct vdec_pipinfo pip_info;
    enum VDEC_OUTPUT_MODE mode = MP_MODE;
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (output_param == NULL))
    {
        return res;
    }

    memset(&pip_info, 0, sizeof(pip_info));

    if(output_param->output_mode == VDEC_PREVIEW)
    {
        mode = PREVIEW_MODE;
    }
    else if(output_param->output_mode == VDEC_SW_PASS)
    {
        mode = SW_PASS_MODE;
    }

    pip_info.adv_setting.init_mode = (mode == PREVIEW_MODE) ? 1 : 0;
    pip_info.adv_setting.bprogressive = output_param->progressive;
    pip_info.adv_setting.out_sys = output_param->tv_sys;
    ret_code = vdec_set_output(info->cur_dev, mode, &pip_info, &output_param->mp_callback, &output_param->pip_callback);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set output %u ret %ld\n", output_param->output_mode, res);

    return res;
}

INT32 hld_decv_ioctl(struct ali_video_info *info, UINT32 cmd, UINT32 param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    switch(cmd)
    {
        case VDEC_IO_GET_STATUS:
        {
            struct vdec_status_info cur_status;
            struct vdec_information *pvdec_stat = (struct vdec_information *)param;

            memset(&cur_status, 0, sizeof(cur_status));
            ret_code = vdec_io_control(info->cur_dev, cmd, (UINT32)(&cur_status));
            if(ret_code == RET_SUCCESS)
            {
                pvdec_stat->status = (cur_status.u_cur_status == VDEC27_STOPPED) ? VDEC_STOPPED : VDEC_STARTED;
                pvdec_stat->first_header_parsed = cur_status.b_first_header_got;
                pvdec_stat->first_pic_showed    = cur_status.u_first_pic_showed;
                pvdec_stat->first_pic_decoded   = cur_status.first_pic_decoded;
                pvdec_stat->pic_width        = cur_status.pic_width;
                pvdec_stat->pic_height       = cur_status.pic_height;
				pvdec_stat->sar_width        = cur_status.sar_width;
				pvdec_stat->sar_height       = cur_status.sar_height;
                pvdec_stat->aspect_ratio     = cur_status.aspect_ratio;
				pvdec_stat->active_format    = cur_status.active_format;
                pvdec_stat->frames_displayed = cur_status.display_idx;
                pvdec_stat->frames_decoded   = cur_status.frames_decoded;
                pvdec_stat->frame_last_pts   = cur_status.frame_last_pts;
                pvdec_stat->show_frame   = cur_status.display_frm;
                pvdec_stat->queue_count  = cur_status.top_cnt;
                pvdec_stat->buffer_size  = cur_status.vbv_size;
                pvdec_stat->buffer_used  = cur_status.valid_size;
                pvdec_stat->frame_rate   = cur_status.frame_rate;
                pvdec_stat->interlaced_frame = cur_status.progressive ? 0 : 1;
                pvdec_stat->top_field_first  = cur_status.top_field_first;
                pvdec_stat->hw_dec_error = cur_status.hw_dec_error;
                pvdec_stat->is_support   = cur_status.is_support;
                pvdec_stat->layer        = cur_status.layer;
                pvdec_stat->ff_mode      = cur_status.ff_mode;
                pvdec_stat->rect_switch_done = cur_status.rect_switch_done;
                pvdec_stat->playback_param.direction     = cur_status.play_direction;
                pvdec_stat->playback_param.rate          = cur_status.play_speed;
                pvdec_stat->api_playback_param.direction = cur_status.api_play_direction;
                pvdec_stat->api_playback_param.rate      = cur_status.api_play_speed;
				pvdec_stat->max_width  = cur_status.max_width;
				pvdec_stat->max_height = cur_status.max_height;
				pvdec_stat->max_frame_rate = cur_status.max_frame_rate;

                if(cur_status.output_mode == MP_MODE)
                {
                    pvdec_stat->output_mode = VDEC_FULL_VIEW;
                }
                else if(cur_status.output_mode == PREVIEW_MODE)
                {
                    pvdec_stat->output_mode = VDEC_PREVIEW;
                }
            }
            break;
        }

        case VDEC_IO_CAPTURE_DISPLAYING_FRAME:
            ret_code = vdec_capture_frame(info, (struct vdec_picture *)param);
            break;

		case VDEC_IO_SET_DISPLAY_MODE:
			ret_code = vdec_display_mode(info, (struct vdec_display_param *)param);
			break;

        default:
            ret_code = vdec_io_control(info->cur_dev, cmd, param);
            break;
    }

    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    return res;
}

INT32 hld_decv_mp_init(struct ali_video_info *info, struct vdec_mp_init_param *init_param)
{
    vdec_init vdec_init;
	struct vdec_io_reg_callback_para cb_param;
    RET_CODE ret_code = RET_SUCCESS;
    UINT32 codec_id = 0;
    INT32 res = -1;

    if((info == NULL) || (init_param == NULL))
    {
        return res;
    }
    codec_id = init_param->codec_tag;
    memset(&vdec_init, 0, sizeof(vdec_init));

    vdec_init.encrypt_mode  = init_param->encrypt_mode;
    vdec_init.decode_mode   = init_param->decode_mode;
    vdec_init.decoder_flag  = init_param->decoder_flag;
    vdec_init.preview       = init_param->preview;
    vdec_init.pfrm_buf      = (vdec_fbconfig *)(init_param->dec_buf_addr);
    vdec_init.phw_mem_cfg   = (vdec_hwmem_config *)(init_param->dec_buf_size);
    vdec_init.dec_buf_addr  = info->mem_addr;
    vdec_init.dec_buf_size  = info->mem_size;
    vdec_init.priv_buf_addr = info->priv_mem_addr;
    vdec_init.priv_buf_size = info->priv_mem_size;
    vdec_init.vbv_buf_addr  = info->vbv_mem_addr;
    vdec_init.vbv_buf_size  = info->vbv_mem_size;
    vdec_init.vdec_id       = info->index;
    vdec_init.fmt_in.fourcc         = init_param->codec_tag;
    vdec_init.fmt_in.frame_rate     = init_param->frame_rate;
    vdec_init.fmt_in.pic_width      = init_param->pic_width;
    vdec_init.fmt_in.pic_height     = init_param->pic_height;
    vdec_init.fmt_in.pixel_aspect_x = init_param->pixel_aspect_x;
    vdec_init.fmt_in.pixel_aspect_y = init_param->pixel_aspect_y;
    memcpy(&(vdec_init.fmt_out), &(vdec_init.fmt_in), sizeof(vdec_init.fmt_in));

    if(info->logo_mode == 0) {
        /* just tell see to use decore callback */
        vdec_init.pfn_decore_de_request = (void *)0x1;
        vdec_init.pfn_decore_de_release = (void *)0x1;
    }

    if((codec_id == wvc1) || (codec_id == wx3))
    {
        codec_id = vc1;
    }

    ret_code = vdec_decore_ioctl((void *)&codec_id, VDEC_CMD_INIT, (void *)&info->codec_handle, (void*)&vdec_init);
    if(ret_code == RET_SUCCESS)
    {
        info->codec_tag = codec_id;
        info->work_mode = 1;
        info->pause_decode  = 0;
        info->pause_display = 0;
        res = 0;

        switch(info->codec_tag)
        {
            case mpg2:
                info->vdec_type = MPEG2_DECODER;
                break;
            case h264:
                info->vdec_type = H264_DECODER;
                break;
            case hevc:
                info->vdec_type = H265_DECODER;
                break;
            case vc1:
                info->vdec_type = VC1_DECODER;
                break;
            case xvid:
                info->vdec_type = MPEG4_DECODER;
                break;
            case vp8:
                info->vdec_type = VP8_DECODER;
                break;
            case rmvb:
                info->vdec_type = RV_DECODER;
                break;
            case mjpg:
                info->vdec_type = MJPG_DECODER;
                break;
            default:
                break;
        }

        if (info->debug_on) {
            cb_param.monitor_rate = 0;
            cb_param.p_cb = (vdec_cbfunc)1;
			cb_param.e_cbtype = VDEC_CB_FIRST_I_DECODED;
			vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
			cb_param.e_cbtype = VDEC_CB_FIRST_SHOWED;
			vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
			cb_param.e_cbtype = VDEC_CB_ERROR;
			vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
			cb_param.e_cbtype = VDEC_CB_INFO_CHANGE;
			vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
			cb_param.e_cbtype = VDEC_CB_STATE_CHANGED;
			vdec_decore_ioctl(info->codec_handle, VDEC_CMD_REG_CB, (void *)(&cb_param), NULL);
        }
    }

    VDEC_PRINTF(info->index, "video %c%c%c%c init resolution <%ldx%ld> frame rate %lu aspect <%ldx%ld> ret %ld\n",
        (UINT8)init_param->codec_tag, (UINT8)(init_param->codec_tag>>8),
        (UINT8)(init_param->codec_tag>>16), (UINT8)(init_param->codec_tag>>24),
        init_param->pic_width, init_param->pic_height, init_param->frame_rate,
        init_param->pixel_aspect_x, init_param->pixel_aspect_y, res);

    VDEC_PRINTF(info->index, "video buffer: fb 0x%lx %lu priv 0x%lx %lu vbv 0x%lx %lu\n",
        vdec_init.dec_buf_addr, vdec_init.dec_buf_size, vdec_init.priv_buf_addr,
        vdec_init.priv_buf_size, vdec_init.vbv_buf_addr, vdec_init.vbv_buf_size);

    return res;
}

INT32 hld_decv_mp_rls(struct ali_video_info *info, struct vdec_mp_rls_param *rls_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (rls_param == NULL))
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CMD_RELEASE, NULL, NULL);
    vdec_get_cur_decoder(info->index, &info->vdec_type);
    hld_decv_set_cur_dev(info, info->vdec_type);
    info->codec_tag = 0;
    info->work_mode = 0;
    info->pause_decode  = 0;
    info->pause_display = 0;
    info->codec_handle = NULL;

    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video release %ld\n", res);

    return res;
}

INT32 hld_decv_mp_flush(struct ali_video_info *info, struct vdec_mp_flush_param *flush_param)
{
    struct output_frm_manager output_frm_info;
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (flush_param == NULL))
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CMD_SW_RESET,
		(void*)flush_param->flush_flag, (void*)&output_frm_info);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video flush %lu ret %ld\n", flush_param->flush_flag, res);

    return res;
}

INT32 hld_decv_mp_extra_data(struct ali_video_info *info, struct vdec_mp_extra_data *extra_data_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (extra_data_param == NULL))
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CMD_EXTRA_DADA, \
                                 (void*)extra_data_param->extra_data, (void*)extra_data_param->extra_data_size);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video decode extra data %ld\n", res);

    return res;
}

INT32 hld_decv_mp_get_status(struct ali_video_info *info, struct vdec_decore_status *decore_status,
    struct vdec_information *vdec_stat)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (decore_status == NULL))
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CMD_GET_STATUS, (void*)decore_status, NULL);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    if (res == 0) {
        if(decore_status->output_mode == MP_MODE)
            decore_status->output_mode = VDEC_FULL_VIEW;
        else if(decore_status->output_mode == PREVIEW_MODE)
            decore_status->output_mode = VDEC_PREVIEW;

        if (vdec_stat != NULL) {
            vdec_stat->status = info->pause_decode ? VDEC_PAUSED : VDEC_STARTED;
            vdec_stat->first_header_parsed = decore_status->first_header_got;
            vdec_stat->first_pic_decoded = decore_status->first_pic_decoded;
            vdec_stat->first_pic_showed = decore_status->first_pic_showed;
            vdec_stat->pic_width = decore_status->pic_width;
            vdec_stat->pic_height = decore_status->pic_height;
            vdec_stat->aspect_ratio = SAR;
            vdec_stat->frames_displayed = decore_status->frames_displayed;
            vdec_stat->frames_decoded = decore_status->frames_decoded;
            vdec_stat->frame_last_pts = decore_status->frame_last_pts;
            vdec_stat->show_frame = info->pause_display;
            vdec_stat->buffer_size = decore_status->buffer_size;
            vdec_stat->buffer_used = decore_status->buffer_used;
            vdec_stat->frame_rate = decore_status->frame_rate;
            vdec_stat->interlaced_frame = decore_status->interlaced_frame;
            vdec_stat->top_field_first = decore_status->top_field_first;
            vdec_stat->is_support = decore_status->decode_error ? 1 : 0;
            vdec_stat->output_mode = decore_status->output_mode;
            vdec_stat->sar_width = decore_status->sar_width;
            vdec_stat->sar_height = decore_status->sar_height;
            vdec_stat->layer = decore_status->layer;
            vdec_stat->hw_dec_error = decore_status->decode_error;
            vdec_stat->rect_switch_done = decore_status->rect_switch_done;
        }
    }

    return res;
}

INT32 hld_decv_mp_pause(struct ali_video_info *info, struct vdec_mp_pause_param *pause_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 pause_deocde = -1;
    INT32 pause_display = -1;
    INT32 res = -1;

    if((info == NULL) || (pause_param == NULL))
    {
        return res;
    }

    pause_deocde  = (pause_param->pause_decode == 0xFF) ? -1 : pause_param->pause_decode;
    pause_display = (pause_param->pause_display == 0xFF) ? -1 : pause_param->pause_display;
    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CMD_PAUSE_DECODE, \
                                 (void*)pause_deocde, (void*)pause_display);
    if(ret_code == RET_SUCCESS)
    {
        if(pause_deocde > 0)
        {
            info->pause_decode = 1;
        }
        else if(pause_deocde == 0)
        {
            info->pause_decode = 0;
        }

        if(pause_display > 0)
        {
            info->pause_display = 1;
        }
        else if(pause_display == 0)
        {
            info->pause_display = 0;
        }
    }

    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video pause <%u %u> ret %ld\n", pause_param->pause_decode, pause_param->pause_display, res);

    return res;
}

INT32 hld_decv_mp_set_sbm(struct ali_video_info *info, struct vdec_mp_sbm_param *sbm_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    UINT32 decode_input = 0;
    UINT32 decode_output = (sbm_param->decode_output == 0xFF) ? -1 : sbm_param->decode_output;
    UINT32 display_input = (sbm_param->display_input == 0xFF) ? -1 : sbm_param->display_input;
    INT32 res = -1;

    if((info == NULL) || (sbm_param == NULL))
    {
        return res;
    }

    decode_input = ((sbm_param->packet_header<<16) | sbm_param->packet_data);
    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_VIDEO_SBM_BUF, (void*)decode_input, (void*)decode_output);
    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_DISPLAY_SBM_BUF, (void*)display_input, NULL);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set sbm <%u %u %u %u> ret %ld\n",
        sbm_param->packet_header, sbm_param->packet_data,
        sbm_param->decode_output, sbm_param->display_input, res);

    return res;
}

INT32 hld_decv_mp_set_sync_mode(struct ali_video_info *info, struct vdec_mp_sync_param *sync_param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if((info == NULL) || (sync_param == NULL))
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_SYNC_MODE, \
                                 (void*)&sync_param->sync_mode, (void*)&sync_param->sync_unit);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set sync mode <%u %u> ret %ld\n", sync_param->sync_mode, sync_param->sync_unit, res);

    return res;
}

INT32 hld_decv_mp_set_display_rect(struct ali_video_info *info, struct vdec_display_rect *display_rect)
{
    RET_CODE ret_code = RET_SUCCESS;
    struct Video_Rect src_rect;
    struct Video_Rect dst_rect;
    INT32 res = -1;

    if((info == NULL) || (display_rect == NULL))
    {
        return res;
    }

    src_rect.x = display_rect->src_x;
    src_rect.y = display_rect->src_y;
    src_rect.w = display_rect->src_w;
    src_rect.h = display_rect->src_h;
    dst_rect.x = display_rect->dst_x;
    dst_rect.y = display_rect->dst_y;
    dst_rect.w = display_rect->dst_w;
    dst_rect.h = display_rect->dst_h;

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_DISPLAY_RECT, (void*)&src_rect, (void*)&dst_rect);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set display rect <%ld %ld %ld %ld> => <%ld %ld %ld %ld> ret %ld\n",
        src_rect.x, src_rect.y, src_rect.w, src_rect.h,
        dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h, res);

    return res;
}

INT32 hld_decv_mp_set_quick_mode(struct ali_video_info *info, UINT32 quick_mode)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_QUICK_MODE, (void*)&quick_mode, NULL);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set quick mode %lu ret %ld\n", quick_mode, res);

    return res;
}

INT32 hld_decv_mp_capture_frame(struct ali_video_info *info, struct vdec_picture *picture)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_capture_frame(info, picture);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    return res;
}

INT32 hld_decv_mp_set_dec_frm_type(struct ali_video_info *info, UINT32 frm_type)
{
    RET_CODE ret_code = RET_SUCCESS;
    UINT32 decode_mode = frm_type;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    ret_code = vdec_decore_ioctl(info->codec_handle, VDEC_CFG_DECODE_MODE, (void*)&decode_mode, NULL);
    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    VDEC_PRINTF(info->index, "video set frame type %lu ret %ld\n", decode_mode, res);

    return res;
}

INT32 hld_decv_mp_ioctl(struct ali_video_info *info, UINT32 cmd, UINT32 param)
{
    RET_CODE ret_code = RET_SUCCESS;
    INT32 res = -1;

    if(info == NULL)
    {
        return res;
    }

    switch(cmd)
    {
        case VDEC_DYNAMIC_FRAME_ALLOC:
            ret_code = vdec_decore_ioctl(info->codec_handle, cmd, (void *)&param, NULL);
            VDEC_PRINTF(info->index, "video dynamic frame alloc %lu ret %ld\n", param, ret_code);
            break;

        case VDEC_STEP_DISPLAY:
            ret_code = vdec_decore_ioctl(info->codec_handle, cmd, (void *)&param, NULL);
            VDEC_PRINTF(info->index, "video step display %lu ret %ld\n", param, ret_code);
            break;

		case VDEC_CMD_REG_CB:
			ret_code = vdec_decore_ioctl(info->codec_handle, cmd, (void *)param, NULL);
			break;

        case VDEC_SET_PIP_PARAM:
            ret_code = vdec_decore_ioctl(info->codec_handle, cmd, (void *)param, NULL);
            break;
        default:
            break;
    }

    res = ((ret_code == RET_SUCCESS) ? 0 : -1);

    return res;
}

INT32 hld_decv_logo_init(struct ali_video_info *info)
{
    struct vdec_mp_init_param init_param;
    struct vdec_mp_sbm_param sbm_param;
    struct sbm_config sbm_cfg;
    INT32 res = 0;

    if (info->sbm_file != NULL) {
        return 0;
    }

    info->sbm_file = filp_open("/dev/ali_sbm0", O_WRONLY, 0);
    if (info->sbm_file == NULL) {
        VDEC_PRINTF(info->index, "Open SBM device fail");
        return -1;
    }

    memset(&sbm_cfg, 0, sizeof(sbm_cfg));
    sbm_cfg.buffer_addr = (info->mp_mem_addr & 0x1FFFFFFF);
    sbm_cfg.buffer_size = info->mp_mem_size;
    sbm_cfg.reserve_size = 8;
    sbm_cfg.wrap_mode = SBM_MODE_PACKET;
    sbm_cfg.lock_mode = SBM_SPIN_LOCK;
    if (info->sbm_file->f_op->unlocked_ioctl) {
        res = info->sbm_file->f_op->unlocked_ioctl(info->sbm_file, SBMIO_CREATE_SBM, (UINT32)&sbm_cfg);
    } else {
        VDEC_PRINTF(info->index, "Create SBM fail\n");
        goto fail;
    }

    info->logo_mode = 1;

    memset(&init_param, 0, sizeof(init_param));
    init_param.codec_tag = (info->vdec_type == H264_DECODER) ? h264 : mpg2;
    init_param.decode_mode = VDEC_MODE_SBM_STREAM;
    res = hld_decv_mp_init(info, &init_param);
    if (res != 0) {
        goto fail;
    }

    sbm_param.packet_header = 0;
    sbm_param.packet_data   = 0;
    sbm_param.decode_output = 0xFF;
    sbm_param.display_input = 0xFF;
    res = hld_decv_mp_set_sbm(info, &sbm_param);
	//res = hld_decv_mp_ioctl(info, VDEC_DYNAMIC_FRAME_ALLOC, 1);

    info->write_header = 1;
    VDEC_PRINTF(info->index, "video %d logo mode initialize ok\n", info->vdec_type);

    return 0;

fail:
    filp_close(info->sbm_file, 0);
    info->logo_mode = 0;
    VDEC_PRINTF(info->index, "video %d logo initialize fail\n", info->vdec_type);
    return -1;
}

INT32 hld_decv_logo_rls(struct ali_video_info *info)
{
    struct vdec_mp_rls_param rls_param;
    INT32 res = 0;

    memset(&rls_param, 0, sizeof(rls_param));
    res = hld_decv_mp_rls(info, &rls_param);

    if(info->sbm_file) {
        if (info->sbm_file->f_op->unlocked_ioctl) {
            res = info->sbm_file->f_op->unlocked_ioctl(info->sbm_file, SBMIO_DESTROY_SBM, 0);
        }

        res = filp_close(info->sbm_file, 0);
        info->sbm_file = NULL;
    }

    info->logo_mode = 0;
    VDEC_PRINTF(info->index, "video %d logo mode release ok\n", info->vdec_type);

    return res;
}
