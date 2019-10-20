/*
 * Scramber Core driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
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

#ifndef __CA_SCR_SYSFS_H__
#define __CA_SCR_SYSFS_H__

#include "ca_scr_priv.h"

#ifdef CONFIG_SYSFS
int ca_scr_sysfs_create(struct ca_scr_dev *scr);
void ca_scr_sysfs_remove(struct ca_scr_dev *scr);
#else
inline int ca_scr_sysfs_create(struct ca_scr_dev *scr) { return -ENOSYS; };
inline void ca_scr_sysfs_remove(struct ca_scr_dev *scr) { return; };
#endif

#endif
