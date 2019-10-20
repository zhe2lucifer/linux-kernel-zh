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
 
/****************************************************************************
 
 *  File: ali_sbm.h
 *
 *  Description: ali share buffer memory for cpu & see access
 *s
 *  History:
 *      Date             Author         Version      Comment
 *      ======           ======          =====       =======
 *  1.  2011.08.03       Dylan.Yang     0.1.000     First version Created
 ****************************************************************************/
#ifndef __ALI_SBM_COMMON_H
#define __ALI_SBM_COMMON_H

#define SBM_MODE_NORMAL          0
#define SBM_MODE_PACKET          1

#define SBM_MUTEX_LOCK           0
#define SBM_SPIN_LOCK            1

struct sbm_config {
    unsigned int buffer_addr;
    unsigned int buffer_size;
    unsigned int block_size;
    unsigned int reserve_size;
    unsigned int wrap_mode;
    unsigned int lock_mode;
};

struct sbm_buf{
    char *buf_addr;
    unsigned int buf_size;
};

struct sbm_req_buf{
	unsigned char *phy_addr;
	unsigned int req_size;
};

#define SBM_MAGIC                0x56
#define SBMIO_CREATE_SBM         _IOW(SBM_MAGIC, 0, struct sbm_config)
#define SBMIO_RESET_SBM          _IO(SBM_MAGIC, 1)
#define SBMIO_SHOW_VALID_SIZE    _IOR(SBM_MAGIC, 2, unsigned int)
#define SBMIO_SHOW_FREE_SIZE     _IOR(SBM_MAGIC, 3, unsigned int)
#define SBMIO_SHOW_TOTAL_SIZE    _IOR(SBM_MAGIC, 4, unsigned int)
#define SBMIO_SHOW_PKT_NUM       _IOR(SBM_MAGIC, 5, unsigned int)
#define SBMIO_REQUEST_WRITE      _IOWR(SBM_MAGIC, 6, struct sbm_buf)
#define SBMIO_IS_FULL            _IOR(SBM_MAGIC, 7, unsigned int)
#define SBMIO_DESTROY_SBM        _IO(SBM_MAGIC, 8)
#define SBMIO_REQ_BUF_INFO       _IOW(SBM_MAGIC,9,struct sbm_req_buf)
#define SBMIO_UPDATE_BUF_INFO    _IOW(SBM_MAGIC,10,struct sbm_req_buf)

#endif

