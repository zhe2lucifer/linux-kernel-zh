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
*    File:    nim_s3503_DVBS.h
*
*    Description:    Header file in LLD.
*    History:
*   		Date			Athor   	 	Version		  				Reason
*	============	=======	===============	=================
*   	1.  08/29/2012  	Russell		 	Ver 0.1			 Create file for S3503 DVBS2 project
*
*****************************************************************************/

#ifndef __LINUX_NIM_DEVICE_H__
#define __LINUX_NIM_DEVICE_H__


struct nim_device
{
    struct  cdev cdev;
    void    *priv;
	struct class 					*class;
	struct device                   *ali_nim_dev_node;
    struct workqueue_struct 		*workqueue;
    struct work_struct 				work;
	struct delayed_work				delay_work;
    struct workqueue_struct 		*autoscan_work_queue;
    struct work_struct				as_work;
	struct delayed_work				delay_as_work;
	struct workqueue_struct 		*plsn_gen_table_workqueue;
	struct work_struct				plsn_gen_table_work;
	struct delayed_work				delay_plsn_gen_table_work;
	struct workqueue_struct 		*plsn_search_workqueue;
	struct work_struct				plsn_search_work;
	struct delayed_work				delay_plsn_search_work;
	struct workqueue_struct         *channel_change_workqueue;
	struct delayed_work				delay_channel_change_work;
	struct work_struct 	   			debug_work;
	bool   							queue_exit_flag;
};



#endif	// __LLD_NIM_S3501_H__ */


