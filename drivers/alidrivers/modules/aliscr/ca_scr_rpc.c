/*
 * Scramber Core driver
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

#include "ca_scr_priv.h"

#define SCR_NPARA(x) ((HLD_SCR_MODULE << 24)|(x << 16))

int ali_scr_umemcpy(void *dest,
	const void *src, __u32 n)
{
	int ret = 0;
	int sflag = access_ok(VERIFY_READ,
		(void __user *)src, n);
	int dflag = access_ok(VERIFY_WRITE,
		(void __user *)dest, n);

	if (segment_eq(get_fs(), USER_DS)) {
		if (sflag && !dflag)
			ret = copy_from_user(dest,
				(void __user *)src, n);
		else if (dflag && !sflag)
			ret = copy_to_user(dest, src, n);
		else if (!sflag && !dflag)
			memcpy(dest, src, n);
		else
			return -1;
	} else {
		memcpy(dest, src, n);
	}

	return ret;
}


void _scr_api_attach(struct ca_scr_dev *scr)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		SCR_NPARA(0)|ALIRPC_RPC_scr_api_attach, \
		NULL);
}

void _scr_api_detach(struct ca_scr_dev *scr)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		SCR_NPARA(0)|ALIRPC_RPC_scr_api_detach, \
		NULL);
}

int _scr_create_sbm_task(struct ca_scr_dev *scr,
	UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
		SCR_NPARA(1)|ALIRPC_RPC_scr_create_sbm_task, \
		NULL);
}

int _scr_delete_sbm_task(struct ca_scr_dev *scr,
	UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
		SCR_NPARA(1)|ALIRPC_RPC_scr_delete_sbm_task, \
		NULL);
}

RET_CODE _scr_api_ioctl(struct ca_scr_dev *scr,
	struct scr_dev *p_scr_dev,
	unsigned int cmd, unsigned int param)
{
	jump_to_func(NULL, ali_rpc_call, p_scr_dev, \
		SCR_NPARA(3)|ALIRPC_RPC_scr_api_ioctl, \
		NULL);
}

RET_CODE _scr_session_create(struct ca_scr_dev *scr,
	unsigned int *sess_id, struct scr_sess_create *param)
{
	dev_dbg(scr->dev, "_scr_session_create[%p][%p], sid[%x]\n",
		sess_id, param, *sess_id);

	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_create, \
		NULL);
}

RET_CODE _scr_session_delete(struct ca_scr_dev *scr,
	unsigned int sess_id)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(1)|ALIRPC_RPC_scr_session_delete, \
		NULL);
}

RET_CODE _scr_session_add_key(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_key *user_key)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_add_key, \
		NULL);
}

RET_CODE _scr_session_del_key(struct ca_scr_dev *scr,
	unsigned int sess_id, unsigned int key_handle)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_del_key, \
		NULL);
}

RET_CODE _scr_session_update_key(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_update_info *user_up)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_update_key, \
		NULL);
}

RET_CODE _scr_session_add_pid(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_pid *pPid)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_add_pid, \
		NULL);
}

RET_CODE _scr_session_del_pid(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_pid *pPid)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_del_pid, \
		NULL);
}

RET_CODE _scr_session_contns_renew(struct ca_scr_dev *scr,
	unsigned int sess_id)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(1)|ALIRPC_RPC_scr_session_contns_renew, \
		NULL);
}

RET_CODE _scr_session_crypto(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_sess_dio_crypto *crypto)
{
	jump_to_func(NULL, ali_rpc_call, sess_id, \
		SCR_NPARA(2)|ALIRPC_RPC_scr_session_crypto, \
		NULL);
}
