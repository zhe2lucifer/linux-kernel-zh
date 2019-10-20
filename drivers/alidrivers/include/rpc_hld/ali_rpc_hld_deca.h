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

#ifndef __DRIVERS_ALI_RPC_HLD_DECA_H
#define __DRIVERS_ALI_RPC_HLD_DECA_H

#include <rpc_hld/ali_rpc_hld.h>            // To include some basic type defintion

#include <alidefinition/adf_deca.h>

enum LLD_DECA_M36_FUNC
{
    FUNC_DECA_M36_ATTACH = 0,
    FUNC_DECA_M36_DVR_ENABLE,
    FUNC_DECA_M36_EXT_DEC_ENABLE,
    FUNC_DECA_M36_INIT_TONE_VOICE,
};


enum HLD_DECA_FUNC
{
    FUNC_DECA_PCM_BUF_RESUME = 0,
    FUNC_DECA_OPEN,
    FUNC_DECA_CLOSE,
    FUNC_DECA_SET_SYNC_MODE,
    FUNC_DECA_START,
    FUNC_DECA_STOP,
    FUNC_DECA_PAUSE,
    FUNC_DECA_IO_CONTROL,
    FUNC_DECA_REQUEST_WRITE,
    FUNC_DECA_UPDATE_WRITE,
    FUNC_DECA_TONE_VOICE,
    FUNC_DECA_STOP_TONE_VOICE,
    FUNC_DECA_REQUEST_DESC_WRITE,
    FUNC_DECA_UPDATE_DESC_WRITE,
    FUNC_DECA_INIT_ASE_VOICE,
    FUNC_DECA_PROCESS_PCM_SAMPLES,
    FUNC_DECA_PROCESS_PCM_BITSTREAM,
    FUNC_DECA_COPY_DATA,
    FUNC_DECA_DECORE_IOCTL_L,
    FUNC_DECA_STANDBY,
    FUNC_DECA_SET_DBG_LEVEL,
    FUNC_DECA_SET_DD_PLUGIN_ADDR,
};

RET_CODE deca_request_write(void *dev,
                            UINT32 req_size,
                            void **ret_buf,
                            UINT32 *ret_buf_size,
                            struct control_block *ctrl_blk);
void deca_update_write(void *dev, UINT32 size);
void deca_process_pcm_samples(UINT32 pcm_bytes_len, UINT8 *pcm_raw_buf, UINT32 sample_rate,
                                                    UINT32 channel_num, UINT32 sample_precision);
void deca_process_pcm_bitstream(UINT32 pcm_bytes_len, UINT8 *pcm_raw_buf, UINT32 bs_length, UINT8 *un_processed_bs,
                                                    UINT32 sample_rate, UINT32 channel_num, UINT32 sample_precision);


#endif



