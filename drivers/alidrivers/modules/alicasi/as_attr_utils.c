/*
 * CASI utils Fxn.
 *
 * Copyright (c) ALi.
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
#include <rpc_hld/ali_rpc_hld.h>
#include "as_attr_utils.h"
#include "../alikl/ca_kl_priv.h"

int as_attr_umemcpy(void *dest, const void *src, __u32 n)
{
	int ret = -EFAULT;
	int sflag = access_ok(VERIFY_READ, (void __user *)src, n);
	int dflag = access_ok(VERIFY_WRITE, (void __user *)dest, n);

	if (segment_eq(get_fs(), USER_DS)) {
		if (sflag && !dflag)
			ret = copy_from_user(dest,
				(void __user *)src, n);
		else	if (dflag && !sflag)
			ret = copy_to_user(dest, src, n);
		else if (!sflag && !dflag)
			memcpy(dest, src, n);
		else
			return -EFAULT;
	} else {
		memcpy(dest, src, n);
		ret = 0;
	}

	return ret;
}

int as_attr_get_kl_res(int kl_fd, struct as_attr_device *as_attr_dev)
{
	int ret;
	struct fd sfd;
	struct ca_kl_session *session = NULL;
	
	ret = fetch_key_cell_by_fd(kl_fd, &as_attr_dev->kl_attr.key_cell);
	if (unlikely(!as_attr_dev->kl_attr.key_cell)) {
		pr_err("fetch key_cell failed\n");
		goto out1;
	}

	sfd = fdget(kl_fd);
	if (unlikely(!sfd.file)) {
		ret = -EBADF;
		pr_err("fetch kl file failed\n");
		goto out2;
	}

	session = sfd.file->private_data;
	if (unlikely(!session)) {
		ret = -EFAULT;
		pr_err("fetch kl session failed\n");
		goto out2;
	}

	as_attr_dev->kl_attr.algo = session->algo;
	as_attr_dev->kl_attr.crypt_mode = session->crypt_mode;
	as_attr_dev->kl_attr.level = session->level;
	
out2:
	fdput(sfd);
out1:
	return ret;	
}
EXPORT_SYMBOL_GPL(as_attr_get_kl_res);

int as_attr_get_dsc_res(int fd, struct as_attr_device *as_attr_dev)
{
	int ret;
	ret =  ca_dsc_get_session_attr(fd, &as_attr_dev->dsc_attr.dsc_ses_attr);
	if(ret < 0) {
		pr_err("get dsc session attibute failed\n");
		goto out;
	}
out:
	return ret;
}
EXPORT_SYMBOL_GPL(as_attr_get_dsc_res);

