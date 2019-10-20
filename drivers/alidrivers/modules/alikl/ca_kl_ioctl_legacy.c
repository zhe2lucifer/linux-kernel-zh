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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/splice.h>

#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include <ca_kl.h>
#include "ca_kl_ioctl_legacy.h"
#include "ca_kl_rpc.h"
#include "ca_kl_vsc_ioctl.h"

int ca_kl_key_dump(struct ca_kl_session *s,
	int kl_sel, int dump_pos, unsigned int *pkey)
{
	CE_DEBUG_KEY_INFO ce_dbg_key;
	int ret;
	struct ca_kl_sub_dev *son = NULL;

	if (dump_pos < 0 || dump_pos > 63 || !pkey)
		return -EINVAL;

	if (!s)
		return -EBADF;

	son = s->pdev;
	if (!son)
		return -EBADF;

	memset(&ce_dbg_key, 0, sizeof(ce_dbg_key));
	ce_dbg_key.sel = CE_KEY_READ;
	ce_dbg_key.len = 4;
	ce_dbg_key.kl_index_sel = kl_sel;
	ce_dbg_key.pos = dump_pos;

	ret = ali_ce_ioctl(son->parent->see_ce_id,
		(CE_IO_CMD(IO_CRYPT_DEBUG_GET_KEY)),
		(__u32)&ce_dbg_key);
	if (0 != ret) {
		dev_dbg(son->dev, "failed, ret:%d\n", ret);
		return -EIO;
	}

	memcpy(pkey, ce_dbg_key.buffer, sizeof(unsigned int)*4);
	return 0;
}


static int ca_kl_key_dbg(struct ca_kl_session *s,
	struct kl_dbg_key *dbg_key)
{
	CE_DEBUG_KEY_INFO ce_dbg_key;
	int ret;
	struct ca_kl_sub_dev *son = NULL;

	if (NULL == dbg_key)
		return -EINVAL;

	if (NULL == s)
		return -EBADF;

	son = s->pdev;
	if (!son)
		return -EBADF;

	memset(&ce_dbg_key, 0, sizeof(ce_dbg_key));
	ce_dbg_key.sel = CE_KEY_READ;
	ce_dbg_key.len = 4;
	ce_dbg_key.kl_index_sel = s->cell->kl_sel;
	ce_dbg_key.pos = 0;

	ret = ali_ce_ioctl(son->parent->see_ce_id,
		(CE_IO_CMD(IO_CRYPT_DEBUG_GET_KEY)),
		(__u32)&ce_dbg_key);
	if (0 != ret) {
		dev_dbg(son->dev, "failed, ret:%d\n", ret);
		return -EIO;
	}

	ret = ali_ce_umemcpy((void __user *)dbg_key->buffer,
		ce_dbg_key.buffer, sizeof(unsigned int)*4);
	if (0 != ret) {
		dev_dbg(son->dev, "failed, ret:%d\n", ret);
		return -EIO;
	}

	return 0;
}

long ca_kl_ioctl_legacy(struct file *file,
	unsigned int cmd, unsigned long args)
{
	int ret = 0;

	struct ca_kl_session *s = NULL;

	s = file2session(file);
	if (!s)
		return -EBADF;

	switch (CE_IO_CMD(cmd)) {
	case CE_IO_CMD(KL_DBG_KEY):
		ret = ca_kl_key_dbg(s, (struct kl_dbg_key *)args);
		break;

	default:
		ret = ca_kl_vsc_ioctl(file, cmd, args);
		if (ret)
			dev_dbg(s->pdev->dev, "invalid ioctl\n");
		break;
	}

	return ret;
}

