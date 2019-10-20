/*
 * CERT utils for ASA and AKL
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
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <alidefinition/adf_cert.h>
#include "ali_cert_utils.h"

int cert_umemcpy(void *dest, const void *src, __u32 n)
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

int cert_uret(int rpc_ret)
{
	int ret = -EPERM;

	switch (rpc_ret) {
	case CERT_NO_ERROR:
		ret = 0;
		break;
	case (-CERT_ERROR_INVALID_PARAM):
		ret = -EINVAL;
		break;
	case (-CERT_ERROR_INVALID_CHAN):
		ret = -EBADR;
		break;
	case (-CERT_ERROR_NODEV):
		ret = -ENODEV;
		break;
	case (-CERT_ERROR_CHANFULL):
	case (-CERT_ERROR_PIDFULL):
		ret = -EBUSY;
		break;
	case (-CERT_ERROR_NOT_SUPPORTED):
		ret = -ENOTSUPP;
		break;
	case (-CERT_ERROR_TIMEOUT):
		ret = -ETIMEDOUT;
		break;
	case (-CERT_ERROR_ALWAYS_BUSY):
		ret = -EDEADLK;
		break;
	case (-CERT_ERROR_SAVE_KEY):
		ret = -EBADRQC;
		break;
	case (-CERT_ERROR_EXCHANGE_STATUS):
		ret = -EBADE;
		break;
	default:
		ret = rpc_ret > 0 ? rpc_ret : -EPERM;
		break;
	}

	return ret;
}

