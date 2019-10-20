/**
 * RPC interface for IRDETO CASI
 *
 * Copyright (c) ALi Corporation.
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

#include "casi_rpc.h"
#include "casi_dev.h"

#define CASI_HLD_NPARA(x) ((HLD_CASI_MODULE<<24)|(x<<16))

#define CASI_DUMP(data,len) \
	do{ \
		int i, l=(len); \
		for(i=0; i<l; i++){ \
			printk(KERN_ERR"0x%02x,",*(unsigned char*)((unsigned int)data+i)); \
			if((i+1)%16==0) \
				printk(KERN_ERR"\n"); \
		} \
		if((i)%16==0) \
			printk(KERN_ERR"\n"); \
		else \
			printk(KERN_ERR"\n"); \
	}while(0)

enum HLD_CASI_FUNC {
	FUNC_CASI_SEE_OPEN,
	FUNC_CASI_SEE_IOCTL,
	FUNC_CASI_SEE_CLOSE,
};

static __u32 casi_io_control[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 2, 0), 
	//desc of pointer ret
	0,                          
	0,
};

int casi_see_open(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
					CASI_HLD_NPARA(0)|FUNC_CASI_SEE_OPEN, \
					NULL);
}

int casi_see_ioctl(__u32 chan, __u32 cmd, __u32 param)
{
	int ret = -1;
	__u32 i;
	__u32 common_desc[sizeof(casi_io_control)];
	__u32 *desc = (__u32 *)common_desc;
	__u32 *b = (__u32 *)casi_io_control;

	for(i = 0; i < sizeof(casi_io_control)/sizeof(__u32); i++)
	{
		desc[i] = b[i];
	}

	switch (cmd) {
		case IO_CASI_TA_DATA_SET:
		{
			DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(struct casi_see_ta_data_prm));
			break;
		}
		case IO_CASI_GEN_NLEVEL_TA_KEY:
		{
			DESC_STATIC_STRU_SET_SIZE(common_desc, 0, sizeof(struct ce_generate_nlevel));
			break;
		}
		default:
			return ret;
	}

	jump_to_func(NULL, ali_rpc_call, chan, CASI_HLD_NPARA(3)|FUNC_CASI_SEE_IOCTL, desc);
}

int casi_see_close(__u32 chan)
{
	jump_to_func(NULL, ali_rpc_call, chan, \
		CASI_HLD_NPARA(1)|FUNC_CASI_SEE_CLOSE, \
		NULL);
}

static const struct casi_see_ops see_casi_ops = {
	.open = casi_see_open,
	.ioctl = casi_see_ioctl,
	.close = casi_see_close,
};

int see_casi_register(void *data)
{
	struct ali_casi_entry *casi_entry;
	if (!data) {
		pr_err("casi device data is NULL\n");
		return -1;
	}
	
	casi_entry = (struct ali_casi_entry *)data;

	casi_entry->see_ops = &see_casi_ops;

	return 0;
}

int see_casi_unregister(void *data)
{
	struct ali_casi_entry *casi_entry = (struct ali_casi_entry *)data;

	casi_entry->see_ops = NULL;

	return 0;
}

