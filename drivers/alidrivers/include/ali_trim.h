/*
 * ali_trim.h - ALi Trim Library header file
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih  <david.shih@alitech.com>
 *	    Lucas.Lai	<lucas.lai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
 
#ifndef _ALI_TRIM_H
#define _ALI_TRIM_H

#include <linux/of.h>

/**
 * of_parse_ali_trim - Obtain trim value from otp
 * @dn: Device node of the requester
 * @trim_property_name: Name of property holding trim device node phandles
 * @index: For properties holding a table of phandles, this is the index into
 *         the table
 * @trim_val: pointer to trim_val. 
 *
 * The return value is 0 in the case of success or a negative error code.
 */
int of_parse_ali_trim(struct device_node *dn, const char *trim_property_name,
			int index, u32 *trim_val);

#endif /* end of _ALI_TRIM_H */
