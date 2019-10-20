/*
 * DeScrambler Core driver
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
#include <linux/types.h>
#include <linux/uaccess.h>
#include <ali_cache.h>

#include "ca_dsc_priv.h"

#undef UC
#define UC(x) ((void *)(((UINT32)(x)&0xBFFFFFFF)|0xa0000000))

#define DSC_NPARA(x) ((HLD_DSC_MODULE << 24)|(x << 16))

__u32 des_io_control[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0), 
	//desc of pointer ret
	0,                          
	0,
};

__u32 aes_io_control[] = {
	/* desc of pointer para */
	1, DESC_OUTPUT_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0),
	/* desc of pointer ret */
	0,
	0,
};

__u32 csa_io_control[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0), 
	//desc of pointer ret
	0,                          
	0,
};

__u32 dsc_io_control[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0), 
	//desc of pointer ret
	0,                          
	0,
};

__u32 dsc_io_control_1[] = 
{ //desc of pointer para
	9, DESC_OUTPUT_STRU(0, sizeof(KEY_PARAM)), DESC_STATIC_STRU(1, 0),\
		 DESC_STATIC_STRU(2, 0*sizeof(AES_KEY_PARAM)), \
		 DESC_STATIC_STRU(3, 0*sizeof(CSA_KEY_PARAM)), \
		 DESC_STATIC_STRU(4, 0*sizeof(DES_KEY_PARAM)), \
		 DESC_STATIC_STRU(5, 0*sizeof(AES_IV_INFO)),\
		 DESC_STATIC_STRU(6, 0*sizeof(DES_IV_INFO)), \
		 DESC_STATIC_STRU(7, 16), DESC_STATIC_STRU(8, 16), 
	9, DESC_P_PARA(0, 2, 0), DESC_P_STRU(1, 0, 1, offsetof(KEY_PARAM, pid_list)),\
		DESC_P_STRU(1, 0, 2, offsetof(KEY_PARAM, p_aes_key_info)), \
		DESC_P_STRU(1, 0, 3, offsetof(KEY_PARAM, p_csa_key_info)), \
		DESC_P_STRU(1, 0, 4, offsetof(KEY_PARAM, p_des_key_info)), \
		DESC_P_STRU(1, 0, 5, offsetof(KEY_PARAM, p_aes_iv_info)), \
		DESC_P_STRU(1, 0, 6, offsetof(KEY_PARAM, p_des_iv_info)), \
		DESC_P_STRU(1, 0, 7, offsetof(KEY_PARAM, init_vector)), \
		DESC_P_STRU(1, 0, 8, offsetof(KEY_PARAM, ctr_counter)),
	//desc of pointer ret
	0,                          
	0,
};

__u32 dsc_ioctl_key_param_no_iv[] = 
{ //desc of pointer para
	7, DESC_OUTPUT_STRU(0, sizeof(KEY_PARAM)), DESC_STATIC_STRU(1, 0),\
		 DESC_STATIC_STRU(2, 0*sizeof(AES_KEY_PARAM)), \
		 DESC_STATIC_STRU(3, 0*sizeof(CSA_KEY_PARAM)), \
		 DESC_STATIC_STRU(4, 0*sizeof(DES_KEY_PARAM)), \
		 DESC_STATIC_STRU(5, 16), DESC_STATIC_STRU(6, 16), 
	7, DESC_P_PARA(0, 2, 0), DESC_P_STRU(1, 0, 1, offsetof(KEY_PARAM, pid_list)),\
		DESC_P_STRU(1, 0, 2, offsetof(KEY_PARAM, p_aes_key_info)), \
		DESC_P_STRU(1, 0, 3, offsetof(KEY_PARAM, p_csa_key_info)), \
		DESC_P_STRU(1, 0, 4, offsetof(KEY_PARAM, p_des_key_info)), \
		DESC_P_STRU(1, 0, 5, offsetof(KEY_PARAM, init_vector)), \
		DESC_P_STRU(1, 0, 6, offsetof(KEY_PARAM, ctr_counter)),
	//desc of pointer ret
	0,							
	0,
};



