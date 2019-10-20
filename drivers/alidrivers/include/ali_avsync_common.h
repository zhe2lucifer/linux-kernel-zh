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
 
#ifndef __ALI_AVSYNC_H__
#define __ALI_AVSYNC_H__

/*! @addtogroup DeviceDriver
 *  @{
 */

/*! @addtogroup ALiAVSYNC
 *  @{
 */

#include <linux/types.h>
#include <ali_magic.h>
#include "ali_basic_common.h"
#include "alidefinition/adf_avsync.h"

#define ALI_AVSYNC_RESET 							_IO(ALI_AVSYNC_MAGIC, 10)                               //!<Reset avsync module.
#define ALI_AVSYNC_START  							_IO(ALI_AVSYNC_MAGIC, 11)                               //!<Start avsync module.
#define ALI_AVSYNC_STOP    							_IO(ALI_AVSYNC_MAGIC, 12)                               //!<Stop avsync module.
#define ALI_AVSYNC_SET_SYNC_MODE  					_IOW(ALI_AVSYNC_MAGIC, 13, unsigned long)               //!<Set sync mode.
#define ALI_AVSYNC_GET_SYNC_MODE   					_IOR(ALI_AVSYNC_MAGIC, 14, unsigned long)               //!<Get sync mode.
#define ALI_AVSYNC_SET_SOURCE_TYPE					_IOW(ALI_AVSYNC_MAGIC, 15, unsigned long)               //!<Set source type.
#define ALI_AVSYNC_GET_SOURCE_TYPE					_IOR(ALI_AVSYNC_MAGIC, 16, AVSYNC_SRCTYPE_E)            //!<Get source type.
#define ALI_AVSYNC_CONFIG_PARAMS					_IOW(ALI_AVSYNC_MAGIC, 17, avsync_cfg_param_t)          //!<Configure basic parameters.
#define ALI_AVSYNC_GET_PARAMS						_IOR(ALI_AVSYNC_MAGIC, 18, avsync_cfg_param_t)          //!<Get basic parameters.
#define ALI_AVSYNC_CONFIG_ADVANCE_PARAMS			_IOW(ALI_AVSYNC_MAGIC, 19, avsync_adv_param_t)          //!<Configure advanced parameters.
#define ALI_AVSYNC_GET_ADVANCE_PARAMS				_IOR(ALI_AVSYNC_MAGIC, 20, avsync_adv_param_t)          //!<Get advanced parameters.
#define ALI_AVSYNC_GET_STATUS						_IOR(ALI_AVSYNC_MAGIC, 21, avsync_status_t)             //!<Get avsync status.
#define ALI_AVSYNC_GET_STATISTICS					_IOR(ALI_AVSYNC_MAGIC, 22, avsync_statistics_t)         //!<Get avsync statistics.
#define ALI_AVSYNC_VIDEO_SMOOTHLY_PLAY_ONOFF	    _IOW(ALI_AVSYNC_MAGIC, 23, struct ali_avsync_rpc_pars)  //!<Configure video play mode:still frame or smooth channel change.
#define ALI_AVSYNC_GET_STC							_IOR(ALI_AVSYNC_MAGIC, 24, struct ali_avsync_rpc_pars)  //!<Get current STC.
#define ALI_AVSYNC_GET_CONTROL_BLOCK_OTHERS	 	    _IOR(ALI_AVSYNC_MAGIC, 25, struct ali_avsync_rpc_pars)  //!<Not in use.


#define ALI_AVSYNC_IO_COMMAND		                128	//!<Avsync IO command.

#define ALI_AVSYNC_SET_DBG_PRINT_OPTION		        240 //!< Recommend: Do not use.
#define ALI_AVSYNC_SET_DBG_POLL_ONOFF		        241 //!< Recommend: Do not use.
#define ALI_AVSYNC_SET_DBG_POLL_OPTION		        242 //!< Recommend: Do not use.
#define ALI_AVSYNC_SET_DBG_POLL_INTERVAL		    243 //!< Recommend: Do not use.


struct ali_avsync_rpc_arg
{
	void *arg;
	int arg_size;
	int out;
};

struct ali_avsync_rpc_pars
{
	int API_ID;
	struct ali_avsync_rpc_arg arg[4];
	int arg_num;
};

/*! @struct ali_avsync_ioctl_command
 @brief Used for ALI_AVSYNC_IO_COMMAND parameters.
 */

struct ali_avsync_ioctl_command
{
	unsigned long ioctl_cmd;//!<IO command name.
	unsigned long param;//!<Parameters.
};

/*!
@}
*/

/*!
@}
*/

#endif
