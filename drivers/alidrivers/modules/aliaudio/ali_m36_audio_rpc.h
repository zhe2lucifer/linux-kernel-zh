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

/*****************************************************************************
*	 Driver for ALi M36 audio device driver
*
*    File:    Linux\kernel\drivers\media\dvb\ali_audio
*
*    Description:    This file contains all globe micros and functions declare
*		             of Ali M36 audio device.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.	2009-11-30     Eric Li  Ver 0.1    Create file.
*
*  	Copyright 2009 ALi Limited
*  	Copyright (C) 2009 ALi Corp.
*****************************************************************************/
#ifndef _ALI_M36_AUDIO_RPC_H
#define _ALI_M36_AUDIO_RPC_H
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
//#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <asm/irq.h>

#include <asm/mach-ali/typedef.h>
//#include <asm/mach-ali/m3602.h>
//#include <asm/mach-ali/gpio.h>

//#include <linux/ali_rpc.h>
//#include <dvb_audio.h>

#include <ali_audio_common.h>
#include <rpc_hld/ali_rpc_hld.h>

#include <rpc_hld/ali_rpc_hld_snd.h>
#include <rpc_hld/ali_rpc_hld_deca.h>

//#include "dvbdev.h"
//#include <ali_rpcng.h>
#include <linux/ali_kumsgq.h>

#define ALI_AUDIO_DEVICE_NAME 	"ali_m36_audio"



struct audio_callback
{
    audio_cb_func   pcb_first_frame_output;
    audio_cb_func   pcb_deca_moniter_new_frame;
    audio_cb_func   pcb_deca_moniter_start;
    audio_cb_func   pcb_deca_moniter_stop;
    audio_cb_func   pcb_deca_moniter_decode_err;
    audio_cb_func   pcb_deca_moniter_other_err;
    audio_cb_func   pcb_deca_state_changed;
    audio_cb_func   pcb_snd_moniter_remain_data_below_threshold;
    audio_cb_func   pcb_snd_moniter_output_data_end;
    audio_cb_func   pcb_snd_moniter_errors_occured;
    audio_cb_func   pcb_snd_moniter_sbm_mix_end;
};


struct ali_audio_device
{
	struct cdev cdev;
	struct deca_device *deca_dev;
	struct snd_device *snd_dev;
    int open_count;
    int deca_open_flag;
    int snd_open_flag;
    struct semaphore sem;

	struct kumsgq *audio_kumsgq;   //cara.shi
	struct mutex audio_mutex;

	ali_audio_ctrl_blk audio_cb;
	int cb_avail;
    unsigned int socket_port_id;
    struct audio_callback call_back;
	unsigned int *pcm_capture_buff;
	unsigned int pcm_capture_buff_len;
};

#endif

