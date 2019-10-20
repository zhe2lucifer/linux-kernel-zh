/*
 * VSC Driver for Conax Virtual Smart Card
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef __CA_VSC_RPC_H__
#define __CA_VSC_RPC_H__

#include <alidefinition/adf_vsc.h>
#include <alidefinition/adf_ce.h>

#define KL_LOW_ADDR		0       /* low addr for even*/
#define KL_HIGH_ADDR	1       /* high addr for odd*/


int vsc_lib_init(const u32 *addr, const u32 *len);

int vsc_dispatch_cmd_transfer(const VSC_PKT *cmd_pkt, u16 *resp_len, u16 *sw);

int vsc_decrypt_store(const u32 *data,
		VSC_STORE_CONFIG *config,
		VSC_STORE_CONFIG *backup,
		u8 *index, u8 *wb);

void vsc_clear_store(void);

void vsc_process_lib(void);

int vsc_lib_get_key(const u16 *key_id);

int vsc_crypt_decw_key(CE_DEVICE *pCeDev, u32 param);

#endif //__CA_VSC_RPC_H__
