/*
 * Key Ladder Core driver
 * Copyright(C) 2014 ALi Corporation. All rights reserved.
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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <ali_cache.h>

#include "ca_kl_priv.h"

#define CE_NPARA(x) ((HLD_CRYPTO_MODULE<<24)|(x<<16))

static __u32 ce_io_control[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0), 
	//desc of pointer ret
	0,                          
	0,
};

int ali_ce_umemcpy(void *dest, const void *src, __u32 n)
{
	int ret = 0;
	int sflag = access_ok(VERIFY_READ,
		(void __user *)src, n);
	int dflag = access_ok(VERIFY_WRITE,
		(void __user *)dest, n);

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


int ali_ce_ioctl(CE_DEVICE *pCeDev, __u32 cmd, __u32 param)
{
	RET_CODE ret = RET_FAILURE;

	__u32 i;
	__u32 common_desc[sizeof(ce_io_control)];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)ce_io_control;

	for(i = 0; i < sizeof(ce_io_control)/sizeof(__u32); i++)
	{
		desc[i] = b[i];
	}

	switch (CE_IO_CMD(cmd)) {
	case CE_IO_CMD(IO_OTP_ROOT_KEY_GET):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(OTP_PARAM));
		break;

	case CE_IO_CMD(IO_CRYPT_DEBUG_GET_KEY):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(CE_DEBUG_KEY_INFO)); 
		break;

	case CE_IO_CMD(IO_CRYPT_POS_IS_OCCUPY):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(CE_POS_STS_PARAM)); 
		break;

	case CE_IO_CMD(IO_CRYPT_POS_SET_USED):
		desc = NULL;
		break;

	case CE_IO_CMD(IO_CRYPT_POS_SET_IDLE):
		desc = NULL;
		break;

	case CE_IO_CMD(IO_CRYPT_FOUND_FREE_POS):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, \
				sizeof(CE_FOUND_FREE_POS_PARAM));
		break;

	case CE_IO_CMD(IO_DECRYPT_PVR_USER_KEY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_PVR_KEY_PARAM));
		break;

	case CE_IO_CMD(IO_CRYPT_VSC_DECW_KEY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_DECW_KEY_PARAM));
		break;

	case CE_IO_CMD(IO_CRYPT_VSC_REC_EN_KEY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_REC_KEY_PARAM));
		break;

	case CE_IO_CMD(IO_CRYPT_VSC_UK_EN_KEY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_UK_KEY_PARAM));
		break;

	case CE_IO_CMD(IO_CRYPT_VSC_CW_DERIVE_CW):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_CW_DERIVATION));
		break;

	case CE_IO_CMD(IO_CRYPT_CW_DERIVE_CW):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_CW_DERIVATION));

		break;

	case CE_IO_CMD(IO_CRYPT_ETSI_CHALLENGE):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, \
			sizeof(CE_ETSI_CHALLENGE));
		break;

	case CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CE_NLEVEL_PARAM));
		break;
	default:
		return RET_FAILURE;
	}

	jump_to_func(NULL, ali_rpc_call, pCeDev, CE_NPARA(3)|FUNC_CE_IOCTL, desc);

	return ret;
}
EXPORT_SYMBOL(ali_ce_ioctl);

int ali_ce_generate_single_level_key(pCE_DEVICE pCeDev,
	pCE_DATA_INFO pCe_data_info)
{
	__u32 desc[] = 
	{ //desc of pointer para
		1, DESC_STATIC_STRU(0, sizeof(CE_DATA_INFO)),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,                          
		0,
	};
	jump_to_func(NULL, ali_rpc_call, pCeDev, CE_NPARA(2)|FUNC_CE_GENERATE_SINGLE_LEVEL_KEY, desc);
	
}

int ali_ce_generate_hdcp_key(pCE_DEVICE pCeDev, __u8 *en_hdcp_key, __u16 len)
{
	__u32 desc[] = 
	{ //desc of pointer para
		1, DESC_STATIC_STRU(0, 288),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,
		0,
	};
	jump_to_func(NULL, ali_rpc_call, pCeDev, CE_NPARA(3)|FUNC_CE_GENERATE_HDCP_KEY, desc);
	
}


void ali_m36_ce_see_init(void)
{
	pCE_DEVICE pCeDev = NULL;

	pCeDev = (pCE_DEVICE)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);
	if (NULL == pCeDev)
		jump_to_func(NULL, ali_rpc_call, NULL, CE_NPARA(0) | FUNC_CE_ATTACH, NULL);
}

void ali_m36_ce_see_uninit(void)
{
	pCE_DEVICE pCeDev = NULL;

	pCeDev = (pCE_DEVICE)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);
	if (NULL != pCeDev)
		jump_to_func(NULL, ali_rpc_call, NULL, CE_NPARA(0) | FUNC_CE_DETACH, NULL);
}

void ali_m36_ce_see_suspend(void)
{
	pCE_DEVICE pCeDev = NULL;
	
	pCeDev = (pCE_DEVICE)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);
	if (NULL != pCeDev) {
		jump_to_func(NULL, ali_rpc_call, NULL, CE_NPARA(0) | FUNC_CE_SUSPEND, NULL);
	}
}

void ali_m36_ce_see_resume(void)
{
	pCE_DEVICE pCeDev = NULL;
	
	pCeDev = (pCE_DEVICE)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);
	if (NULL != pCeDev) {
		jump_to_func(NULL, ali_rpc_call, NULL, CE_NPARA(0) | FUNC_CE_RESUME, NULL);
	}
}