int ali_dsc_umemcpy(void *dest, const void *src, __u32 n)
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

static int ali_des_ioctl_rpc(DES_DEV *pDesDev, __u32 cmd, __u32 param)
{
	__u32 i;
	__u32 common_desc[40];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)des_io_control;
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	
	for (i = 0; i < sizeof(des_io_control)/sizeof(__u32); i++) {
		desc[i] = b[i];
	}
	
	switch (DSC_IO_CMD(cmd)) {
		case DSC_IO_CMD(IO_INIT_CMD): {
			DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(DES_INIT_PARAM));
			break;
		}	
		
		case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
		case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
			if (NULL == key_param->p_des_iv_info)
			{
				b = (__u32 *)dsc_ioctl_key_param_no_iv;
				for (i = 0; i < sizeof(dsc_ioctl_key_param_no_iv)/sizeof(__u32); i++) {
					desc[i] = b[i];
				}
			}
			else
			{
				b = (__u32 *)dsc_io_control_1;
				for (i = 0; i < sizeof(dsc_io_control_1)/sizeof(__u32); i++) {
					desc[i] = b[i];
				}

				DESC_STATIC_STRU_SET_SIZE(desc, 6, 2*8);
			}

			
			for (i = 0; i < 3; i++) {
				DESC_STATIC_STRU_SET_SIZE(desc, (i+2), 2*key_param->key_length/8);
			}
			
			DESC_STATIC_STRU_SET_SIZE(desc, 1, sizeof(__u16)*key_param->pid_len);
			break;		  
		}
		
		case DSC_IO_CMD(IO_ADD_DEL_PID): {
			DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(PID_PARAM ));
			break;
		}

		case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
			DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(IV_OTHER_PARAM));
			break;
		}
		
		case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
			desc = NULL;
			break;

		case DSC_IO_CMD(IO_CONTINUOUS_RENEW):
			desc = NULL;
			break;

		default:
			return ALI_DSC_ERROR_OPERATION_NOT_SUPPORTED;
	}
	
	jump_to_func(NULL, ali_rpc_call, pDesDev, DSC_NPARA(3)|FUNC_DES_IOCTL, desc);	

}

int ali_des_ioctl(struct ca_dsc_dev *dsc,
	DES_DEV *pDesDev, __u32 cmd, __u32 param)
{
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	DES_INIT_PARAM des_param;
	PID_PARAM pid_param;
	IV_OTHER_PARAM other_param;
	__u32 param_rpc = param;

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD): {
		ali_dsc_umemcpy(&des_param, (void *)param,
			sizeof(DES_INIT_PARAM));
		param_rpc = (__u32)&des_param;
		break;
	}
	case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
	case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
		param_rpc = (__u32)key_param;
		break;
	}

	case DSC_IO_CMD(IO_ADD_DEL_PID): {
		memcpy(&pid_param, (void *)param, sizeof(PID_PARAM));
		param_rpc = (__u32)&pid_param;
		break;
	}

	case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
		memcpy(&other_param, (void *)param, sizeof(IV_OTHER_PARAM));
		param_rpc = (__u32)&other_param;
		break;
	}

	case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
		break;

	case DSC_IO_CMD(IO_CONTINUOUS_RENEW):
		break;

	default:
	    dev_err(dsc->dev, "Dsc rpc invalid cmd=%d\n", cmd);
		break;
	}

	return ali_des_ioctl_rpc(pDesDev, cmd, param_rpc);
}


