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

 /*
 *      File:           alihwdma-ioctl.h
 *      Version:        1.0
 *      Date:           2012-03-09
 *      Description:    ali bdma module io control header file.
 */
 
#ifndef _LINUX_ALI_HWDMA_IOCTL_H
#define _LINUX_ALI_HWDMA_IOCTL_H

#define ALI_HWDMA_IOC_MAGIC 'J'

#include <asm/ioctl.h>
#include "ali_bdma.h"

#define ALI_HWDMA_START     			_IOW(ALI_HWDMA_IOC_MAGIC, 1, ali_hwdma_job*)
#define ALI_HWDMA_2D_START  			_IOW(ALI_HWDMA_IOC_MAGIC, 2, ali_hwdma_2Djob*)
#define ALI_HWDMA_Reset	    			_IO(ALI_HWDMA_IOC_MAGIC, 3)
#define ALI_HWDMA_SET_LATENCY  			_IOW(ALI_HWDMA_IOC_MAGIC, 4, unsigned char)
#define ALI_HWDMA_SET_BLOCK_MODE		_IOW(ALI_HWDMA_IOC_MAGIC, 5, bool)				/* Set BDMA to block mode true: enable(default) false: disable */
#define ALI_HWDMA_WAIT_FINISH_TIMEOUT	_IOW(ALI_HWDMA_IOC_MAGIC, 5, unsigned int)		/* Wait BDMA Finish or timeout than returen */
#endif
