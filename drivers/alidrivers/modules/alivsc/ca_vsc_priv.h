/*
 * Conax Virtual Smart Card Core driver
 * Copyright(C) 2016 ALi Corporation. All rights reserved.
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

#ifndef __CA_VSC_PRIV_H__
#define __CA_VSC_PRIV_H__


#include <ca_vsc.h>
#include <see_bus.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#include <linux/cdev.h>
#include <linux/platform_device.h>


#define VSC_STATE_SMC_OPENED     (1 << 0)
#define VSC_STATE_SMC_DATA       (1 << 1)
#define VSC_STATE_STORE_OPENED   (1 << 2)
#define VSC_STATE_STORE_INITED   (1 << 3)
#define VSC_STATE_STORE_LOADED   (1 << 4)


struct ca_vsc_smc {
	dev_t devt;
	struct cdev cdev;
	struct device *dev;
	struct class *dev_class;

	struct ca_vsc_drv *drv;
	struct see_client *see_clnt;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif

	int state;
	int cnt;
	wait_queue_head_t wq;
	/* status word */
	u16 sw;
	/* number of bytes to write */
	u8 response[VSC_DATA_SIZE_MAX];
	/* Array containing response from VSC-Lib */
	s32 response_len;
	 /* number of response bytes read */
	struct vsc_cmd_transfer tr;
	/* temporary cmd transfer buffer */
};

struct ca_vsc_store {
	dev_t devt;
	struct cdev cdev;
	struct device *dev;
	struct class *dev_class;

	struct ca_vsc_drv *drv;
	struct see_client *see_clnt;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif

	wait_queue_head_t wq;
	int state;
	struct vsc_store store;
	int wb;
};


struct ca_vsc_plat_data {
	unsigned int lib_addr;
	unsigned int lib_len;
};

/* definition for private data structure for vsc driver */
struct ca_vsc_drv {
	struct see_client *clnt;
	struct ca_vsc_plat_data *pdata;

	void *see_ce_id;

	struct mutex mutex;
	struct ca_vsc_smc *smc_dev;
	struct ca_vsc_store *store_dev;
};


#endif