static int ali_aes_ioctl_rpc(AES_DEV *pAesDev, __u32 cmd, __u32 param)
{
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	__u32 i;
	__u32 common_desc[40];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)aes_io_control;

	for(i = 0; i < sizeof(aes_io_control)/sizeof(__u32); i++)
	{
		desc[i] = b[i];
	}

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD): {
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(AES_INIT_PARAM));
		break;
	}

	case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
	case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
		if (NULL == key_param->p_aes_iv_info)
		{
			b = (__u32 *)dsc_ioctl_key_param_no_iv;
			for (i = 0; i < sizeof(dsc_ioctl_key_param_no_iv)/sizeof(__u32); i++) {
				desc[i] = b[i];
			}
		}
		else
		{
			b = (__u32 *)dsc_io_control_1;
			for (i = 0; i < sizeof(dsc_io_control_1)/sizeof(__u32); i++) {
				desc[i] = b[i];
			}

			DESC_STATIC_STRU_SET_SIZE(desc, 5, 2*16);
		}

		for (i = 0; i < 3; i++) {
			DESC_STATIC_STRU_SET_SIZE(desc, (i+2), 2*key_param->key_length/8);
		}
		
		DESC_STATIC_STRU_SET_SIZE(desc, 1, sizeof(__u16)*key_param->pid_len);
		
		break;
	}

	case DSC_IO_CMD(IO_ADD_DEL_PID): {
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(PID_PARAM ));
		break;
	}

	case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(IV_OTHER_PARAM));
		break;
	}

	case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
		desc = NULL;
		break;

	case DSC_IO_CMD(IO_CONTINUOUS_RENEW):
		desc = NULL;
		break;

	case DSC_IO_CMD(IO_AES_CRYPT_WITH_FP):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(AES_CRYPT_WITH_FP));
		break;

	default:
		return ALI_DSC_ERROR_OPERATION_NOT_SUPPORTED;
	}

	jump_to_func(NULL, ali_rpc_call, pAesDev, DSC_NPARA(3)|FUNC_AES_IOCTL, desc);
}

int ali_aes_ioctl(struct ca_dsc_dev *dsc,
	AES_DEV *pAesDev, __u32 cmd, __u32 param)
{
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	AES_CRYPT_WITH_FP *crypt_fp = (AES_CRYPT_WITH_FP *)param;
	AES_INIT_PARAM aes_param;
	PID_PARAM pid_param;
	IV_OTHER_PARAM other_param;
	__u32 param_rpc = param;

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD):
		memcpy(&aes_param, (void *)param, sizeof(AES_INIT_PARAM));
		param_rpc = (__u32)&aes_param;
	break;

	case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
	case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
		param_rpc = (__u32)key_param;
		break;
	}

	case DSC_IO_CMD(IO_ADD_DEL_PID): {
		memcpy(&pid_param, (void *)param, sizeof(PID_PARAM));
		param_rpc = (__u32)&pid_param;
		break;
	}

	case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
		memcpy(&other_param, (void *)param, sizeof(IV_OTHER_PARAM));
		param_rpc = (__u32)&other_param;
		break;
	}

	case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
		break;

	case DSC_IO_CMD(IO_CONTINUOUS_RENEW):
		break;

	case DSC_IO_CMD(IO_AES_CRYPT_WITH_FP): {
		param_rpc = (__u32)crypt_fp;
		break;
	}
	
	default:
		dev_err(dsc->dev, "Dsc rpc invalid cmd=%d\n", cmd);
		break;
	}
	
	return ali_aes_ioctl_rpc(pAesDev, cmd, param_rpc);
}

