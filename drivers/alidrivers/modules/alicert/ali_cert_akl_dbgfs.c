/*
 * CERT debugfs for AKL debuging
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
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#include "ali_cert_akl_dbgfs.h"

static int cert_akl_dbg_key_info_get(void *data, __u64 *val)
{
	struct cert_akl_key *key = data;

	if (key->cell) {
		*val = key->cell->pos;

		dev_info(key->drv->dev,
			"pos: 0x%x, algo: %d\n",
			key->cell->pos, key->algo);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(cert_akl_dbg_key_info_fops,
		cert_akl_dbg_key_info_get, NULL, "0x%llx\n");

void cert_akl_dbgfs_create(struct cert_akl_drv *drv)
{
	drv->debugfs_dir = debugfs_create_dir("ca_akl", NULL);

	if (!drv->debugfs_dir || IS_ERR_OR_NULL(drv->debugfs_dir))
		dev_err(drv->dev, "debugfs create dentry failed\n");
}

void cert_akl_dbgfs_remove(struct cert_akl_drv *drv)
{
	if (drv && drv->debugfs_dir)
		debugfs_remove_recursive(drv->debugfs_dir);
}

int cert_akl_dbgfs_add(struct cert_akl_key *key)
{
	char name[128];

	if (unlikely(!key ||  !key->drv ||
		!key->drv->debugfs_dir))
		return -EBADF;

	memset(name, 0, sizeof(name));
	sprintf(name, "cert_akl_key_info@%p", key);
	key->debugfs = debugfs_create_file(name, S_IFREG | S_IRUGO,
					    key->drv->debugfs_dir,
					    (void *)key,
					    &cert_akl_dbg_key_info_fops);

	if (!key->debugfs || IS_ERR_OR_NULL(key->debugfs))
		dev_err(key->drv->dev, "debugfs create file failed\n");

	return 0;
}

void cert_akl_dbgfs_del(struct cert_akl_key *key)
{
	if (unlikely(!key || !key->debugfs))
		return;

	debugfs_remove(key->debugfs);
}

