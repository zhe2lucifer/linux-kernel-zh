/*
 * RPC interface for CF
 *
 * Copyright (c) 2016 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>

#include "ali_cf_rpc.h"
#include "ali_cf_priv.h"

static int cf_see_open(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL,
		CF_NPARA(0) | FUNC_CF_OPEN, NULL);
}

static int cf_see_close(int chan)
{
	jump_to_func(NULL, ali_rpc_call, chan,
		CF_NPARA(1) | FUNC_CF_CLOSE, NULL);
}

static int cf_see_ioctl(int chan, int cmd, void *param)
{
	__u32 desc[] = { //desc of pointer para
		1, DESC_STATIC_STRU(0, 0),
		1, DESC_P_PARA(0, 2, 0), 
		//desc of pointer ret
		0,                          
		0,
	};

	switch (cmd) {
	case ALI_CF_IO_VERSION_INFO:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_version));
		break;
	case ALI_CF_IO_FEATURE_VECTOR:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_feature));
		break;
	case ALI_CF_IO_CF_STATUS:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_cf_status));
		break;
	case ALI_CF_IO_TRANS_STATUS:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_trans_status));
		break;
	case ALI_CF_IO_DECM_STATUS:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_decm_status));
		break;
	case ALI_CF_IO_ISSUE_OP_FEATURE:
	case ALI_CF_IO_ISSUE_OP_DIFF:
	case ALI_CF_IO_ISSUE_OP_CWCORSHV:
		DESC_STATIC_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_operation));
		break;
	case ALI_CF_IO_READ_OP_RESULT:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_result));
		break;
	case ALI_CF_IO_WAIT_OP_DONE:
		DESC_OUTPUT_STRU_SET_SIZE(desc, 0,
				sizeof(struct cf_trans_status));
		break;
	default:
		return -ENOTTY;
	}

	jump_to_func(NULL, ali_rpc_call, chan,
		CF_NPARA(3) | FUNC_CF_IOCTL, desc);
}

static int cf_see_write(int chan, void *buf, int count)
{
	__u32 desc[] = { //desc of pointer para
		1, DESC_STATIC_STRU(0, sizeof(struct cf_operation)),
		1, DESC_P_PARA(0, 1, 0), 
		//desc of pointer ret
		0,                          
		0,
	};

	jump_to_func(NULL, ali_rpc_call, chan,
		CF_NPARA(3) | FUNC_CF_WRITE, desc);
}

static int cf_see_read(int chan, void *buf, int count)
{
	__u32 desc[] = { //desc of pointer para
		1, DESC_OUTPUT_STRU(0, sizeof(struct cf_result)),
		1, DESC_P_PARA(0, 1, 0), 
		//desc of pointer ret
		0,                          
		0,
	};

	jump_to_func(NULL, ali_rpc_call, chan,
		CF_NPARA(3) | FUNC_CF_READ, desc);
}

static int cf_see_set_target(int pos)
{
	jump_to_func(NULL, ali_rpc_call, pos,
		CF_NPARA(1) | FUNC_CF_SET_TARGET, NULL);
}


static const struct cf_see_ops cf_see_rpc_ops = {
	.open = cf_see_open,
	.close = cf_see_close,
	.ioctl = cf_see_ioctl,
	.write = cf_see_write,
	.read = cf_see_read,
	.set_target = cf_see_set_target,
};

int cf_see_register(void *data)
{
	struct cf_drv *drv = data;

	drv->see_ops = &cf_see_rpc_ops;

	jump_to_func(NULL, ali_rpc_call, NULL,
		CF_NPARA(0) | FUNC_CF_ATTACH, NULL);
}

int cf_see_unregister(void *data)
{
	struct cf_drv *drv = data;

	drv->see_ops = NULL;
	jump_to_func(NULL, ali_rpc_call, NULL,
		CF_NPARA(0) | FUNC_CF_DETACH, NULL);
}