static int ali_dsc_ioctl_rpc(DSC_DEV *pDscDev, __u32 cmd, __u32 param)
{
	__u32 i;
	__u32 common_desc[40];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)dsc_io_control;

	for(i = 0; i < sizeof(dsc_io_control)/sizeof(__u32); i++)
	{
		desc[i] = b[i];
	}

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_PARSE_DMX_ID_GET_CMD):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(__u32));
		break;
	case DSC_IO_CMD(IO_PARSE_DMX_ID_SET_CMD):
		desc = NULL;
		break;
	case DSC_IO_CMD(IO_DSC_GET_DES_HANDLE):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(__u32));
		break;
	case DSC_IO_CMD(IO_DSC_GET_AES_HANDLE):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(__u32));
		break;
	case DSC_IO_CMD(IO_DSC_GET_CSA_HANDLE):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(__u32));
		break;
	case DSC_IO_CMD(IO_DSC_GET_SHA_HANDLE):
		DESC_OUTPUT_STRU_SET_SIZE(common_desc, 0, sizeof(__u32));
		break;
	case DSC_IO_CMD(IO_DSC_SET_PVR_KEY_PARAM):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(DSC_PVR_KEY_PARAM));
		break;
	case DSC_IO_CMD(IO_DSC_SET_ENCRYPT_PRIORITY):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(DSC_EN_PRIORITY));
		break;
	case DSC_IO_CMD(IO_DSC_GET_DRIVER_VERSION):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, 20);
		break;
	case DSC_IO_CMD(IO_DSC_VER_CHECK):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(DSC_VER_CHK_PARAM));
		break;
	case DSC_IO_CMD(IO_DSC_CALC_CMAC):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(DSC_CALC_CMAC));
		break;
	case DSC_IO_CMD(IO_DSC_SET_PVR_KEY_IDLE):
	case DSC_IO_CMD(IO_DSC_SET_CLR_CMDQ_EN):
	case DSC_IO_CMD(IO_DSC_DELETE_HANDLE_CMD):
		desc = NULL;
		break;
	case DSC_IO_CMD(IO_DSC_FIXED_DECRYPTION):
	case DSC_IO_CMD(IO_DSC_SYS_UK_FW):
	default:
		return ALI_DSC_ERROR_OPERATION_NOT_SUPPORTED;
	}

	jump_to_func(NULL, ali_rpc_call, pDscDev, \
				DSC_NPARA(3)|FUNC_DSC_IOCTL, \
				desc);
}

int ali_dsc_ioctl(struct ca_dsc_dev *dsc,
	DSC_DEV *pDscDev, __u32 cmd, __u32 param)
{
	DSC_PVR_KEY_PARAM *pvr_key = (DSC_PVR_KEY_PARAM *)param;
	__u32 param_rpc = param;
	
	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_PARSE_DMX_ID_GET_CMD):
		break;
	case DSC_IO_CMD(IO_PARSE_DMX_ID_SET_CMD):
		break;
	case DSC_IO_CMD(IO_DSC_GET_DES_HANDLE):
		break;
	case DSC_IO_CMD(IO_DSC_GET_AES_HANDLE):
		break;
	case DSC_IO_CMD(IO_DSC_GET_CSA_HANDLE):
		break;
	case DSC_IO_CMD(IO_DSC_GET_SHA_HANDLE):
		break;
	case DSC_IO_CMD(IO_DSC_SET_PVR_KEY_PARAM):
		param_rpc = (__u32)pvr_key;
		break;
	case DSC_IO_CMD(IO_DSC_SET_ENCRYPT_PRIORITY):
		break;
	case DSC_IO_CMD(IO_DSC_GET_DRIVER_VERSION):
		break;
	case DSC_IO_CMD(IO_DSC_VER_CHECK):
		break;
	case DSC_IO_CMD(IO_DSC_CALC_CMAC):
		break;
	case DSC_IO_CMD(IO_DSC_SET_PVR_KEY_IDLE):
	case DSC_IO_CMD(IO_DSC_SET_CLR_CMDQ_EN):
	case DSC_IO_CMD(IO_DSC_DELETE_HANDLE_CMD):
		break;
	case DSC_IO_CMD(IO_DSC_FIXED_DECRYPTION):
	case DSC_IO_CMD(IO_DSC_SYS_UK_FW):
	default:
		return ALI_DSC_ERROR_OPERATION_NOT_SUPPORTED;
	}

	return 	ali_dsc_ioctl_rpc(pDscDev, cmd, param_rpc);
}

