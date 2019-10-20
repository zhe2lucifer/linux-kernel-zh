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
 
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
//#include <linux/smp_lock.h>
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

#include <rpc_hld/ali_rpc_hld_pvr.h>

#include "../ali_rpc.h"

/*********************************************/
static UINT32 decrypt_control[] =
{
	//desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct PVR_BLOCK_ENC_PARAM)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};
static UINT32 desc1[] =
{
	//desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct PVR_RPC_RAW_DECRYPT)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};
static UINT32 desc2[] = {	2, DESC_OUTPUT_STRU(0, sizeof(DEEN_CONFIG)), DESC_OUTPUT_STRU(1, sizeof(PVR_REC_VIDEO_PARAM)), \
					2, DESC_P_PARA(0, 0, 0), DESC_P_PARA(1, 4, 1), \
					0, 0,};

static UINT32 desc3[] =
{
	//desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(PVR_decrypt_evo)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};


static UINT32 desc4[] =
{
	//desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(PVR_KEY_PARAM)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};



RET_CODE pvr_block_de_encrypt( DEEN_CONFIG *p_de_en, UINT8 *input, UINT8 *output, UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param)
{
    jump_to_func(NULL, ali_rpc_call, p_de_en, (HLD_PVR_MODULE<<24) | (5<<16) | FUNC_PVR_RPC_DECRYPT, desc2);
}

RET_CODE pvr_raw_decrypt_main(UINT32 cmd, PVR_RPC_RAW_DECRYPT *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, desc1);
}

RET_CODE pvr_update_paras(UINT32 cmd, PVR_BLOCK_ENC_PARAM *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, decrypt_control);
}

RET_CODE pvr_set_block_size(UINT32 cmd,UINT32 block_size)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, NULL);
}

RET_CODE pvr_free_resources(UINT32 cmd,PVR_RPC_RAW_DECRYPT *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, desc1);
}

RET_CODE pvr_start_block_rec(UINT32 cmd, PVR_BLOCK_ENC_PARAM *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, decrypt_control);
}

RET_CODE pvr_decrytp_raw_evo(UINT32 cmd, PVR_decrypt_evo *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, desc3);
}

RET_CODE pvr_capture_pvr_key(UINT32 cmd, PVR_KEY_PARAM *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, desc4);
}

RET_CODE pvr_release_pvr_key(UINT32 cmd, UINT32 stream_id)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, NULL);
}

RET_CODE pvr_playback_set_key(UINT32 cmd,PVR_BLOCK_ENC_PARAM *param)
{
	jump_to_func(NULL, ali_rpc_call, cmd, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, decrypt_control);
}

RET_CODE pvr_decrytp_raw_evo_sub(UINT32 cmd, PVR_decrypt_evo *param)
{

	jump_to_func(&ret, ali_rpc_call, pars, (HLD_PVR_MODULE<<24) | (2<<16) | FUNC_PVR_RPC_IOCTL, desc3);
}


