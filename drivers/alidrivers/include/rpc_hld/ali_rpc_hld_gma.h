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
 
#ifndef __DRIVERS_ALI_RPC_HLD_GMA_H
#define __DRIVERS_ALI_RPC_HLD_GMA_H

#include <alidefinition/adf_gma.h>

enum LLD_GMA_M36F_FUNC
{
    FUNC_GMA_M36F_ATTACH = 0,
};

enum HLD_GMA_FUNC
{
	FUNC_GMA_OPEN = 0,
	FUNC_GMA_CLOSE,
	FUNC_GMA_IO_CONTROL,
	FUNC_GMA_SHOW,
	FUNC_GMA_SCALE,
	FUNC_GMA_SET_PALLETTE,
	FUNC_GMA_GET_PALLETTE,
	FUNC_GMA_CREATE_REGION,
	FUNC_GMA_DELETE_REGION,
	FUNC_GMA_GET_REGION_INFO,
	FUNC_GMA_SET_REGION_INFO,
	FUNC_GMA_REGION_SHOW,
};

#endif