static __u16 ali_dsc_get_free_stream_id_rpc(enum DMA_MODE dma_mode)
{
	jump_to_func(NULL, ali_rpc_call, dma_mode, \
	 			  DSC_NPARA(1)|FUNC_GET_FREE_STREAM_ID, \
	 			  NULL);
}

__u16 ali_dsc_get_free_stream_id(struct ca_dsc_dev *dsc,
	enum DMA_MODE dma_mode)
{
	return ali_dsc_get_free_stream_id_rpc(dma_mode);
}

static __u16 ali_dsc_get_free_sub_device_id_rpc(__u8 sub_mode)
{
	jump_to_func(NULL, ali_rpc_call, sub_mode, \
	 			  DSC_NPARA(1)|FUNC_GET_FREE_SUB_DEVICE_ID, \
	 			  NULL);
}

__u32 ali_dsc_get_free_sub_device_id(struct ca_dsc_dev *dsc,
	__u8 sub_mode)
{
	return ali_dsc_get_free_sub_device_id_rpc(sub_mode);
}

static int ali_dsc_set_sub_device_id_idle_rpc(__u8 sub_mode, 
	__u32 device_id)
{
	jump_to_func(NULL, ali_rpc_call, sub_mode, \
	 			  DSC_NPARA(2)|FUNC_SET_SUB_DEVICE_ID_IDLE, \
	 			  NULL);
}

int ali_dsc_set_sub_device_id_idle(struct ca_dsc_dev *dsc,
	__u8 sub_mode, __u32 device_id)
{
	return ali_dsc_set_sub_device_id_idle_rpc(sub_mode, device_id);
}


static int ali_dsc_set_stream_id_idle_rpc(__u32 pos)
{
	jump_to_func(NULL, ali_rpc_call, pos, \
	 			  DSC_NPARA(1)|FUNC_SET_STREAM_ID_IDLE, \
	 			  NULL);
}

int ali_dsc_set_stream_id_idle(struct ca_dsc_dev *dsc,
	__u32 pos)
{
	return ali_dsc_set_stream_id_idle_rpc(pos);
}

static int ali_csa_ioctl_rpc(CSA_DEV *pCsaDev, __u32 cmd, __u32 param)
{
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	__u32 i;
	__u32 common_desc[40];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)csa_io_control;
	
	for(i = 0; i < sizeof(csa_io_control)/sizeof(__u32); i++)
	{
		desc[i] = b[i];
	}

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD):
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(CSA_INIT_PARAM ));
		break;

	case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
	case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
		b = (__u32 *)dsc_io_control_1;
		for (i = 0; i < sizeof(dsc_io_control_1)/sizeof(__u32); i++) {
			desc[i] = b[i];
		}

		for (i = 0; i < 3; i++) {
			DESC_STATIC_STRU_SET_SIZE(desc, (i+2), 2*key_param->key_length/8);
		}

		DESC_STATIC_STRU_SET_SIZE(desc, 1, sizeof(__u16)*key_param->pid_len);
		break;
	}

	case DSC_IO_CMD(IO_ADD_DEL_PID): {
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(PID_PARAM ));
		break;
	}

	case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
		DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(IV_OTHER_PARAM));
		break;
	}

	case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
		desc = NULL;
		break;

	default:
	    return ALI_DSC_ERROR_OPERATION_NOT_SUPPORTED;
	}

	jump_to_func(NULL, ali_rpc_call, pCsaDev, DSC_NPARA(3)|FUNC_CSA_IOCTL, desc);
}

