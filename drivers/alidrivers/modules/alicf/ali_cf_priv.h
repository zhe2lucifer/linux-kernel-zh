/*
 * Driver structures for CF
 *
 * Copyright (c) 2016 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CF_PRIV_H
#define _ALI_CF_PRIV_H

#include "ali_cf_rpc.h"
#include "ali_cf_utils.h"
#include <ali_cf.h>
#include <alidefinition/adf_cf.h>
#include <see_bus.h>
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

struct cf_plat_data {
	int is_cf_disabled; /* cf has been disabled*/
};
#define CF_DEV_NODE "ali_cf0"
#define CF_TARGET_DEV_NODE "ali_cf_target"
#define CF_INVALID_VALUE ((int)-1)

struct cf_drv {
	struct semaphore sem;
	dev_t devt;
	struct device *dev;
	struct class *dev_class;
	struct cdev cdev;
	int chan; /* channel for operating the SEE of CF*/
	struct see_client *see_clnt;
	const struct cf_see_ops *see_ops;
	struct dentry *debugfs_dir;
};

#endif

