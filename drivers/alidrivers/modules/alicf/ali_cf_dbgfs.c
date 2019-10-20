/*
 * CF debugfs
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
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#include "ali_cf_dbgfs.h"

void cf_dbgfs_create(struct cf_drv *drv)
{
	drv->debugfs_dir = debugfs_create_dir("ali_cf", NULL);

	if (!drv->debugfs_dir || IS_ERR_OR_NULL(drv->debugfs_dir))
		dev_err(drv->dev, "debugfs create dentry failed\n");
}

void cf_dbgfs_remove(struct cf_drv *drv)
{
	if (drv && drv->debugfs_dir)
		debugfs_remove_recursive(drv->debugfs_dir);
}