int ali_csa_ioctl(struct ca_dsc_dev *dsc,
	CSA_DEV *pCsaDev, __u32 cmd, __u32 param)
{
	KEY_PARAM *key_param = (KEY_PARAM *)param;
	CSA_INIT_PARAM csa_param;
	PID_PARAM pid_param;
	IV_OTHER_PARAM other_param;
	__u32 param_rpc = param;

	switch (DSC_IO_CMD(cmd)) {
	case DSC_IO_CMD(IO_INIT_CMD):
		ali_dsc_umemcpy(&csa_param, (void *)param,
			sizeof(csa_param));
		param_rpc = (__u32)&csa_param;
		break;

	case DSC_IO_CMD(IO_CREAT_CRYPT_STREAM_CMD):
	case DSC_IO_CMD(IO_KEY_INFO_UPDATE_CMD): {
		param_rpc = (__u32)key_param;
		break;
	}

	case DSC_IO_CMD(IO_ADD_DEL_PID): {
		memcpy(&pid_param, (void *)param, sizeof(PID_PARAM));
		param_rpc = (__u32)&pid_param;
		break;
	}

	case DSC_IO_CMD(IO_PARAM_INFO_UPDATE): {
		memcpy(&other_param, (void *)param, sizeof(IV_OTHER_PARAM));
		param_rpc = (__u32)&other_param;
		break;
	}

	case DSC_IO_CMD(IO_DELETE_CRYPT_STREAM_CMD):
		break;

	default:
	    dev_err(dsc->dev, "Dsc rpc invalid cmd=%d\n", cmd);
		break;
	}

	return ali_csa_ioctl_rpc(pCsaDev, cmd, param_rpc);
}

void ali_m36_dsc_see_init(void)
{
	DSC_DEV *pDscDev = (DSC_DEV *) hld_dev_get_by_type ( NULL, HLD_DEV_TYPE_DSC );
	if(NULL == pDscDev)
	{
		jump_to_func(NULL, ali_rpc_call, NULL, \
					DSC_NPARA(0)|FUNC_DSC_ATTACH, \
					NULL);
	}
}

void ali_m36_dsc_see_uninit(void)
{
	DSC_DEV *pDscDev = (DSC_DEV *) hld_dev_get_by_type ( NULL, HLD_DEV_TYPE_DSC );
	if(pDscDev)
	{
		jump_to_func(NULL, ali_rpc_call, NULL, \
					DSC_NPARA(0)|FUNC_DSC_DETACH, \
					NULL);
	}
}
static int ali_dsc_create_sbm_task_rpc(UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
	 			  DSC_NPARA(1)|FUNC_DSC_CREATE_SBM_TASK, \
	 			  NULL);
}

int ali_dsc_create_sbm_task(struct ca_dsc_dev *dsc,
	UINT32 sbm_id)
{
	return ali_dsc_create_sbm_task_rpc(sbm_id);
}

static int  ali_dsc_delete_sbm_task_rpc(UINT32 sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
	 			  DSC_NPARA(1)|FUNC_DSC_DELETE_SBM_TASK, \
	 			  NULL);
}

int ali_dsc_delete_sbm_task(struct ca_dsc_dev *dsc,
	UINT32 sbm_id)
{
	return ali_dsc_delete_sbm_task_rpc(sbm_id);
}

void ali_m36_dsc_see_suspend(void)
{
	DSC_DEV *pDscDev = (DSC_DEV *) hld_dev_get_by_type ( NULL, HLD_DEV_TYPE_DSC );
	if(pDscDev)
	{
		jump_to_func(NULL, ali_rpc_call, NULL, \
					DSC_NPARA(0)|FUNC_DSC_SUSPEND, \
					NULL);
	}
}

void ali_m36_dsc_see_resume(void)
{
	DSC_DEV *pDscDev = (DSC_DEV *) hld_dev_get_by_type ( NULL, HLD_DEV_TYPE_DSC );
	if(pDscDev)
	{
		jump_to_func(NULL, ali_rpc_call, NULL, \
					DSC_NPARA(0)|FUNC_DSC_RESUME, \
					NULL);
	}
}

int ali_trig_ram_mon(__u32 start_addr,__u32 end_addr, 
						__u32 interval, __u32 sha_mode, 
						int DisableOrEnable)
{
	jump_to_func(NULL, ali_rpc_call, start_addr, \
				DSC_NPARA(5)|FUNC_TRIG_RAM_MON, \
				NULL);
}
EXPORT_SYMBOL(ali_trig_ram_mon);
