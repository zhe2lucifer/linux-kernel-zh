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
 
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
//#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ali_rpc.h>

#include <rpc_hld/ali_rpc_hld_vbi.h>

#include "../ali_rpc.h"
#include <adf_vbi.h>




enum LLD_TTX_VBI_FUNC{
	FUNC_TTX_VBI_OPEN = 0,   
	FUNC_TTX_VBI_CLOSE,
	FUNC_TTX_VBI_WRITE,	
	FUNC_TTX_VBI_IOCTL,
};

void vbi_see_start(VBI_SOURCE_TYPE type)
{	
	printk("%s-%d\n",__FUNCTION__,__LINE__);
    jump_to_func(NULL, ali_rpc_call, type, (LLD_VBI_M33_MODULE<<24)|(1<<16)|FUNC_TTX_VBI_OPEN, NULL);
	
}
EXPORT_SYMBOL(vbi_see_start);


void vbi_see_stop(void)
{
    jump_to_func(NULL, ali_rpc_call, NULL, (LLD_VBI_M33_MODULE<<24)|(0<<16)|FUNC_TTX_VBI_CLOSE, NULL);
}
EXPORT_SYMBOL(vbi_see_stop);

static UINT32 vbi_write_para[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct vbi_data_array_t)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

UINT8 write_ttx_packet(struct vbi_data_array_t *p_data,UINT32 size)
{
	//printk("%s:%d\n",__FUNCTION__,__LINE__);
	jump_to_func(NULL, ali_rpc_call, p_data, (LLD_VBI_M33_MODULE<<24)|(2<<16)|FUNC_TTX_VBI_WRITE,vbi_write_para);
	//printk("%s:%d\n",__FUNCTION__,__LINE__);
}
EXPORT_SYMBOL(write_ttx_packet);



static UINT32 desc_vbi_ioctl[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, 0),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

RET_CODE ttx_vbi_ioctl(UINT32 cmd,UINT32 param)
{
	UINT32 i;
	UINT32 common_desc[sizeof(desc_vbi_ioctl)];
	UINT32 *desc = (UINT32 *)common_desc;
	UINT32 *b = (UINT32 *)desc_vbi_ioctl;

	for(i = 0; i < sizeof(desc_vbi_ioctl)/sizeof(UINT32); i++)
		desc[i] = b[i];

	switch(cmd)
	{
		
		case IO_VBI_SET_VPO_HD_SD_PARAM:
		case IO_VBI_CHECK_TTX_TASK_START:	
		case IO_VBI_SELECT_OUTPUT_DEVICE:
		default:
			desc = NULL;
			break;
	}
	printk("%s-cmd=0x%x\n",__FUNCTION__,(unsigned int)cmd);
	jump_to_func(NULL, ali_rpc_call, cmd, (LLD_VBI_M33_MODULE<<24)|(2<<16)|FUNC_TTX_VBI_IOCTL, desc);


}
EXPORT_SYMBOL(ttx_vbi_ioctl);



