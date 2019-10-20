/*
 * VSC dedicated Key Ladder Core driver.
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
#ifndef __CA_KL_VSC_IOCTL_H__
#define __CA_KL_VSC_IOCTL_H__

#include "ca_kl_priv.h"

#ifdef CONFIG_ALI_VSC
long ca_kl_vsc_ioctl(struct file *file, unsigned int cmd,
			 unsigned long args);
#else
inline long ca_kl_vsc_ioctl(struct file *file, unsigned int cmd,
				unsigned long args)
{
	return -ENOSYS;
}
#endif

#endif /*__CA_KL_VSC_IOCTL_H__*/
