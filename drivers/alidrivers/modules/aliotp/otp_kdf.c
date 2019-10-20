/*
 * One Time Programming Core driver
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "otp_priv.h"

#define KDF_CTRL (0x120)
#define KDF_STATUS (0x124)

#define KDF_CTRL_ROOTKEY_SEL_BIT	(11)
/* 0: key0, 1: key1 */
#define KDF_CTRL_KDF_TYPE_BIT	(8)
/* 0: kdf, 1: etsi_kdf */
#define KDF_CTRL_KDF_TEXT_BIT	(9)
/* [10:9] --00:text1, 01:text2, 10:text3, 11:text4 */
#define KDF_CTRL_TRIGGER_BIT (0)

static int otp_kdf_busy(struct otp_dev *otp)
{
	unsigned int live = 2000;

	while ((ioread32(otp->base + KDF_STATUS) & 1) &&
			(live--))
		msleep_interruptible(1);

	return !live;
}


int otp_etsi_kdf_trigger(struct otp_dev *otp, struct otp_vendor_id *pvid)
{
	int i, value = 0;

	if (!otp || !pvid)
		return -EINVAL;
		
	if (1 == pvid->isValid) {
		iowrite32(pvid->HighVID, otp->base + OTP_VENDOR_ID_HIGH);
		iowrite32(pvid->LowVID, otp->base + OTP_VENDOR_ID_LOW);
	}
	
	iowrite32(1 << KDF_CTRL_KDF_TYPE_BIT,
			otp->base + KDF_CTRL);

	for (i = 0; i < 3; i++) {
		value = ioread32(otp->base + KDF_CTRL) &
				~(3 << KDF_CTRL_KDF_TEXT_BIT);
		value |= i << KDF_CTRL_KDF_TEXT_BIT;
		iowrite32(value, otp->base + KDF_CTRL);

		value |= 1 << KDF_CTRL_TRIGGER_BIT;
		iowrite32(value, otp->base + KDF_CTRL);
		if (otp_kdf_busy(otp))
			return -EBUSY;
	}

	return 0;
}

int otp_std_kdf_trigger(struct otp_dev *otp,
		int root, int text_idx)
{
	int value = 0;

	if (!otp)
		return -EINVAL;

	if ((root != 0) && (root != 1))
		return -EINVAL;

	if (text_idx < 0 || text_idx > 3)
		return -EINVAL;

	iowrite32(root << KDF_CTRL_ROOTKEY_SEL_BIT,
			otp->base + KDF_CTRL);

	value = ioread32(otp->base + KDF_CTRL) &
			~(3 << KDF_CTRL_KDF_TEXT_BIT);
	value |= text_idx << KDF_CTRL_KDF_TEXT_BIT;
	iowrite32(value, otp->base + KDF_CTRL);

	value |= 1 << KDF_CTRL_TRIGGER_BIT;
	iowrite32(value, otp->base + KDF_CTRL);
	if (otp_kdf_busy(otp))
		return -EBUSY;
		
	return 0;
}

