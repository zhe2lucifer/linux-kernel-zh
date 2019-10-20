/**
 * RPC interface for CASI
 *
 * Copyright (c) ALi Corporation.
 *
 * This file is released under the GPLv2
 */

#ifndef _CASI_RPC_H
#define _CASI_RPC_H

#include <alidefinition/adf_dsc.h>
#include <alidefinition/adf_ce.h>
#include "ca_casi_common.h"

#define IO_CASI_TA_DATA_SET 1
#define IO_CASI_GEN_NLEVEL_TA_KEY 2

struct casi_see_ops {
	int (*open)(void);
	int (*ioctl)(__u32 chan, __u32 cmd, __u32 param);
	int (*close)(__u32 chan);
};

struct casi_see_ta_data_prm {
	__u32 ta_data_phy;
	int algo;
	void *sub_dev_see_hdl;
	unsigned short stream_id;
};

int see_casi_register(void *prm);
int see_casi_unregister(void *prm);

#endif //_CASI_RPC_H

