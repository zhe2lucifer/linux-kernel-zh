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
 
#ifndef _ALI_DSC_H_
#define _ALI_DSC_H_

#include <linux/types.h>
#include <ali_dsc_common.h>


struct ca_session_attr {
	void *sub_dev_see_hdl;
		/*!< Pointer of the sub device handler at see for the current session. */
	unsigned short stream_id;
		/*!< Stream id of the current session. */
	int crypt_mode;
		/*!< Crypt mode of the current session. */        
    unsigned short sub_dev_id;
        /*sub dev id */
};


struct ca_key_attr {
	unsigned int key_handle;
		/*!< Key handle at see for the current key. */
	int kl_sel;
		/*!< KeyLadder key sel for the current key. */
	int key_pos;
		/*!< Key pos for the current key. */
};

__s32 ca_dsc_get_session_attr(__s32 ca_fd, struct ca_session_attr* p_ca_attr);
__s32 ca_dsc_get_key_attr(__s32 ca_fd, __s32 key_id, struct ca_key_attr* p_key_attr);

#endif 
