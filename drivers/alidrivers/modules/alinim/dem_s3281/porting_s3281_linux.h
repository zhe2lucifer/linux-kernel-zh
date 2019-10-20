/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#ifndef __PORTING_S3281_LINUX_H__
#define __PORTING_S3281_LINUX_H__

#include "../porting_linux_header.h"
#include "../basic_types.h"
#include "../tun_common.h"

#include "../nim_device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_FUNC_ON				0x00000001	/* Function on */
#define SYS_FUNC_OFF			0x00000000	/* Function disable */

#ifdef __cplusplus
}
#endif

#endif

