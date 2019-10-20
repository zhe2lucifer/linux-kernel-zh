/*
 * ALi CRC32 driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <ali_cache.h>
#include "ali_crc.h"

#define SCR_NPARA(x) ((HLD_SCR_MODULE << 24)|(x << 16))

int _crc_api_attach(struct ali_crc_dev_tag *crc)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		SCR_NPARA(0)|ALIRPC_RPC_scr_api_attach, \
		NULL);
}

void _crc_api_detach(struct ali_crc_dev_tag *crc)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		SCR_NPARA(0)|ALIRPC_RPC_scr_api_detach, \
		NULL);
}

int _crc_session_create_rpc(unsigned int *sess_id,
	struct scr_sess_create *param)
{
	u32 crc_sess_create_desc[] =
	{ //desc of pointer para
		2, DESC_OUTPUT_STRU(0, sizeof(unsigned int)), DESC_STATIC_STRU(0, sizeof(struct scr_sess_create)),
		2, DESC_P_PARA(0, 0, 0), DESC_P_PARA(0, 1, 1),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_create, \
		crc_sess_create_desc);
}

int _crc_session_create(struct ali_crc_dev_tag *crc,
	unsigned int *sess_id, struct scr_sess_create *param)
{
	dev_dbg(&crc->clnt->dev, "_crc_session_create[%p][%p], sid[%x]\n",
		sess_id, param, *sess_id);

	return _crc_session_create_rpc(sess_id, param);
}

int _crc_session_delete(struct ali_crc_dev_tag *crc,
	unsigned int sess_id)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(1)|ALIRPC_RPC_scr_session_delete, \
		NULL);
}

int _crc_update(struct ali_crc_dev_tag *crc,
	unsigned int sess_id, struct scr_sess_dio_crc32 *crc32)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_crc32_update, \
		NULL);
}

int _crc_final(struct ali_crc_dev_tag *crc,
	unsigned int sess_id, unsigned int *rc)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_crc32_final, \
		NULL);
}

int _crc_create_sbm_task(struct ali_crc_dev_tag *crc,
	UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
		SCR_NPARA(1)|ALIRPC_RPC_crc32_create_sbm_task, \
		NULL);
}

int _crc_delete_sbm_task(struct ali_crc_dev_tag *crc,
	UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
		SCR_NPARA(1)|ALIRPC_RPC_crc32_delete_sbm_task, \
		NULL);
}
