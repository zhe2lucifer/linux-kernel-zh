/*
 * VSC Driver for Conax Virtual Smart Card
 *
 * Copyright (c) 2015 ALi Corp
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
#include <alidefinition/adf_vsc.h>
#include <alidefinition/adf_ce.h>
#include "ca_vsc_rpc.h"
#include "../alikl/ca_kl_rpc.h"



void ca_vsc_store_mem_alloc(u32 *mem_addr);
void ca_vsc_store_config(VSC_STORE_CONFIG *p1, u32 *p2);
void ca_vsc_smc_cmd_response(const VSC_PKT *pkt, const u16 *sw);

enum LLD_VSC_SW_FUNC
{
	FUNC_VSC_LIB_INIT = 0, 
	FUNC_VSC_DISPATCH_CMD_TRANSFER, 
	FUNC_VSC_NVRAM_MALLOC,  
	FUNC_VSC_NVRAM_WRITE_FLASH,
	FUNC_CMD_RESPONSE_TRANSFER,  
	FUNC_VSC_LIB_GET_KEY,  
	FUNC_CMD_DECRYPT_DECW, 
	FUNC_VSC_DECRYPT_STORE,
	FUNC_VSC_GENRATE_REC_KEY, 
	FUNC_VSC_CLEAR_STORE,  
	FUNC_VSC_DECRYPT_UK,
	FUNC_VSC_LOAD_PK_TO_CE,  // SPECIAL API FOR 3281 
	FUNC_VSC_CONFIG_CHANGE_KEY, 
	FUNC_VSC_CRYPT_CW_DERIVE_CW,
	FUNC_VSC_PROCESS_LIB, // API FOR C1903A(SMI+CONAX)
};

/**
* @brief VSC remote call list
*/
u32 lld_vsc_entry[] =
{
    (UINT32) vsc_lib_init,
    (UINT32) vsc_dispatch_cmd_transfer,
    (UINT32) ca_vsc_store_mem_alloc,
    (UINT32) ca_vsc_store_config,
    (UINT32) ca_vsc_smc_cmd_response,
    (UINT32) vsc_lib_get_key,
    (UINT32) vsc_decrypt_store,
    (UINT32) vsc_clear_store,
};

#define VSC_LLD_NPARA(x) ((LLD_VSC_MODULE<<24)|(x<<16))

void lld_vsc_callee(u8 *msg)
{
	ali_rpc_ret((u32)lld_vsc_entry, msg);
}

int vsc_lib_init(const u32 *addr, const u32 *len)
{
	u32 desc[] = {
		2, DESC_STATIC_STRU(0, 4), DESC_STATIC_STRU(1, 4),
		2, DESC_P_PARA(0, 0, 0), DESC_P_PARA(0, 1, 1),
		0,
		0,
	};

	u32 func_desc = (u32)(VSC_LLD_NPARA(2) | FUNC_VSC_LIB_INIT);
	jump_to_func(NULL, ali_rpc_call, addr, func_desc, desc);
}

int vsc_dispatch_cmd_transfer(const VSC_PKT *cmd_pkt,
		u16 *resp_len, u16 *sw)
{
	u32 desc[] =
	{
		3, DESC_STATIC_STRU(0, sizeof (VSC_PKT)),
		DESC_OUTPUT_STRU(1, 2),
		DESC_OUTPUT_STRU(2, 2),
		3, DESC_P_PARA(0, 0, 0), DESC_P_PARA(0, 1, 1), DESC_P_PARA(0, 2, 2),
		0,
		0,
	};

	u32 func_desc = (u32)(VSC_LLD_NPARA(3) | FUNC_VSC_DISPATCH_CMD_TRANSFER);
	jump_to_func(NULL, ali_rpc_call, cmd_pkt, func_desc, desc);
}

int vsc_decrypt_store(const u32 *vsc_store_addr,
		VSC_STORE_CONFIG *config,
		VSC_STORE_CONFIG *backup_config,
		u8 *index, u8 *wb)
{
	u32 desc[] = {
		5, DESC_STATIC_STRU(0, 4),
		DESC_OUTPUT_STRU(1, sizeof (VSC_STORE_CONFIG)),
		DESC_OUTPUT_STRU(2, sizeof (VSC_STORE_CONFIG)),
		DESC_STATIC_STRU(1, 16),
		DESC_STATIC_STRU(1, 16),
		5,
		DESC_P_PARA(0, 0, 0), DESC_P_PARA(0, 1, 1), DESC_P_PARA(0, 2, 2),
		DESC_P_PARA(0, 3, 3), DESC_P_PARA(0, 4, 4),
		0,
		0,
	};

	u32 func_desc = (u32)(VSC_LLD_NPARA(5) | FUNC_VSC_DECRYPT_STORE);
	jump_to_func(NULL, ali_rpc_call, vsc_store_addr, func_desc, desc);
}

void vsc_clear_store(void)
{
	u32 func_desc = (u32)(VSC_LLD_NPARA(0) | FUNC_VSC_CLEAR_STORE);
	jump_to_func(NULL, ali_rpc_call, NULL, func_desc, NULL);
}

void vsc_process_lib(void)
{
	u32 func_desc = (u32)(VSC_LLD_NPARA(0) | FUNC_VSC_PROCESS_LIB);
	jump_to_func(NULL, ali_rpc_call, NULL, func_desc, NULL);
}

int vsc_lib_get_key(const u16 *key_id)
{
	UINT32 desc[] = {
		1, DESC_STATIC_STRU(0,2),
		1, DESC_P_PARA(0, 0, 0),
		0,
		0,
	};

	u32 func_desc = (u32)(VSC_LLD_NPARA(1) | FUNC_VSC_LIB_GET_KEY);
	jump_to_func(NULL, ali_rpc_call, key_id, func_desc, desc);
}

int vsc_crypt_decw_key(CE_DEVICE *pCeDev, u32 param)
{
	__u32 cmd = (CE_IO_CMD(IO_CRYPT_VSC_DECW_KEY));
	return ali_ce_ioctl(pCeDev, cmd, param);
}

