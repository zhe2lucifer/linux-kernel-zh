/*
 * Security Hashing Algorithm driver
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
#include <linux/dma-mapping.h>

#include "ali_sha.h"

#define DSC_NPARA(x) ((HLD_DSC_MODULE << 24)|(x << 16))

u32 sha_io_control[] = {
	/* desc of pointer para */
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0),
	/* desc of pointer ret */
	0,
	0,
};

u32 sha_crypt_control[] = {
	/* desc of pointer para */
	1, DESC_OUTPUT_STRU(0, 64),
	1, DESC_P_PARA(0, 2, 0),
	/* desc of pointer ret */
	0,
	0,
};

static int ali_umemcpy(void *dest, const void *src, u32 n)
{
	int ret = 0;
	int sflag = access_ok(VERIFY_READ, (void __user *)src, n);
	int dflag = access_ok(VERIFY_WRITE, (void __user *)dest, n);

	if (segment_eq(get_fs(), USER_DS)) {
		if (sflag && !dflag)
			ret = copy_from_user(dest, (void __user *)src, n);
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

int see_sha_sbm_open(SHA_DEV *pShaDev, int sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, pShaDev,
		DSC_NPARA(2)|FUNC_SHA_CREATE_SBM_TASK, NULL);
}

int ali_sha_ioctl_rpc(SHA_DEV *pShaDev, u32 cmd, u32 param)
{
	u32 i;
	u32 common_desc[sizeof(sha_io_control)];
	u32 *desc = (u32 *)common_desc;
	u32 *b = (u32 *)sha_io_control;

	for (i = 0; i < sizeof(sha_io_control)/sizeof(u32); i++)
		desc[i] = b[i];

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(SHA_INIT_PARAM));
		break;

	default:
		return RET_FAILURE;
	}

	jump_to_func(NULL, ali_rpc_call, pShaDev,
		DSC_NPARA(3)|FUNC_SHA_IOCTL, desc);
}

int ali_sha_ioctl(SHA_DEV *pShaDev, u32 cmd, u32 param)
{
	SHA_INIT_PARAM sha_param;

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD):
		ali_umemcpy(&sha_param, (void *)param,
			sizeof(sha_param));
		break;

	default:
		pr_info("sha rpc error: invalid cmd %d\n", cmd);
		return RET_FAILURE;
	}

	return ali_sha_ioctl_rpc(pShaDev, cmd, (u32)&sha_param);
}

int ali_sbm_release_task(int sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id,
		DSC_NPARA(1)|FUNC_SHA_DELETE_SBM_TASK, NULL);
}

int ali_sha_digest_rpc(SHA_DEV *pShaDev, u8 *input,
		u8 *output, u32 data_length)
{
	jump_to_func(NULL, ali_rpc_call, pShaDev,
				DSC_NPARA(4)|FUNC_SHA_DIGEST,
				sha_crypt_control);
}

int ali_sha_digest(SHA_DEV *pShaDev,
	u8 *input, u8 *output, u32 data_len)
{
	int ret = -1;
	u8 hash_out[ALI_SHA_MAX_DIGEST_SIZE];

	memset(hash_out, 0, sizeof(hash_out));

	ret = ali_sha_digest_rpc(pShaDev, input, hash_out, data_len);
	if (!ret)
		memcpy(output, hash_out, sizeof(hash_out));

	return ret;
}

void ali_m36_sha_see_init(void)
{
	DSC_DEV *pDscDev = (DSC_DEV *)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_DSC);
	if (NULL == pDscDev) {
		jump_to_func(NULL, ali_rpc_call, NULL,
			DSC_NPARA(0)|FUNC_DSC_ATTACH, NULL);
	}
}

static u32 ali_sha_get_free_sub_device_id_rpc(u8 sub_mode)
{
	jump_to_func(NULL, ali_rpc_call, sub_mode,
		DSC_NPARA(1)|FUNC_GET_FREE_SUB_DEVICE_ID, NULL);
}

u32 ali_sha_get_free_sub_device_id(void)
{
	u8 sub_mode = SHA;

	return ali_sha_get_free_sub_device_id_rpc(sub_mode);
}

static int ali_sha_set_sub_device_id_idle_rpc(u8 sub_mode,
	u32 device_id)
{
	jump_to_func(NULL, ali_rpc_call, sub_mode,
		DSC_NPARA(2)|FUNC_SET_SUB_DEVICE_ID_IDLE, NULL);
}

int ali_sha_set_sub_device_id_idle(u32 device_id)
{
	u8 sub_mode = SHA;

	return ali_sha_set_sub_device_id_idle_rpc(sub_mode, device_id);
}
