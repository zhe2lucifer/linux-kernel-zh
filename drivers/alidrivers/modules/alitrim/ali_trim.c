/*
 * Trim Library
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/types.h>

#include <ca_otp_dts.h>

int of_parse_ali_trim(struct device_node *dn, const char *trim_property_name,
			int index, u32 *trim_val)
{
	int ret = -1;
	struct device_node *trim_np;

	trim_np = of_parse_phandle(dn, trim_property_name, index);
	if (!trim_np) {
		pr_err("%s: Property '%s' parse fail !!\n",
			__func__, trim_property_name);
		return -EINVAL;
 	}

	/* get info from OTP defined in device tree */
	ret = of_parse_ali_otp(trim_np, "otp-addr-pos-bits", trim_val);
	if (ret || (*trim_val == 0)) {
		if (ret)
			pr_warn("%s: of_parse_ali_otp fail!\n", __func__);

		/* Try to get default value in dts node if exist */
		ret = of_property_read_u32(trim_np, "default", trim_val);
		if (ret) {
			pr_err("%s: get default trim fail!\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}
