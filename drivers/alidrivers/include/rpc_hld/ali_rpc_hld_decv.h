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
 
#ifndef __DRIVERS_ALI_RPC_HLD_DECV_H
#define __DRIVERS_ALI_RPC_HLD_DECV_H

#include "ali_rpc_hld.h"

enum LLD_DECV_M36_FUNC
{
    FUNC_VDEC_M36_ATTACH = 0,
    FUNC_VDEC_S3601_DE_RELEASE,
    FUNC_VDEC_S3601_DE_REQUEST,
    FUNC_VDEC_ENABLE_ADVANCE_PLAY,
    FUNC_VDEC_DISABLE_DVIEW,
};

enum LLD_DECV_AVC_FUNC
{
    FUNC_VDEC_AVC_ATTACH = 0,
};

enum LLD_DECV_HEVC_FUNC
{
    FUNC_VDEC_HEVC_ATTACH = 0,
};

enum LLD_DECV_AVS_FUNC
{
    FUNC_VDEC_AVS_ATTACH = 0,
};

enum HLD_DECV_FUNC
{
    FUNC_VDEC_OPEN = 0,
    FUNC_VDEC_CLOSE,
    FUNC_VDEC_START,
    FUNC_VDEC_STOP,
    FUNC_VDEC_VBV_REQUEST,
    FUNC_VDEC_VBV_UPDATE,
    FUNC_VDEC_SET_OUTPUT,
    FUNC_VDEC_SYNC_MODE,
    FUNC_VDEC_PROFILE_LEVEL,
    FUNC_VDEC_IO_CONTROL,
    FUNC_VDEC_PLAYMODE,
    FUNC_VDEC_DVR_SET_PARAM,
    FUNC_VDEC_DVR_PAUSE,
    FUNC_VDEC_DVR_RESUME,
    FUNC_VDEC_STEP,
    FUNC_H264_DECODER_SELECT,
    FUNC_GET_SELECTED_DECODER,
    FUNC_IS_CUR_DECODER_AVC,
    FUNC_SET_AVC_OUTPUT_MODE_CHECK_CB,
    FUNC_VIDEO_DECODER_SELECT,
    FUNC_GET_CURRENT_DECODER,
    FUNC_VDEC_COPY_DATA,
    FUNC_VDEC_DECORE_IOCTL,
    FUNC_VDEC_SELECT_DECODER,
    FUNC_VDEC_GET_CUR_DECODER,
    FUNC_VDEC_GET_CUR_DEV_NAME,
};

#endif
