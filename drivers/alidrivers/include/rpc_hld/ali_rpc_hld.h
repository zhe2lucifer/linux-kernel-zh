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

#ifndef __DRIVERS_ALI_RPC_HLD_H
#define __DRIVERS_ALI_RPC_HLD_H

#define RPC_HLD_INTERNAL

#include <alidefinition/adf_basic.h>
#include <alidefinition/adf_ret.h>
#include <alidefinition/adf_media.h>
#include <alidefinition/adf_decv.h>
#include <alidefinition/adf_hld_dev.h>

#include <ali_video_common.h>

typedef void (*OSAL_T_LSR_PROC_FUNC_PTR)(UINT32);
typedef void (*OSAL_T_HSR_PROC_FUNC_PTR)(UINT32);

#define LOW_PRI             31    // Min number, but the highest level
#define HIGH_PRI            0     // Max number,but the lowest level
#define DEF_PRI             20    // default level for normal thread
#define HSR_PRI             10    // HSR level, higher than normal
#define DYN_PRI             18    // Dynamic level, only  once.

#ifndef ALI_OSAL_PRI
#define ALI_OSAL_PRI
enum
{
	OSAL_PRI_LOW		= LOW_PRI,			/* Lowest,  for idle task */
	OSAL_PRI_NORMAL		= DEF_PRI,			/* Normal,  for user task */
	OSAL_PRI_HIGH		= 17,				/* High,    for system task */
	OSAL_PRI_CRITICL	= HSR_PRI,			/* Highest, for HSR scheduler */
};
#endif

enum ALI_RPC_CB_TYPE
{
	ALI_RPC_CB_VDEC,
	ALI_RPC_CB_VPO,
	ALI_RPC_CB_VPO_HDMI,
	ALI_RPC_CB_SND_HDMI,
	ALI_RPC_CB_SNC_SPC,
	ALI_RPC_CB_IMG,
	ALI_RPC_CB_VDE,
	ALI_RPC_CB_MUS,
	ALI_RPC_CB_VDEC_SPEC,
	ALI_RPC_CB_VDEC_INFO,
	ALI_RPC_CB_BOOTMEDIA_HDMI,
	ALI_RPC_CB_SND_FIRST_FRAME_OUTPUT,
    ALI_RPC_CB_DECA_MONITOR_NEW_FRAME, //!<One frame was decoded by audio decoder.
    ALI_RPC_CB_DECA_MONITOR_START, //audio decoder is in start status.
    ALI_RPC_CB_DECA_MONITOR_STOP, //audio decoder is in stop status.
    ALI_RPC_CB_DECA_MONITOR_DECODE_ERR, //audio decoder return one err status.
    ALI_RPC_CB_DECA_MONITOR_OTHER_ERR, //audio decoder occurred other err.
    ALI_RPC_CB_DECA_STATE_CHANGED,  //audio decoder state is changed.
    ALI_RPC_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD, // Moniter Sound card dma data is below the threshold.
    ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END, // Moniter Sound card dma data is occured the end.
    ALI_RPC_CB_SND_MONITOR_ERRORS_OCCURED, // Moniter Sound card is occured some errors.
    ALI_RPC_CB_SND_MONITOR_SBM_MIX_END,     // moniter SBM mix end.
};
/*Forward Declare to clean compile error*/
enum TVSystem;

#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID

#define HLD_VIRTUAL_DEVICE_MAGIC_MSK (0xFFFFFF<<8)
#define HLD_VIRTUAL_DEVICE_MAGIC     (0xED0123<<8)

struct hld_virtual_device
{
	struct hld_virtual_device		*next;				  		/* Next device structure */
	UINT32				        	type; 				  		/* Device type */
	INT8				        	name[HLD_MAX_NAME_SIZE];	/* Device name */
	UINT32                      	virtual_dev_id;             /*Virtual device id*/
};

struct hld_device_ext
{
    struct hld_device_ext        	*next;                		/* Next device structure */
    struct hld_virtual_device    	*virtual_dev;          		/* Virtual device struct */
    UINT32                        	real_dev;             		/* Real device address */
};

//#ifdef DUAL_ENABLE
struct remote_hld_device_ext
{
	struct remote_hld_device_ext	*next;				  		/* Next device structure */
	UINT32				    		type; 				  		/* Device type */
	INT8				    		name[HLD_MAX_NAME_SIZE];	/* Device name */
	UINT32                  		remote_dev_id;
};
//#endif

#endif


typedef void (*ali_rpc_cb_routine)(UINT32 type, UINT32 para);


void ali_rpc_hld_base_callee(UINT8 *msg);

#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID
void *real_dev_get_by_vir_id(UINT32 virtual_dev_id);
void *hld_dev_get_by_name(INT8 *name);
void *hld_dev_get_by_type(void *sdev, UINT32 type);
void *hld_dev_get_by_id(UINT32 type, UINT16 id);
INT32 hld_dev_add_remote(struct hld_virtual_device *vir_dev);
INT32 hld_dev_remove_remote(struct hld_virtual_device *vir_dev);
#else
void *hld_dev_get_by_name(INT8 *name);
void *hld_dev_get_by_type(void *sdev, UINT32 type);
void *hld_dev_get_by_id(UINT32 type, UINT16 id);
INT32 hld_dev_add_remote(struct hld_device *dev, UINT32 dev_addr);
INT32 hld_dev_remove_remote(struct hld_device *dev);
#endif

void hld_dev_memcpy(void *dest, const void *src, unsigned int len);
void hld_dev_see_init(void *addr);

void ali_rpc_register_callback(enum ALI_RPC_CB_TYPE type, void *cb_func);
UINT32 hld_get_see_version(UINT8 *dest);
void hld_disable_see_printf(unsigned long disable);
void hld_hit_see_heart(void);
void hld_enable_see_exception(void);
void hld_show_see_plugin_info(void);
#endif

