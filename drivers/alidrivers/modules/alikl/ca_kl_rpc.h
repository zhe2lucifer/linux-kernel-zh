
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

#ifndef _CA_KL_RPC_H_
#define _CA_KL_RPC_H_

#include <alidefinition/adf_ce.h>

int ali_ce_umemcpy(void *dest, const void *src, __u32 n);
int ali_ce_ioctl(CE_DEVICE *pCeDev, __u32 cmd, __u32 param);
int ali_ce_generate_single_level_key(pCE_DEVICE pCeDev, pCE_DATA_INFO pCe_data_info);
int ali_ce_generate_hdcp_key(pCE_DEVICE pCeDev, __u8 *en_hdcp_key, __u16 len);
void ali_m36_ce_see_init(void);
void ali_m36_ce_see_uninit(void);
void ali_m36_ce_see_suspend(void);
void ali_m36_ce_see_resume(void);


#endif

