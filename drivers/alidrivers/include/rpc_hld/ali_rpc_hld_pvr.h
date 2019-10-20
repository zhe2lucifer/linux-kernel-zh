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
 
#ifndef __DRIVERS_ALI_RPC_HLD_SND_H
#define __DRIVERS_ALI_RPC_HLD_SND_H

#include <rpc_hld/ali_rpc_hld.h>            // To include some basic type definition
#include <alidefinition/adf_pvr.h>
#include <alidefinition/adf_dsc.h>


enum HLD_PVR_FUNC
{
	FUNC_PVR_RPC_IOCTL = 0,
        
	FUNC_PVR_RPC_DECRYPT = 1,
	/* ... */
};

#define PVR_NPARA(x) ((HLD_PVR_MODULE << 24)|(x << 16))

RET_CODE pvr_block_de_encrypt( DEEN_CONFIG *p_de_en, UINT8 *input, UINT8 *output, UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param);
RET_CODE pvr_raw_decrypt_main(UINT32 cmd, PVR_RPC_RAW_DECRYPT *param);
RET_CODE pvr_update_paras(UINT32 cmd, PVR_BLOCK_ENC_PARAM *param);
RET_CODE pvr_set_block_size(UINT32 cmd,UINT32 block_size);
RET_CODE pvr_free_resources(UINT32 cmd,PVR_RPC_RAW_DECRYPT *param);
RET_CODE pvr_start_block_rec(UINT32 cmd, PVR_BLOCK_ENC_PARAM *param);
RET_CODE pvr_decrytp_raw_evo(UINT32 cmd, PVR_decrypt_evo *param);
RET_CODE pvr_capture_pvr_key(UINT32 cmd, PVR_KEY_PARAM *param);
RET_CODE pvr_release_pvr_key(UINT32 cmd, UINT32 stream_id);
RET_CODE pvr_playback_set_key(UINT32 cmd,PVR_BLOCK_ENC_PARAM *param);
RET_CODE pvr_decrytp_raw_evo_sub(UINT32 cmd, PVR_decrypt_evo *param);

#endif
